#ifndef ICAL_PARSER_H
#define ICAL_PARSER_H

#include "config.h"

// ============================================================================
// CALENDARIO iCal
// ============================================================================

String icalUnescape(String s) {
  s.replace("\\n", "\n");
  s.replace("\\N", "\n");
  s.replace("\\,", ",");
  s.replace("\\;", ";");
  s.replace("\\\\", "\\");
  return s;
}

time_t parseICalDateToTime(const String &val, bool isUtc, bool dateOnly) {
  struct tm tm;
  memset(&tm, 0, sizeof(tm));

  if (dateOnly) {
    tm.tm_year = val.substring(0, 4).toInt() - 1900;
    tm.tm_mon = val.substring(4, 6).toInt() - 1;
    tm.tm_mday = val.substring(6, 8).toInt();
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;
  } else {
    tm.tm_year = val.substring(0, 4).toInt() - 1900;
    tm.tm_mon = val.substring(4, 6).toInt() - 1;
    tm.tm_mday = val.substring(6, 8).toInt();
    tm.tm_hour = val.substring(9, 11).toInt();
    tm.tm_min = val.substring(11, 13).toInt();
    tm.tm_sec = val.substring(13, 15).toInt();
    tm.tm_isdst = -1;
  }

  if (isUtc) {
    // Calcolo manuale timegm per evitare l'overhead estremo di tzset()
    int year = tm.tm_year + 1900;
    int month = tm.tm_mon;
    int days = 0;
    for (int y = 1970; y < year; y++) {
      days += (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)) ? 366 : 365;
    }
    static const int days_before_month[2][12] = {
        {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
        {0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
    };
    int leap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 1 : 0;
    days += days_before_month[leap][month < 0 ? 0 : (month > 11 ? 11 : month)];
    days += tm.tm_mday - 1;
    return (time_t)days * 86400LL + tm.tm_hour * 3600LL + tm.tm_min * 60LL + tm.tm_sec;
  } else {
    return mktime(&tm);
  }
}

void processLine(String &line, bool &inEvent, Event &curr, bool &inTask, Task &currTask);
void parseICalStream(WiFiClient *stream);

void fetchICalUrl(const char* url) {
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();
  HTTPClient http;

  http.setTimeout(10000);          // era default 5s, ma aumenta la banda

  Serial.printf("Scarico iCal da: %s\n", url);
  if (http.begin(*client, url)) {
    http.addHeader("Accept-Encoding", "identity"); // disabilita gzip se non lo gestisci
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      WiFiClient *stream = http.getStreamPtr();
      parseICalStream(stream);
    } else {
      Serial.printf("HTTP error: %d\n", httpCode);
    }
    http.end();
  } else {
    Serial.println("http.begin() fallito");
  }
  delete client;
}

void fetchAndParseICal() {
  WiFi.setSleep(false);            // disabilita il power saving WiFi — GRANDE impatto
  now = time(nullptr);
  eventsCount = 0;
  tasksCount = 0;

  fetchICalUrl(ICAL_URL);
#ifdef TASK_ICAL_URL
  fetchICalUrl(TASK_ICAL_URL);
#endif

  // Ordina gli eventi in ordine cronologico (dal più vicino al più lontano)
  for (int i = 0; i < eventsCount - 1; i++) {
    for (int j = 0; j < eventsCount - i - 1; j++) {
      if (events[j].start > events[j + 1].start) {
        Event temp = events[j];
        events[j] = events[j + 1];
        events[j + 1] = temp;
      }
    }
  }
}

void parseICalStream(WiFiClient *stream) {
  String lastLine = "";
  lastLine.reserve(512);
  bool inEvent = false;
  Event curr;
  bool inTask = false;
  Task currTask;

  const size_t CHUNK = 1024;
  uint8_t buf[CHUNK];
  String leftover = "";

  while (stream->connected() || stream->available()) {
    if (stream->available()) {
      size_t len = stream->readBytes(buf, min((size_t)stream->available(), CHUNK));
      String chunk = leftover + String((char*)buf).substring(0, len);
      leftover = "";
      
      int start = 0;
      int nl;
      while ((nl = chunk.indexOf('\n', start)) >= 0) {
        String raw = chunk.substring(start, nl);
        raw.replace("\r", "");
        start = nl + 1;
        
        if (raw.length() > 0) {
          if (raw[0] == ' ' || raw[0] == '\t') {
            lastLine += raw.substring(1);
          } else {
            if (lastLine.length() > 0) processLine(lastLine, inEvent, curr, inTask, currTask);
            lastLine = raw;
          }
        }
      }
      leftover = chunk.substring(start);
    } else {
      delay(1);
    }
  }
  // flush
  if (leftover.length() > 0 || lastLine.length() > 0) {
    lastLine += leftover;
    processLine(lastLine, inEvent, curr, inTask, currTask);
  }
}

void processLine(String &line, bool &inEvent, Event &curr, bool &inTask, Task &currTask) {
  const char* raw = line.c_str();
  const char* colon = strchr(raw, ':');
  if (!colon) return;

  int colonPos = colon - raw;
  String name = line.substring(0, colonPos);
  String value = line.substring(colonPos + 1);
  name.trim();
  value.trim();

  // Estrai la chiave base (prima del ';' se presente)
  String key = name;
  int semi = name.indexOf(';');
  if (semi != -1) key = name.substring(0, semi);

  if (key == "BEGIN") {
    if (value == "VEVENT") {
      inEvent = true;
      curr = Event();
    } else if (value == "VTODO") {
      inTask = true;
      currTask = Task();
      currTask.done = false;
      currTask.due = 0;
    }
  } else if (key == "END") {
    if (value == "VEVENT") {
      inEvent = false;

      // Se è un task (icona 🎯), lo aggiungiamo a tasks e lo saltiamo per events
      if (curr.summary.indexOf("🎯") >= 0) {
        if (tasksCount < MAX_TASKS) {
          Task t;
          String stripped = curr.summary;
          int spaceIdx = stripped.indexOf(" ");
          if (spaceIdx >= 0 && spaceIdx < 6) {
            stripped = stripped.substring(spaceIdx + 1);
          }
          t.title = stripped;
          t.title.trim();
          t.done = false;
          t.due = curr.start; 
          tasks[tasksCount++] = t;
        }
        return; // Salta il resto del processing (non aggiungerlo a events)
      }

      time_t limit = now + (31 * 24 * 60 * 60);

      if (curr.rruleWeekly) {
        // Espandi tutte le occorrenze settimanali nella finestra now..limit
        time_t duration = (curr.end != 0) ? (curr.end - curr.start) : (curr.allDay ? 86400 : 0);
        time_t until = (curr.rruleUntil != 0) ? curr.rruleUntil : limit;
        time_t step = (time_t)curr.rruleInterval * 7 * 24 * 3600;
        for (time_t occ = curr.start; occ <= min(limit, until); occ += step) {
          time_t occEnd = occ + duration;
          if (occEnd >= now && eventsCount < MAX_EVENTS) {
            Event copy = curr;
            copy.start = occ;
            copy.end   = (curr.end != 0) ? occEnd : curr.end;
            copy.rruleWeekly = false; // non ri-espandere
            events[eventsCount++] = copy;
          }
        }
      } else {
        time_t effectiveEnd = (curr.end != 0) ? curr.end : (curr.start + (curr.allDay ? 86400 : 0));
        if (effectiveEnd >= now && curr.start <= limit && eventsCount < MAX_EVENTS) {
          events[eventsCount++] = curr;
        }
      }
    } else if (value == "VTODO") {
      inTask = false;
      if (tasksCount < MAX_TASKS) {
        tasks[tasksCount++] = currTask;
      }
    }
  } else if (inEvent) {
    if (key == "SUMMARY") {
      curr.summary = icalUnescape(value);
    } else if (key == "DESCRIPTION") {
      curr.description = icalUnescape(value);
    } else if (key == "DTSTART") {
      bool isUtc = value.endsWith("Z");
      // Gestisci parametri come TZID=Europe/Rome nel nome del campo
      String clean;
      if (isUtc) {
        clean = value.substring(0, value.length() - 1);
      } else {
        // rimuovi eventuale part 'T' prefix check
        clean = value;
      }
      bool dateOnly = (clean.indexOf('T') == -1);
      curr.allDay = dateOnly;
      // Per TZID (ora locale), isUtc=false → mktime() applica il fuso locale
      curr.start = parseICalDateToTime(clean, isUtc, dateOnly);
    } else if (key == "DTEND") {
      bool isUtc = value.endsWith("Z");
      String clean = isUtc ? value.substring(0, value.length() - 1) : value;
      bool dateOnly = (clean.indexOf('T') == -1);
      curr.end = parseICalDateToTime(clean, isUtc, dateOnly);
    } else if (key == "RRULE") {
      // Parsing minimale: FREQ=WEEKLY;UNTIL=YYYYMMDD[T...];INTERVAL=N
      if (value.indexOf("FREQ=WEEKLY") >= 0) {
        curr.rruleWeekly = true;
        // Leggi INTERVAL
        int iIdx = value.indexOf("INTERVAL=");
        if (iIdx >= 0) {
          curr.rruleInterval = value.substring(iIdx + 9).toInt();
          if (curr.rruleInterval < 1) curr.rruleInterval = 1;
        } else {
          curr.rruleInterval = 1;
        }
        // Leggi UNTIL
        int uIdx = value.indexOf("UNTIL=");
        if (uIdx >= 0) {
          String untilStr = value.substring(uIdx + 6);
          // Tronca al ';' successivo se presente
          int sc = untilStr.indexOf(';');
          if (sc >= 0) untilStr = untilStr.substring(0, sc);
          bool uUtc = untilStr.endsWith("Z");
          if (uUtc) untilStr = untilStr.substring(0, untilStr.length() - 1);
          bool uDate = (untilStr.indexOf('T') == -1);
          curr.rruleUntil = parseICalDateToTime(untilStr, uUtc, uDate);
        } else {
          curr.rruleUntil = 0;
        }
      }
    }
  } else if (inTask) {
    if (key == "SUMMARY") {
      currTask.title = icalUnescape(value);
    } else if (key == "STATUS") {
      if (value == "COMPLETED") currTask.done = true;
    } else if (key == "DUE") {
      bool isUtc = value.endsWith("Z");
      String clean = isUtc ? value.substring(0, value.length() - 1) : value;
      bool dateOnly = (clean.indexOf('T') == -1);
      currTask.due = parseICalDateToTime(clean, isUtc, dateOnly);
    }
  }
}

void printEvents() {
  Serial.println("=== Eventi ===");
  for (int i = 0; i < eventsCount; i++) {
    Event &e = events[i];
    Serial.printf("--- Evento %d ---\n", i + 1);
    Serial.printf("SUMMARY: %s\n", e.summary.c_str());
    if (e.allDay) {
      struct tm *tm = localtime(&e.start);
      char buf[64];
      strftime(buf, sizeof(buf), "%Y-%m-%d (all-day)", tm);
      Serial.printf("DATA: %s\n", buf);
    } else {
      struct tm *tm = localtime(&e.start);
      char buf[64];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
      Serial.printf("START: %s\n", buf);
      if (e.end != 0) {
        struct tm *tm2 = localtime(&e.end);
        char buf2[64];
        strftime(buf2, sizeof(buf2), "%Y-%m-%d %H:%M:%S", tm2);
        Serial.printf("END:   %s\n", buf2);
      }
    }
    if (e.description.length()) {
      Serial.printf("DESC: %s\n", e.description.c_str());
    }
  }
}

#endif // ICAL_PARSER_H

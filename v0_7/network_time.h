#ifndef NETWORK_TIME_H
#define NETWORK_TIME_H

#include "config.h"

// ============================================================================
// GESTIONE RETE E TEMPO
// ============================================================================

void connessioneWiFi() {
  int tentativi = 0;
  const int maxTentativi = 5;

  Serial.print("📶 Connessione al WiFi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED && tentativi < maxTentativi) {
    delay(1000);
    Serial.print(".");
    tentativi++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅🛜 WiFi connesso!");
    Serial.print("🌐 IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ WiFi non connesso, proseguo con il programma...");
  }
}

void connessioneNTP() {
  int tentativi = 0;
  const int maxTentativi = 5;

  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  Serial.println("⏱ Attendo sincronizzazione NTP...");

  while (!getLocalTime(&timeinfo) && tentativi < maxTentativi) {
    delay(500);
    Serial.print("⌛");
    tentativi++;
  }

  if (tentativi < maxTentativi) {
    Serial.println("\n✅🕒 Ora e data ottenute!");
  } else {
    Serial.println("\n❌ NTP non sincronizzato, proseguo con il programma...");
  }
}

String getDateLong() {
  if (!getLocalTime(&timeinfo)) return "N/A";

  const char *daysOfWeek[] = { "domenica", "lunedi", "martedi", "mercoledi",
                               "giovedi", "venerdi", "sabato" };
  const char *months[] = { "gen", "feb", "mar", "apr", "mag", "giu",
                           "lug", "ago", "set", "ott", "nov", "dic" };

  char buffer[40];
  sprintf(buffer, "%s %02d %s %04d",
          daysOfWeek[timeinfo.tm_wday],
          timeinfo.tm_mday,
          months[timeinfo.tm_mon],
          timeinfo.tm_year + 1900);
  return String(buffer);
}

String getTime() {
  if (!getLocalTime(&timeinfo)) return "N/A";

  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d",
          timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return String(buffer);
}

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
    int y = val.substring(0, 4).toInt();
    int m = val.substring(4, 6).toInt();
    int d = val.substring(6, 8).toInt();
    tm.tm_year = y - 1900;
    tm.tm_mon = m - 1;
    tm.tm_mday = d;
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    tm.tm_isdst = -1;
    return mktime(&tm);
  } else {
    int y = val.substring(0, 4).toInt();
    int mo = val.substring(4, 6).toInt();
    int da = val.substring(6, 8).toInt();
    int hh = val.substring(9, 11).toInt();
    int mm = val.substring(11, 13).toInt();
    int ss = val.substring(13, 15).toInt();
    tm.tm_year = y - 1900;
    tm.tm_mon = mo - 1;
    tm.tm_mday = da;
    tm.tm_hour = hh;
    tm.tm_min = mm;
    tm.tm_sec = ss;
    tm.tm_isdst = -1;

    char *oldTZ = getenv("TZ");
    char *oldCopy = NULL;
    if (oldTZ) oldCopy = strdup(oldTZ);
    if (isUtc) {
      setenv("TZ", "UTC0", 1);
      tzset();
    }
    time_t t = mktime(&tm);
    if (oldCopy) {
      setenv("TZ", oldCopy, 1);
      free(oldCopy);
    } else {
      unsetenv("TZ");
    }
    tzset();
    return t;
  }
}

void fetchAndParseICal() {
  now = time(nullptr);
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();
  HTTPClient http;

  Serial.println("Scarico iCal...");
  if (http.begin(*client, ICAL_URL)) {
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

void parseICalStream(WiFiClient *stream) {
  eventsCount = 0;
  String lastLine = "";
  bool inEvent = false;
  Event curr;

  while (stream->connected() || stream->available()) {
    String raw = stream->readStringUntil('\n');
    raw.trim();
    if (raw.length() == 0) continue;

    if (raw[0] == ' ' || raw[0] == '\t') {
      lastLine += raw.substring(1);
      continue;
    } else {
      if (lastLine.length() > 0) {
        processLine(lastLine, inEvent, curr);
      }
      lastLine = raw;
    }
  }

  if (lastLine.length() > 0) {
    processLine(lastLine, inEvent, curr);
  }
}

void processLine(String &line, bool &inEvent, Event &curr) {
  int colon = line.indexOf(':');
  String name = (colon >= 0) ? line.substring(0, colon) : line;
  String value = (colon >= 0) ? line.substring(colon + 1) : "";
  name.trim();
  value.trim();

  if (name == "BEGIN" && value == "VEVENT") {
    inEvent = true;
    curr = Event();
  } else if (name == "END" && value == "VEVENT") {
    inEvent = false;
    if (curr.start >= now && eventsCount < MAX_EVENTS) {
      events[eventsCount++] = curr;
    }
  } else if (inEvent) {
    String key = name;
    int semi = name.indexOf(';');
    if (semi != -1) key = name.substring(0, semi);

    if (key == "SUMMARY") curr.summary = icalUnescape(value);
    else if (key == "DESCRIPTION") curr.description = icalUnescape(value);
    else if (key == "DTSTART") {
      bool isUtc = value.endsWith("Z");
      String clean = value;
      if (isUtc) clean = value.substring(0, value.length() - 1);
      bool dateOnly = (clean.indexOf('T') == -1);
      curr.allDay = dateOnly;
      curr.start = parseICalDateToTime(clean, isUtc, dateOnly);
    } else if (key == "DTEND") {
      bool isUtc = value.endsWith("Z");
      String clean = value;
      if (isUtc) clean = value.substring(0, value.length() - 1);
      bool dateOnly = (clean.indexOf('T') == -1);
      curr.end = parseICalDateToTime(clean, isUtc, dateOnly);
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

void printEventsTFT() {
  int y = 80;
  int x = 80;
  tft.setTextColor(TFT_WHITE, sfondo_pageTask);
  tft.setTextSize(2);
  tft.setCursor(x, y);
  tft.println("=== Eventi ===");

  for (int i = 0; i < eventsCount; i++) {
    Event &e = events[i];

    tft.setCursor(x, y);
    tft.printf("--- Evento %d ---\n", i + 1);
    y += 20;

    tft.setCursor(x, y);
    tft.println(e.summary);
    y += 20;

    tft.setCursor(x, y);
    if (e.allDay) {
      struct tm *tm = localtime(&e.start);
      char buf[32];
      strftime(buf, sizeof(buf), "%Y-%m-%d (all-day)", tm);
      tft.print("DATA: ");
      tft.println(buf);
      y += 20;
    } else {
      struct tm *tm = localtime(&e.start);
      char buf[32];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", tm);
      tft.print("START: ");
      tft.println(buf);
      y += 20;
    }

    if (e.description.length()) {
      tft.setCursor(x, y);
      tft.print("DESC: ");
      tft.println(e.description);
      y += 20;
    }

    if (y > tft.height() - 20) {
      break;
    }
  }
}

void printTasksTFT() {
  int y = 80;
  int x = 80;
  tft.setTextSize(2);
  tft.setCursor(x, y);
  tft.setTextColor(TFT_WHITE);
  tft.println("=== To-Do List ===");

  int start = pageIndex * TASKS_PER_PAGE;
  time_t now = time(nullptr);

  for (int i = start; i < tasksCount && i < start + TASKS_PER_PAGE; i++) {
    Task &t = tasks[i];
    tft.setCursor(x, y);
    tft.setTextColor(t.done ? TFT_DARKGREY : TFT_WHITE);

    tft.print(t.title);
    if (t.due != 0 && t.due >= now) {
      struct tm *tmDue = localtime(&t.due);
      char buf[32];
      strftime(buf, sizeof(buf), " (%Y-%m-%d %H:%M)", tmDue);
      tft.print(buf);
    }
    y += 20;
  }
}

// ============================================================================
// SINCRONIZZAZIONE CODICE
// ============================================================================}

bool isNewerVersion(String currentVer, String newVer) {
  currentVer.replace("v", "");
  newVer.replace("v", "");
  
  int currMajor = 0, currMinor = 0;
  int newMajor = 0, newMinor = 0;

  int dot = currentVer.indexOf('.');
  if (dot > 0) {
    currMajor = currentVer.substring(0, dot).toInt();
    currMinor = currentVer.substring(dot + 1).toInt();
  }
  dot = newVer.indexOf('.');
  if (dot > 0) {
    newMajor = newVer.substring(0, dot).toInt();
    newMinor = newVer.substring(dot + 1).toInt();
  }

  if (newMajor > currMajor) return true;
  if (newMajor == currMajor && newMinor > currMinor) return true;
  return false;
}

void checkForUpdate() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  http.begin(VERSION_CHECK_URL);
  http.setTimeout(10000);
  int httpCode = http.GET();

  if (httpCode == 200) {
    DynamicJsonDocument doc(512);
    deserializeJson(doc, http.getString());
    availableVersion = doc["version"].as<String>();
    downloadURL = doc["download_url"].as<String>();
    updateAvailable = isNewerVersion(String(FIRMWARE_VERSION), availableVersion);
    
    if (updateAvailable) {
      Serial.println("Aggiornamento disponibile: " + availableVersion);
    }
  }
  http.end();
  lastVersionCheck = millis();
}

void performOTAUpdate() {
  if (!updateAvailable || downloadURL == "") return;

  Serial.println("Avvio aggiornamento...");

  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(downloadURL);
  http.setTimeout(30000);
  int httpCode = http.GET();

  if (httpCode == 200) {
    int contentLength = http.getSize();
    if (Update.begin(contentLength)) {
      WiFiClient* client = http.getStreamPtr();
      size_t written = 0;
      uint8_t buff[256];

      while (http.connected() && written < contentLength) {
        size_t available = client->available();
        if (available) {
          int bytesRead = client->readBytes(buff, min(available, sizeof(buff)));
          written += Update.write(buff, bytesRead);
          Serial.println("Progresso: " + String((written * 100) / contentLength) + "%");
        }
        delay(1);
      }

      if (Update.end(true)) {
        Serial.println("Aggiornamento completato! Riavvio...");
        delay(1000);
        ESP.restart();
      } else {
        Serial.println("Errore installazione: " + String(Update.errorString()));
      }
    }
  } else {
    Serial.println("Errore download: " + String(httpCode));
  }
  http.end();
}

#endif
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

void fetchAndParseICal() {
  WiFi.setSleep(false);            // disabilita il power saving WiFi — GRANDE impatto
  now = time(nullptr);
  WiFiClientSecure *client = new WiFiClientSecure;
  client->setInsecure();
  HTTPClient http;

  http.setTimeout(10000);          // era default 5s, ma aumenta la banda

  Serial.println("Scarico iCal...");
  if (http.begin(*client, ICAL_URL)) {
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

void parseICalStream(WiFiClient *stream) {
  eventsCount = 0;
  String lastLine = "";
  lastLine.reserve(512);
  bool inEvent = false;
  Event curr;

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
            if (lastLine.length() > 0) processLine(lastLine, inEvent, curr);
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
    processLine(lastLine, inEvent, curr);
  }

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

void processLine(String &line, bool &inEvent, Event &curr) {
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
    }
  } else if (key == "END") {
    if (value == "VEVENT") {
      inEvent = false;
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

// ============================================================================
// CALENDARIO GRAFICO - griglia mensile
// ============================================================================
static const int CAL_HDR_H = 38;
static const int CAL_DN_H  = 20;
static const int CAL_TOP   = CAL_HDR_H + CAL_DN_H; // 58
static const int CAL_ROWS  = 6;
static const int CAL_COLS  = 7;
static const int CAL_CW    = 480 / CAL_COLS; // 68
static const int CAL_CH    = (320 - CAL_TOP) / CAL_ROWS; // 43

int calGetStartWday(int month, int year) {
  struct tm t = {}; t.tm_year=year-1900; t.tm_mon=month-1; t.tm_mday=1; t.tm_isdst=-1;
  mktime(&t);
  return (t.tm_wday + 6) % 7; // 0=Lun .. 6=Dom
}

int calDaysInMonth(int month, int year) {
  struct tm t = {}; t.tm_year=year-1900; t.tm_mon=month; t.tm_mday=0; t.tm_isdst=-1;
  mktime(&t); return t.tm_mday;
}

int calGetEventsForDay(int day, int month, int year, int* outIdx, int maxOut) {
  struct tm ts = {}; ts.tm_year=year-1900; ts.tm_mon=month-1; ts.tm_mday=day; ts.tm_isdst=-1;
  time_t dS = mktime(&ts), dE = dS + 86400;
  int n = 0;
  for (int i = 0; i < eventsCount && n < maxOut; i++) {
    time_t es = events[i].start;
    time_t ee = (events[i].end!=0) ? events[i].end : es+(events[i].allDay?86400:3600);
    if (es < dE && ee > dS) outIdx[n++] = i;
  }
  return n;
}

void drawCalendarGrid(int month, int year) {
  tft.fillScreen(0x0821);
  // Header
  tft.fillRect(0, 0, 480, CAL_HDR_H, 0x1082);
  drawHouse(8, 6);
  const char* mN[] = {"GENNAIO","FEBBRAIO","MARZO","APRILE","MAGGIO","GIUGNO",
                      "LUGLIO","AGOSTO","SETTEMBRE","OTTOBRE","NOVEMBRE","DICEMBRE"};
  char hdr[32]; sprintf(hdr, "%s %d", mN[month-1], year);
  tft.setTextSize(2); tft.setTextColor(TFT_WHITE);
  tft.setCursor(240-(int)strlen(hdr)*6+20, 11);
  tft.print(hdr);
  // Nomi giorni
  tft.fillRect(0, CAL_HDR_H, 480, CAL_DN_H, 0x10A2);
  const char* dn[] = {"L","M","M","G","V","S","D"};
  for (int c=0; c<7; c++) {
    tft.setTextSize(1); tft.setTextColor(c>=5?0xF800:0x7BCF);
    tft.setCursor(c*CAL_CW + CAL_CW/2-3, CAL_HDR_H+6);
    tft.print(dn[c]);
  }
  int startW = calGetStartWday(month, year);
  int dim    = calDaysInMonth(month, year);
  getLocalTime(&timeinfo);
  int todD=timeinfo.tm_mday, todM=timeinfo.tm_mon+1, todY=timeinfo.tm_year+1900;
  int evtIdx[8];
  static const uint16_t EP[]={0xFFE0,0x07FF,0xFD20}; // giallo, ciano, arancione
  for (int row=0; row<CAL_ROWS; row++) {
    for (int col=0; col<CAL_COLS; col++) {
      int d  = row*CAL_COLS + col - startW + 1;
      int cx = col*CAL_CW, cy = CAL_TOP + row*CAL_CH;
      tft.drawRect(cx, cy, CAL_CW, CAL_CH, 0x2104);
      if (d<1 || d>dim) continue;
      bool isToday = (d==todD && month==todM && year==todY);
      int n = calGetEventsForDay(d, month, year, evtIdx, 8);
      uint16_t bg = isToday ? 0x0228 : (n>0 ? 0x1143 : 0x0841);
      tft.fillRect(cx+1, cy+1, CAL_CW-2, CAL_CH-2, bg);
      if (isToday) {
        tft.drawRect(cx, cy, CAL_CW, CAL_CH, TFT_CYAN);
        tft.drawRect(cx+1, cy+1, CAL_CW-2, CAL_CH-2, TFT_CYAN);
      }
      // Numero giorno
      tft.setTextSize(1);
      tft.setTextColor(isToday ? TFT_CYAN : (col>=5 ? 0xFB00 : TFT_WHITE));
      char ds[3]; sprintf(ds,"%d",d);
      tft.setCursor(cx+3, cy+3); tft.print(ds);
      // Labels eventi (max 3)
      for (int e=0; e<3 && e<n; e++) {
        String s = events[evtIdx[e]].summary; s.trim();
        if (s.length()>7) s=s.substring(0,7);
        tft.setTextSize(1); tft.setTextColor(EP[e]);
        tft.setCursor(cx+2, cy+14+e*10); tft.print(s);
      }
    }
  }
}

// Restituisce il giorno (1-31) toccato, oppure -1
int calGetTouchedDay(uint16_t tx, uint16_t ty, int month, int year) {
  int gy = 320 - (int)ty; // Y invertita
  if (gy < CAL_TOP || gy >= CAL_TOP + CAL_ROWS*CAL_CH) return -1;
  int row = (gy - CAL_TOP) / CAL_CH;
  int col = (int)tx / CAL_CW;
  if (col<0 || col>=CAL_COLS) return -1;
  int d = row*CAL_COLS + col - calGetStartWday(month,year) + 1;
  int dim = calDaysInMonth(month, year);
  if (d<1 || d>dim) return -1;
  return d;
}

void drawDayDetail(int day, int month, int year) {
  tft.fillScreen(0x0821);
  // Header
  tft.fillRect(0, 0, 480, CAL_HDR_H, 0x1082);
  const char* mN[] = {"Gen","Feb","Mar","Apr","Mag","Giu","Lug","Ago","Set","Ott","Nov","Dic"};
  char title[32]; sprintf(title, "%d %s %d", day, mN[month-1], year);
  tft.setTextSize(2); tft.setTextColor(TFT_WHITE);
  tft.setCursor(60, 11); tft.print(title);
  // Bottone "< Cal." in alto a DESTRA (touch x>380 && y>280)
  tft.fillRoundRect(410, 5, 66, 28, 6, 0x3186);
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
  tft.setCursor(416, 15); tft.print("< Cal.");
  // Lista eventi
  int evtIdx[MAX_EVENTS];
  int n = calGetEventsForDay(day, month, year, evtIdx, MAX_EVENTS);
  if (n==0) {
    tft.setTextColor(0x7BCF); tft.setTextSize(2);
    tft.setCursor(140, 160); tft.print("Nessun evento"); return;
  }
  int y=44;
  for (int i=0; i<n && y<310; i++) {
    Event& e = events[evtIdx[i]];
    bool hasDsc = e.description.length()>0;
    int cH = hasDsc ? 60 : 44;
    tft.fillRoundRect(6, y, 468, cH, 6, 0x1143);
    tft.drawRoundRect(6, y, 468, cH, 6, TFT_CYAN);
    tft.setTextSize(1);
    if (e.allDay) {
      tft.setTextColor(TFT_LIGHTGREY); tft.setCursor(12,y+5); tft.print("Tutto il giorno");
    } else {
      struct tm* st=localtime(&e.start); char tb[12]; strftime(tb,sizeof(tb),"%H:%M",st);
      String ts2=String(tb);
      if (e.end!=0) { struct tm* et=localtime(&e.end); char tb2[12]; strftime(tb2,sizeof(tb2),"-%H:%M",et); ts2+=String(tb2); }
      tft.setTextColor(TFT_CYAN); tft.setCursor(12,y+5); tft.print(ts2);
    }
    tft.setTextSize(1); tft.setTextColor(TFT_YELLOW);
    String summ=e.summary; if(summ.length()>60) summ=summ.substring(0,57)+"...";
    tft.setCursor(12,y+18); tft.print(summ);
    if (hasDsc) {
      tft.setTextColor(0x8C71);
      String desc=e.description; if(desc.length()>60) desc=desc.substring(0,57)+"...";
      tft.setCursor(12,y+32); tft.print(desc);
    }
    y += cH+4;
  }
}

// Legacy stub (non più usata, rimane per compatibilità)
void printEventsTFT() {
  int y = 70; // Spostato a 70 per non sovrascrivere il titolo "Pagina 1"
  int x = 10;
  int cardWidth = 460;
  
  if (eventsCount == 0) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setCursor(x + 10, y + 20);
    tft.println("Nessun evento in programma.");
    return;
  }

  // Costruisci layout: quante card per pagina? circa 3-4, proviamo a calcolarlo
  // o semplicemente cicliamo finchè c'è spazio, MA partendo dall'indice giusto
  int eventiStart = 0;
  
  // un semplice sistema: ogni evento occupa circa 80-110px. 
  // in 240 px ci stanno 2/3 eventi.
  // Salteremo gli eventi già mostrati
  int skipped = 0;
  int shownOnThisPage = 0;
  bool moreEvents = false;

  for (int i = 0; i < eventsCount; i++) {
    Event &e = events[i];
    
    int cardHeight = 70;
    if (e.description.length() > 0) {
      cardHeight += 30;
    }

    // Se l'evento corrente appartiene a una pagina precedente, saltiamolo
    // Ma come sappiamo quanti eventi c'erano nelle pagine prima?
    // Un modo fisso è "3 eventi per pagina"
    if (i < eventiPageIndex * 3) {
      continue;
    }

    // Se usciamo dallo schermo con questo evento
    if (y + cardHeight > tft.height() - 40) { // Lasciamo 40px per il tasto "Altri eventi"
      moreEvents = true; // ci sono altri eventi da mostrare nella prossima pagina
      break;
    }

    // Disegna la card sfondo (blu scuro trasparente/arrotondato)
    tft.fillRoundRect(x, y, cardWidth, cardHeight, 10, 0x18E3); 
    tft.drawRoundRect(x, y, cardWidth, cardHeight, 10, TFT_CYAN);

    // Titolo evento
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(x + 15, y + 15);
    String summaryStr = e.summary;
    if (summaryStr.length() > 30) {
      summaryStr = summaryStr.substring(0, 27) + "...";
    }
    tft.println(summaryStr);

    // Data/Ora dell'evento
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(x + 15, y + 40);
    
    if (e.allDay) {
      struct tm *tm = localtime(&e.start);
      char buf[64];
      strftime(buf, sizeof(buf), "%d/%m/%Y - Tutto il giorno", tm);
      tft.print("\x0F "); 
      tft.println(buf);
    } else {
      struct tm *tm = localtime(&e.start);
      char buf[64];
      strftime(buf, sizeof(buf), "%d/%m/%Y alle %H:%M", tm);
      tft.print("\x09 "); 
      tft.println(buf);
    }

    // Descrizione 
    if (e.description.length() > 0) {
      tft.setTextColor(TFT_LIGHTGREY);
      tft.setCursor(x + 15, y + 60);
      String descStr = e.description;
      if (descStr.length() > 60) {
        descStr = descStr.substring(0, 57) + "...";
      }
      tft.println(descStr);
    }

    y += cardHeight + 15; 
    shownOnThisPage++;
  }

  // Disegna bottone "Altri eventi" o "Torna all'inizio"
  if (eventsCount > 3) {
    tft.fillRoundRect(150, 275, 180, 40, 10, TFT_BLUE);
    tft.drawRoundRect(150, 275, 180, 40, 10, TFT_WHITE);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    
    if (moreEvents) {
      tft.setCursor(165, 287);
      tft.print("Altri Eventi");
    } else {
      tft.setCursor(160, 287);
      tft.print("Torna all'inizio");
      // Resetta per il prossimo click
      eventiPageIndex = -1; // -1 diventerà 0 al prossimo click nel loop
    }
  }
}

void printTasksTFT() {
  tft.fillScreen(0x0821);

  // Header
  tft.fillRect(0, 0, 480, 38, 0x1082);
  drawHouse(8, 6);
  tft.setTextSize(2); tft.setTextColor(TFT_WHITE);
  tft.setCursor(60, 11); tft.print("TO-DO LIST");

  // Contatore completati
  int done = 0;
  for (int i = 0; i < tasksCount; i++) if (tasks[i].done) done++;
  tft.setTextSize(1); tft.setTextColor(0x07E0);
  char prog[24]; sprintf(prog, "%d/%d completati", done, tasksCount);
  tft.setCursor(350, 16); tft.print(prog);

  if (tasksCount == 0) {
    tft.setTextColor(0x7BCF); tft.setTextSize(2);
    tft.setCursor(120, 160); tft.print("Nessun task presente");
    return;
  }

  int y = 44;
  int start = pageIndex * TASKS_PER_PAGE;
  for (int i = start; i < tasksCount && i < start + TASKS_PER_PAGE && y < 310; i++) {
    Task& t = tasks[i];
    int cH = 44;

    // Card
    uint16_t cardBg = t.done ? 0x0C23 : 0x1143;
    tft.fillRoundRect(6, y, 468, cH, 6, cardBg);
    tft.drawRoundRect(6, y, 468, cH, 6, t.done ? 0x07E0 : TFT_CYAN);

    // Checkbox (sinistra)
    if (t.done) {
      // Segno di spunta verde
      tft.fillRoundRect(14, y+12, 18, 18, 3, 0x07E0);
      tft.setTextSize(1); tft.setTextColor(TFT_BLACK);
      tft.setCursor(18, y+16); tft.print("OK");
    } else {
      // Quadratino vuoto
      tft.drawRoundRect(14, y+12, 18, 18, 3, TFT_WHITE);
    }

    // Titolo task
    tft.setTextSize(1);
    tft.setTextColor(t.done ? 0x7BCF : TFT_WHITE);
    String title = t.title;
    if (title.length() > 52) title = title.substring(0, 49) + "...";
    tft.setCursor(40, y + 10); tft.print(title);

    // Data scadenza (se presente e futura)
    time_t nowT = time(nullptr);
    if (t.due != 0) {
      struct tm* td = localtime(&t.due);
      char buf[32]; strftime(buf, sizeof(buf), "Scade: %d/%m/%Y", td);
      tft.setTextSize(1);
      tft.setTextColor(t.due < nowT ? 0xF800 : 0xFD20); // rosso se scaduto, arancione altrimenti
      tft.setCursor(40, y + 26); tft.print(buf);
    }

    y += cH + 4;
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
      WiFiClient *client = http.getStreamPtr();
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


// ============================================================================
// METEO - Open-Meteo API (Moncalieri, TO)
// ============================================================================

struct WeatherDay {
  String label;  // "Oggi", "Dom", "Lun" ecc.
  int tempMax;   // °C arrotondato
  int tempMin;
  int weatherCode;  // WMO weather code
  int precProb;     // % probabilità precip.
  int humidity;     // % umidità max
};

// Restituisce una stringa-etichetta corta basata sul WMO weather code
const char *weatherCodeToLabel(int code) {
  if (code == 0) return "Sole";
  if (code <= 2) return "Parz.nuv";
  if (code == 3) return "Nuvoloso";
  if (code <= 49) return "Nebbia";
  if (code <= 59) return "Piogger.";
  if (code <= 69) return "Pioggia";
  if (code <= 79) return "Neve";
  if (code <= 84) return "Rovesci";
  if (code <= 86) return "Neve fort";
  if (code <= 99) return "Temporale";
  return "N/A";
}

// Disegna una piccola icona meteo TFT centrata in (cx, cy)
void drawWeatherTFTIcon(int cx, int cy, int code) {
  if (code == 0) {
    // Sole pieno
    tft.fillCircle(cx, cy, 14, TFT_YELLOW);
    for (int i = 0; i < 8; i++) {
      float a = i * 45.0 * PI / 180.0;
      tft.drawLine(cx + 17 * cos(a), cy + 17 * sin(a), cx + 22 * cos(a), cy + 22 * sin(a), TFT_YELLOW);
    }
  } else if (code <= 2) {
    // Sole con nuvola
    tft.fillCircle(cx - 6, cy - 6, 10, TFT_YELLOW);
    tft.fillCircle(cx - 2, cy + 4, 8, TFT_WHITE);
    tft.fillCircle(cx + 8, cy + 2, 10, TFT_WHITE);
    tft.fillRect(cx - 2, cy + 4, 18, 8, TFT_WHITE);
  } else if (code <= 3) {
    // Nuvole
    tft.fillCircle(cx - 8, cy, 9, TFT_LIGHTGREY);
    tft.fillCircle(cx + 2, cy - 5, 11, TFT_LIGHTGREY);
    tft.fillCircle(cx + 12, cy, 9, TFT_LIGHTGREY);
    tft.fillRect(cx - 8, cy, 22, 10, TFT_LIGHTGREY);
  } else if (code <= 49) {
    // Nebbia
    tft.drawFastHLine(cx - 10, cy - 4, 20, TFT_LIGHTGREY);
    tft.drawFastHLine(cx - 14, cy, 28, TFT_LIGHTGREY);
    tft.drawFastHLine(cx - 10, cy + 4, 20, TFT_LIGHTGREY);
  } else if (code <= 69 || (code >= 80 && code <= 84)) {
    // Nuvola con pioggia (include Rovesci che sono 80-84)
    tft.fillCircle(cx - 6, cy - 4, 9, TFT_LIGHTGREY);
    tft.fillCircle(cx + 6, cy - 4, 9, TFT_LIGHTGREY);
    tft.fillRect(cx - 6, cy - 4, 14, 8, TFT_LIGHTGREY);
    for (int i = 0; i < 3; i++) {
      tft.drawLine(cx - 6 + i * 6, cy + 7, cx - 9 + i * 6, cy + 16, TFT_CYAN);
    }
  } else if (code <= 79 || (code >= 85 && code <= 86)) {
    // Neve
    tft.fillCircle(cx - 6, cy - 4, 9, TFT_LIGHTGREY);
    tft.fillCircle(cx + 6, cy - 4, 9, TFT_LIGHTGREY);
    tft.fillRect(cx - 6, cy - 4, 14, 8, TFT_LIGHTGREY);
    for (int i = 0; i < 3; i++) {
      tft.fillCircle(cx - 6 + i * 6, cy + 13, 3, TFT_WHITE);
    }
  } else {
    // Temporale
    tft.fillCircle(cx - 6, cy - 4, 9, 0x632C);
    tft.fillCircle(cx + 6, cy - 4, 9, 0x632C);
    tft.fillRect(cx - 6, cy - 4, 14, 8, 0x632C);
    // Fulmine
    tft.fillTriangle(cx + 2, cy + 6, cx - 4, cy + 14, cx + 1, cy + 14, TFT_YELLOW);
    tft.fillTriangle(cx - 1, cy + 12, cx + 5, cy + 20, cx - 4, cy + 20, TFT_YELLOW);
  }
}

// Scarica previsioni 4 giorni da Open-Meteo e le mette in days[0..3]
String weatherLastError = "";

bool fetchWeather(WeatherDay days[4]) {
  if (WiFi.status() != WL_CONNECTED) {
    weatherLastError = "WiFi non connesso";
    Serial.println("[METEO] " + weatherLastError);
    return false;
  }

  HTTPClient http;

  // HTTP plain: Open-Meteo supporta HTTP e non causa problemi di memoria SSL
  http.begin("http://api.open-meteo.com/v1/forecast"
             "?latitude=45.0070&longitude=7.6693"
             "&daily=temperature_2m_max,temperature_2m_min,weather_code,precipitation_probability_max,relative_humidity_2m_max"
             "&timezone=Europe%2FRome&forecast_days=4");
  http.setTimeout(15000);
  // getString() decomprime automaticamente gzip (getStream() no)
  int code = http.GET();

  if (code != 200) {
    weatherLastError = "HTTP: " + String(code);
    Serial.println("[METEO] " + weatherLastError);
    http.end();
    return false;
  }

  String body = http.getString();
  http.end();

  Serial.println("[METEO] Body len=" + String(body.length()));

  DynamicJsonDocument doc(4096);
  DeserializationError err = deserializeJson(doc, body);

  if (err) {
    weatherLastError = String("JSON: ") + err.c_str();
    Serial.println("[METEO] " + weatherLastError);
    return false;
  }
  weatherLastError = "";


  JsonArray tMax = doc["daily"]["temperature_2m_max"];
  JsonArray tMin = doc["daily"]["temperature_2m_min"];
  JsonArray codes = doc["daily"]["weather_code"];  // campo aggiornato API v2
  JsonArray precs = doc["daily"]["precipitation_probability_max"];
  JsonArray hums  = doc["daily"]["relative_humidity_2m_max"];
  JsonArray dates = doc["daily"]["time"];

  // Nomi giorni brevi in italiano
  const char *giorniBrevi[] = { "Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab" };

  for (int i = 0; i < 4; i++) {
    days[i].tempMax = (int)round((float)tMax[i]);
    days[i].tempMin = (int)round((float)tMin[i]);
    days[i].weatherCode = (int)codes[i];
    days[i].precProb = (int)precs[i];
    days[i].humidity = (int)hums[i];

    if (i == 0) {
      days[i].label = "Oggi";
    } else {
      // Ricava il giorno della settimana dalla stringa "YYYY-MM-DD"
      String dateStr = dates[i].as<String>();  // es. "2025-02-24"
      struct tm t = {};
      t.tm_year = dateStr.substring(0, 4).toInt() - 1900;
      t.tm_mon = dateStr.substring(5, 7).toInt() - 1;
      t.tm_mday = dateStr.substring(8, 10).toInt();
      mktime(&t);
      days[i].label = String(giorniBrevi[t.tm_wday]);
    }
  }
  Serial.println("[METEO] Dati scaricati OK. Previsioni per i prossimi giorni:");
  for (int i = 0; i < 4; i++) {
    Serial.printf("  %s - %s: Max %d C, Min %d C, Umidita' %d%%, Pioggia %d%%\n",
                  days[i].label.c_str(), weatherCodeToLabel(days[i].weatherCode),
                  days[i].tempMax, days[i].tempMin, days[i].humidity, days[i].precProb);
  }
  return true;
}

// Legge dati aggiornati dal sensore KY-001 (DS18B20)
void readSensor() {
  Serial.print("[DS18B20] Lettura... ");
  sensors.requestTemperatures(); 
  float t = sensors.getTempCByIndex(0);
  
  if (t == DEVICE_DISCONNECTED_C) {
    Serial.println("Errore (controlla i cavi o il pin!)");
  } else {
    Serial.printf("Temp: %.1f C\n", t);
    roomTemp = t;
    roomHum = 0.0f; // KY-001 non misura l'umidità
  }
}

// Disegna la schermata meteo completa (Oggi orizzontale, prossimi 3 in verticale)
void drawWeatherPage() {
  // 1. Legge il sensore prima di disegnare
  readSensor();

  // Sfondo sfumato blu notte
  for (int y = 0; y < 320; y++) {
    uint8_t r = map(y, 0, 319, 0, 2);
    uint8_t g = map(y, 0, 319, 5, 10);
    uint8_t b = map(y, 0, 319, 20, 12);
    tft.drawFastHLine(0, y, 480, tft.color565(r * 8, g * 8, b * 8));
  }

  // Casetta per tornare alla home
  drawHouse(10, 10);

  // Titolo della pagina
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(60, 10);
  tft.println("METEO");

  // Riquadro dati KY-015 (Stanza) in linea
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(200, 16);
  tft.print("Stanza: ");
  tft.setTextColor(TFT_YELLOW);
  if (roomTemp == 0.0f && roomHum == 0.0f) {
    tft.print("-- c  ");
  } else {
    tft.print((int)round(roomTemp)); tft.print(" c  ");
  }
  tft.setTextColor(TFT_CYAN);
  if (roomTemp == 0.0f && roomHum == 0.0f) {
    tft.print("--%");
  } else {
    tft.print("--%"); // KY-001 non misura l'umidità
  }

  // Linea separatrice
  tft.drawFastHLine(0, 42, 480, TFT_LIGHTGREY);

  // Scarica dati
  WeatherDay days[4];
  bool ok = fetchWeather(days);

  if (!ok) {
    tft.setTextColor(TFT_RED);
    tft.setTextSize(2);
    tft.setCursor(60, 140);
    tft.println("Errore caricamento meteo");
    tft.setCursor(60, 165);
    tft.setTextColor(TFT_YELLOW);
    tft.println(weatherLastError);  // Dettaglio: WiFi, HTTP:xxx, JSON:xxx
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setTextSize(1);
    tft.setCursor(60, 195);
    tft.println("Premi la casetta per tornare");
    return;
  }

  // ==== OGGI (Orizzontale) ====
  tft.fillRoundRect(10, 48, 460, 90, 10, 0x1A7F);
  
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(20, 56);
  tft.println(days[0].label);

  drawWeatherTFTIcon(60, 95, days[0].weatherCode);

  tft.setTextSize(1);
  tft.setTextColor(TFT_LIGHTGREY);
  const char *lbl0 = weatherCodeToLabel(days[0].weatherCode);
  int lbl0W = strlen(lbl0) * 6;
  tft.setCursor(60 - lbl0W / 2, 120);
  tft.println(lbl0);

  tft.setTextSize(4);
  tft.setTextColor(TFT_ORANGE);
  char bufMax0[8]; sprintf(bufMax0, "%d", days[0].tempMax);
  tft.setCursor(120, 75);
  tft.print(bufMax0);
  tft.setTextSize(2); tft.print(" max");

  tft.setTextSize(4);
  tft.setTextColor(TFT_CYAN);
  char bufMin0[8]; sprintf(bufMin0, "%d", days[0].tempMin);
  tft.setCursor(250, 75);
  tft.print(bufMin0);
  tft.setTextSize(2); tft.setTextColor(TFT_LIGHTGREY); tft.print(" min");

  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN);
  tft.setCursor(370, 65);
  tft.print("U: "); tft.print(days[0].humidity); tft.print("%");
  tft.setTextColor(TFT_CYAN); // Azzurro differente per la pioggia
  tft.setCursor(370, 95);
  tft.print("P: "); tft.print(days[0].precProb); tft.print("%");

  // ==== PROSSIMI 3 GIORNI (Verticale) ====
  int colW = 146;
  int space = 8;
  for (int i = 1; i < 4; i++) {
    int startX = 10 + (i - 1) * (colW + space);
    int cx = startX + colW / 2;
    int baseY = 145;

    tft.fillRoundRect(startX, baseY, colW, 165, 10, 0x0C3F);

    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    int labelW = days[i].label.length() * 12;
    tft.setCursor(cx - labelW / 2, baseY + 8);
    tft.println(days[i].label);

    drawWeatherTFTIcon(cx, baseY + 45, days[i].weatherCode);

    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY);
    const char *lbl = weatherCodeToLabel(days[i].weatherCode);
    int lblW = strlen(lbl) * 6;
    tft.setCursor(cx - lblW / 2, baseY + 66);
    tft.println(lbl);

    tft.setTextSize(2);
    tft.setTextColor(TFT_ORANGE);
    char tmp[16];
    sprintf(tmp, "%d", days[i].tempMax);
    int w1 = strlen(tmp) * 12;
    sprintf(tmp, "%d", days[i].tempMin);
    int w2 = strlen(tmp) * 12;
    tft.setCursor(cx - (w1 + 12 + w2) / 2, baseY + 85);
    tft.print(days[i].tempMax);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.print("/");
    tft.setTextColor(TFT_CYAN);
    tft.print(days[i].tempMin);

    tft.setTextSize(2);
    tft.setTextColor(TFT_CYAN);
    tft.setCursor(cx - 36, baseY + 115);
    tft.print("U: "); tft.print(days[i].humidity); tft.print("%");

    tft.setTextColor(TFT_CYAN);
    tft.setCursor(cx - 36, baseY + 140);
    tft.print("P: "); tft.print(days[i].precProb); tft.print("%");
  }
}

#endif

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
  } else if (code <= 69) {
    // Nuvola con pioggia
    tft.fillCircle(cx - 6, cy - 4, 9, TFT_LIGHTGREY);
    tft.fillCircle(cx + 6, cy - 4, 9, TFT_LIGHTGREY);
    tft.fillRect(cx - 6, cy - 4, 14, 8, TFT_LIGHTGREY);
    for (int i = 0; i < 3; i++) {
      tft.drawLine(cx - 6 + i * 6, cy + 7, cx - 9 + i * 6, cy + 16, TFT_CYAN);
    }
  } else if (code <= 86) {
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
             "?latitude=44.9978&longitude=7.6881"
             "&daily=temperature_2m_max,temperature_2m_min,weather_code"
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
  JsonArray dates = doc["daily"]["time"];

  // Nomi giorni brevi in italiano
  const char *giorniBrevi[] = { "Dom", "Lun", "Mar", "Mer", "Gio", "Ven", "Sab" };

  for (int i = 0; i < 4; i++) {
    days[i].tempMax = (int)round((float)tMax[i]);
    days[i].tempMin = (int)round((float)tMin[i]);
    days[i].weatherCode = (int)codes[i];

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
  Serial.println("[METEO] Dati scaricati OK");
  return true;
}

// Disegna la schermata meteo completa (4 colonne)
void drawWeatherPage() {
  // Sfondo sfumato blu notte
  for (int y = 0; y < 320; y++) {
    uint8_t r = map(y, 0, 319, 0, 2);
    uint8_t g = map(y, 0, 319, 5, 10);
    uint8_t b = map(y, 0, 319, 20, 12);
    tft.drawFastHLine(0, y, 480, tft.color565(r * 8, g * 8, b * 8));
  }

  // Titolo
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(3);
  tft.setCursor(200, 20);
  tft.println("METEO");

  // Casetta per tornare alla home
  drawHouse();

  // Linea separatrice
  tft.drawFastHLine(0, 55, 480, TFT_LIGHTGREY);

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

  // 4 colonne di larghezza 120px ciascuna
  int colW = 120;
  for (int i = 0; i < 4; i++) {
    int cx = i * colW + colW / 2;  // centro della colonna
    int baseY = 65;

    // Sfondo cella (arrotondato, leggermente più chiaro)
    uint16_t cardColor = (i == 0) ? 0x1A7F : 0x0C3F;
    tft.fillRoundRect(i * colW + 5, baseY, colW - 10, 240, 10, cardColor);

    // Etichetta giorno
    tft.setTextSize(2);
    tft.setTextColor(i == 0 ? TFT_YELLOW : TFT_WHITE);
    int labelW = days[i].label.length() * 12;
    tft.setCursor(cx - labelW / 2, baseY + 8);
    tft.println(days[i].label);

    // Icona meteo
    drawWeatherTFTIcon(cx, baseY + 65, days[i].weatherCode);

    // Condizione testuale
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY);
    const char *lbl = weatherCodeToLabel(days[i].weatherCode);
    int lblW = strlen(lbl) * 6;
    tft.setCursor(cx - lblW / 2, baseY + 100);
    tft.println(lbl);

    // Temp MAX
    tft.setTextSize(3);
    tft.setTextColor(TFT_ORANGE);
    char bufMax[8];
    sprintf(bufMax, "%d", days[i].tempMax);
    int maxW = strlen(bufMax) * 18;
    tft.setCursor(cx - maxW / 2, baseY + 120);
    tft.println(bufMax);
    tft.setTextSize(1);
    tft.setCursor(cx + maxW / 2, baseY + 122);
    tft.println("max");

    // Temp MIN
    tft.setTextSize(3);
    tft.setTextColor(TFT_CYAN);
    char bufMin[8];
    sprintf(bufMin, "%d", days[i].tempMin);
    int minW = strlen(bufMin) * 18;
    tft.setCursor(cx - minW / 2, baseY + 160);
    tft.println(bufMin);
    tft.setTextSize(1);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setCursor(cx + minW / 2, baseY + 162);
    tft.println("min");

    // Gradi "°C" sotto
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(cx - 14, baseY + 195);
    tft.println("C");
  }
}

#endif

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
  int y = 50; // Inizia un po' più in alto dato che non c'è più il titolo
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

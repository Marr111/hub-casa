#ifndef WEATHER_H
#define WEATHER_H

#include "config.h"

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

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  
  http.end();

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
    sensorReady = false;  // Lettura non valida
  } else {
    Serial.printf("Temp: %.1f C\n", t);
    roomTemp = t;
    sensorReady = true;   // Lettura valida: anche 0°C è un dato reale
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

  // Riquadro dati KY-001 (Stanza)
  // FIX: usare sensorReady invece di roomTemp==0.0f per evitare il bug a 0°C in inverno
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(200, 16);
  tft.print("Stanza: ");
  tft.setTextColor(TFT_YELLOW);
  if (!sensorReady) {
    tft.print("-- c  ");
  } else {
    tft.print((int)round(roomTemp)); tft.print(" c  ");
  }
  // KY-001 non misura l'umidità: mostriamo sempre "--%" (il ramo else era inutile)
  tft.setTextColor(TFT_CYAN);
  tft.print("--%");

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

#endif // WEATHER_H

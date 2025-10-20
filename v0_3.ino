#include "FS.h"
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

TFT_eSPI tft = TFT_eSPI();

// File di calibrazione touch
#define CALIBRATION_FILE "/TouchCalData2"
#define REPEAT_CAL false

// Definizioni display
#define ST7796_DRIVER
#define TFT_WIDTH 320
#define TFT_HEIGHT 480

#define COLOR_ARROW TFT_WHITE

#define sfondo_page0 TFT_BLUE
#define sfondo_page1 TFT_YELLOW
#define sfondo_page2 TFT_BLUE
#define sfondo_page3 TFT_RED
#define sfondo_page4 TFT_GREEN
#define sfondo_page5 TFT_YELLOW
#define sfondo_page6 TFT_CYAN
#define sfondo_page7 TFT_MAGENTA
#define sfondo_pageImpostazioni TFT_DARKGREY

const char *ssid = "A25 di Matteo";
const char *password = "matteo123";
const char *ntpServer = "pool.ntp.org";

const char *ICAL_URL = "https://calendar.google.com/calendar/ical/casettamatteo1%40gmail.com/public/basic.ics";
#define MAX_EVENTS 3
int eventsCount = 0;
time_t now;
#define MAX_TASKS 10
int tasksCount = 0;
int pageIndex = 0;
#define TASKS_PER_PAGE 10

const long gmtOffset_sec = 3600;      // Italia GMT+1
const int daylightOffset_sec = 3600;  // Ora legale
String dataLunga;
String ora;

unsigned long lastActivity = 0;       // ultimo tocco registrato
const unsigned long timeout = 30000;  // secondi per 1000

int page = 0;

int stato_scroll_bar = 1;

int esci_dal_loop=1;

struct tm timeinfo;

struct TouchPoint {
  uint16_t x;
  uint16_t y;
  bool touched;
};

struct Event {
  String summary;
  String description;
  time_t start = 0;
  time_t end = 0;
  bool allDay = false;
};

Event events[MAX_EVENTS];

struct Task {
  String title;
  bool done;
  time_t due;  // opzionale: scadenza
};

Task tasks[MAX_TASKS];

TouchPoint touch_coordinate() {
  TouchPoint p = { 0, 0, false };  // inizializza x,y,touched

  if (tft.getTouch(&p.x, &p.y)) {
    lastActivity = millis();  // aggiorna ultimo tocco
    p.touched = true;
  }

  return p;
}

String getDateLong() {
  if (!getLocalTime(&timeinfo)) return "N/A";

  // Giorni della settimana
  const char *daysOfWeek[] = { "domenica", "lunedi", "martedi", "mercoledi", "giovedi", "venerdi", "sabato" };
  // Mesi
  const char *months[] = { "gen", "feb", "mar", "apr", "mag", "giu",
                           "lug", "ago", "set", "ott", "nov", "dic" };

  char buffer[40];
  sprintf(buffer, "%s %02d %s %04d",
          daysOfWeek[timeinfo.tm_wday],  // giorno della settimana
          timeinfo.tm_mday,              // giorno del mese
          months[timeinfo.tm_mon],       // mese
          timeinfo.tm_year + 1900);      // anno
  return String(buffer);
}

String getTime() {
  if (!getLocalTime(&timeinfo)) return "N/A";

  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  return String(buffer);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("✨ Inizializzazione ESP32-S3... ✨");

  // Inizializza display
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  // Accendi backlight
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Inizializza touch
  pinMode(TOUCH_CS, OUTPUT);
  digitalWrite(TOUCH_CS, HIGH);

  // Messaggio sul display
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(20, 20);
  tft.println("Stiamo preparando");
  tft.println(" il tuo dispositivo");

  connessioneWiFi();
  connessioneNTP();

  // Imposta timezone (es. Europe/Rome). Modifica se serve.
  // Formato: "CET-1CEST,M3.5.0/02:00,M10.5.0/03:00"
  setenv("TZ", "CET-1CEST,M3.5.0/02:00,M10.5.0/03:00", 1);
  tzset();
  //fetchAndParseICal();
  //printEvents();

  Serial.println("🚀 Setup completato!");
}

void loop() {

  page0();

  TouchPoint tp = touch_coordinate();
  if (tp.touched) {
    Serial.print("Uso coordinate: ");
    Serial.print(tp.x);
    Serial.print(", ");
    Serial.println(tp.y);
  }
  delay(500);
}

void page0() {
  uint16_t x, y;
  page = 0;

  tft.fillScreen(sfondo_page0);  //grafica

  drawWiFiSymbol(400, 20);
  drawGearIcon(445, 25);

  tft.setTextColor(TFT_WHITE, sfondo_page0);
  tft.setTextSize(5);
  tft.setCursor(60, 50);
  tft.println("Casa Casetta");
  tft.setTextSize(2);
  tft.setCursor(120, 300);
  tft.println("Premi per proseguire");

  while (!tft.getTouch(&x, &y)) {
    dataLunga = getDateLong();
    ora = getTime();

    tft.setTextSize(4);
    tft.setCursor(130, 130);
    tft.println(ora);
    tft.setTextSize(3);
    tft.setCursor(75, 200);
    tft.println(dataLunga);
    if (tft.getTouch(&x, &y)) {
      if (x > 440 && y > 280) {
        pageImpostazioni();
        Serial.print("Pagina Impostazioni");
      } else {
        Serial.print("Pagina 1");
        page1();
      }
    }
  }
}

void page1() {
  tft.fillScreen(sfondo_page1);
  tft.setTextColor(TFT_WHITE, sfondo_page1);
  tft.setCursor(130, 30);
  tft.setTextSize(4);
  tft.println("Pagina 1");
  drawArrows();
  while (1) {
    cambio_pagina();
    checkInactivity();
  }
}

void page2() {
  tft.fillScreen(sfondo_page2);
  tft.setTextColor(TFT_WHITE, sfondo_page2);
  printEventsTFT();
  tft.setCursor(130, 30);
  tft.setTextSize(4);
  tft.println("Pagina 2");
  drawArrows();
  while (1) {
    cambio_pagina();
    checkInactivity();
  }
}

void page3() {
  tft.fillScreen(sfondo_page3);
  tft.setTextColor(TFT_WHITE, sfondo_page3);
  printTasksTFT();
  tft.setCursor(130, 30);
  tft.setTextSize(4);
  tft.println("Pagina 3");
  drawArrows();
  while (1) {
    cambio_pagina();
    checkInactivity();
  }
}

void page4() {
  tft.fillScreen(sfondo_page4);
  tft.setTextColor(TFT_WHITE, sfondo_page4);
  tft.setCursor(130, 30);
  tft.setTextSize(4);
  tft.println("Pagina 4");
  drawArrows();
  while (1) {
    cambio_pagina();
    checkInactivity();
  }
}

void page5() {
  tft.fillScreen(sfondo_page5);
  tft.setTextColor(TFT_WHITE, sfondo_page5);
  tft.setCursor(130, 30);
  tft.setTextSize(4);
  tft.println("Pagina 5");
  drawArrows();
  while (1) {
    cambio_pagina();
    checkInactivity();
  }
}

void page6() {
  tft.fillScreen(sfondo_page6);
  tft.setCursor(130, 30);
  tft.setTextColor(TFT_WHITE, sfondo_page6);
  tft.setTextSize(4);
  tft.println("Pagina 6");
  drawArrows();
  while (1) {
    cambio_pagina();
    checkInactivity();
  }
}

void page7() {
  tft.fillScreen(sfondo_page7);
  tft.setCursor(130, 30);
  tft.setTextColor(TFT_WHITE, sfondo_page7);
  tft.setTextSize(4);
  tft.println("Pagina 7");
  drawArrows();
  while (1) {
    cambio_pagina();
    checkInactivity();
  }
}

void pageImpostazioni() {
  tft.fillScreen(sfondo_pageImpostazioni);
  tft.setCursor(100, 30);
  tft.setTextColor(TFT_WHITE, sfondo_pageImpostazioni);
  tft.setTextSize(4);
  tft.println("IMPOSTAZIONI");
  drawScrollBar(450, 20, 300, 2);
  drawHouse();

  for (int y = 70; y < 320; y = y + 50) {
    tft.fillRoundRect(15, y, 420, 40, 15, TFT_LIGHTGREY);
  }

  while (esci_dal_loop==1) {  // finché non cambia pagina
    if (stato_scroll_bar == 1) stato_scroll_bar1();
    else if (stato_scroll_bar == 2) stato_scroll_bar2();
    else if (stato_scroll_bar == 3) stato_scroll_bar3();
    delay(100);
  }
  esci_dal_loop=1;
}


void cambio_pagina() {
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    lastActivity = millis();  // aggiorna ultimo tocco

    if (page > 1) {
      if (x > 20 && x < 60 && y > tft.height() / 2 - 30 && y < tft.height() / 2 + 30) {
        Serial.println("Freccia sinistra premuta");
        page = page - 1;
        Serial.print("Pagina ");
        Serial.println(page);
      }
    }
    if (page < 7) {
      if (x > tft.width() - 60 && x < tft.width() - 20 && y > tft.height() / 2 - 30 && y < tft.height() / 2 + 30) {
        Serial.println("Freccia destra premuta");
        page = page + 1;
        Serial.print("Pagina ");
        Serial.println(page);
      }
    } else {
      Serial.println("Pagine finite");
    }
    switch (page) {
      case 1:
        page1();
        break;
      case 2:
        page2();
        break;
      case 3:
        page3();
        break;
      case 4:
        page4();
        break;
      case 5:
        page5();
        break;
      case 6:
        page6();
        break;
      case 7:
        page7();
        break;
      default:
        page1();  // se per errore page esce dal range, torna alla pagina 1
        break;
    }
  }
}

void checkInactivity() {
  if ((millis() - lastActivity > timeout) && page != 0) {
    Serial.println("⏳ Timeout inattività! Ritorno a 🏠 Home (page0)");
    page0();
  }
}

void fetchAndParseICal() {
  now = time(nullptr);  // ora corrente
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

// sostituisce sequenze di escape ICS tipo \n \, \\ ecc.
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
    // val es: YYYYMMDD
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
    return mktime(&tm);  // interpretato come locale (midnight locale)
  } else {
    // val es: YYYYMMDDTHHMMSS  (se arrivava con Z, chiamante ha tolto la Z)
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

    // per stringhe UTC (Z) vogliamo che mktime interpreti tm come UTC.
    // cambiamo temporaneamente TZ a UTC -> mktime restituisce epoch corretto.
    char *oldTZ = getenv("TZ");
    char *oldCopy = NULL;
    if (oldTZ) oldCopy = strdup(oldTZ);
    if (isUtc) {
      setenv("TZ", "UTC0", 1);
      tzset();
    }
    time_t t = mktime(&tm);
    // ripristina TZ
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
    curr = Event();  // reset
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
  int y = 80;  // posizione verticale iniziale
  int x = 80;  // posizione verticale iniziale
  tft.setTextColor(TFT_WHITE, sfondo_page2);
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

    // Se ci avviciniamo alla fine dello schermo, fermiamo il ciclo
    if (y > tft.height() - 20) {
      break;  // mostra solo quello che entra nello schermo
    }
  }
}

void printTasksTFT() {
  int y = 80;  // posizione verticale iniziale
  int x = 80;  // posizione verticale iniziale tft.setTextSize(2);
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

void touch_calibrate() {
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // Controlla se esiste già calibrazione
  if (SPIFFS.exists(CALIBRATION_FILE)) {
    if (REPEAT_CAL) {
      SPIFFS.remove(CALIBRATION_FILE);
    } else {
      File f = SPIFFS.open(CALIBRATION_FILE, "r");
      if (f) {
        if (f.readBytes((char *)calData, 14) == 14)
          calDataOK = 1;
        f.close();
      }
    }
  }

  // Esegui nuova calibrazione
  Serial.println("Iniziando calibrazione touch...");
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(50, 50);
  tft.setTextFont(2);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);

  tft.println("Touch agli angoli come indicato");

  tft.setTextFont(1);
  tft.println();

  if (REPEAT_CAL) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.println("Imposta REPEAT_CAL a false");
    tft.println("per salvare la calibrazione");
  }

  // Calibra
  tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 20);

  // tft.setTextColor(TFT_GREEN, TFT_BLACK);
  // tft.println("Calibrazione completa!");

  // Salva calibrazione
  File f = SPIFFS.open(CALIBRATION_FILE, "w");
  if (f) {
    f.write((const unsigned char *)calData, 14);
    f.close();
    Serial.println("Calibrazione salvata");
  }

  /* tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 20);
  tft.println("Touch test - calibrato");*/
}

void drawArrows() {
  // Freccia sinistra
  tft.fillTriangle(
    20, tft.height() / 2,       // punta
    60, tft.height() / 2 - 30,  // angolo alto
    60, tft.height() / 2 + 30,  // angolo basso
    COLOR_ARROW);

  // Freccia destra
  tft.fillTriangle(
    tft.width() - 20, tft.height() / 2,       // punta
    tft.width() - 60, tft.height() / 2 - 30,  // angolo alto
    tft.width() - 60, tft.height() / 2 + 30,  // angolo basso
    COLOR_ARROW);
}

void drawWiFiSymbol(int x, int y) {
  bool connected = (WiFi.status() == WL_CONNECTED);
  uint16_t color = connected ? TFT_GREEN : TFT_RED;

  tft.fillRect(x - 20, y - 20, 40, 40, TFT_BLUE);  // cancella area

  // Punto centrale
  tft.fillCircle(x, y + 12, 3, color);

  // Semicerchi (approssimati con linee)
  for (int r = 6; r <= 18; r += 4) {
    for (int angle = 230; angle <= 320; angle += 1) {  // semicerchio
      int px = x + r * cos(angle * PI / 180.0);
      int py = y + r * sin(angle * PI / 180.0) + 12;
      tft.drawPixel(px, py, color);
    }
  }
}

void drawScrollBar(int x, int y, int h, int posizioni) {
  //se 0 tutte e due le frecce, se 1 solo quella su, se 2 solo quella giu
  int w = 25;                     // larghezza fissa della barra
  int arrowH = 25;                // altezza area freccia
  int trackH = h - (arrowH * 2);  // area centrale

  // sfondo
  tft.fillRect(x, y, w, h, TFT_DARKGREY);

  // freccia SU
  if (posizioni == 0 || posizioni == 1) {
    tft.fillTriangle(x + w / 2, y + 5, x + 5, y + arrowH - 5, x + w - 5, y + arrowH - 5, TFT_WHITE);
  }
  // freccia GIÙ
  int yb = y + h - arrowH;
  if (posizioni == 0 || posizioni == 2) {
    tft.fillTriangle(x + 5, yb + 5, x + w - 5, yb + 5, x + w / 2, yb + arrowH - 5, TFT_WHITE);
  }
  // binario centrale
  tft.fillRect(x + w / 3, y + arrowH, w / 3, trackH, TFT_BLACK);
}

void stato_scroll_bar1() {
  for (int y = 70; y < 320; y = y + 50) {
    tft.fillRoundRect(15, y, 420, 40, 15, TFT_LIGHTGREY);
  }

  drawScrollBar(450, 20, 300, 2);
  tft.fillRect(453, 45, 21, 20, TFT_GREEN);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(30, 80);
  tft.println("1 Collegamento Wi-Fi");
  drawCircleWithDot(415, 90, 15);
  tft.setCursor(30, 130);
  tft.println("2 Sinc NTP");
  drawCircleWithDot(415, 140, 15);
  tft.setCursor(30, 180);
  tft.println("3 Calib touch");
  drawCircleWithDot(415, 190, 15);
  tft.setCursor(30, 230);
  tft.println("4");
  tft.setCursor(30, 280);
  tft.println("5");
  while (1) {
    stato_scroll_bar = touchMenu(stato_scroll_bar);
    TouchPoint tp = touch_coordinate();
    if (tp.touched) {
      if (tp.x > 410 && tp.x < 430 && tp.y > 220 && tp.y < 250) {
        drawCaricamento(415, 90, 2);
        connessioneWiFi();
        drawCaricamento(415, 90, 2);
        tft.fillRect(400, 75, 31, 31, TFT_LIGHTGREY);
        drawCircleWithDot(415, 90, 15);
      } else if (tp.x > 410 && tp.x < 430 && tp.y > 150 && tp.y < 180) {
        Serial.println("ricalibrazione ntp");
        drawCaricamento(415, 140, 2);
        connessioneNTP();
        drawCaricamento(415, 140, 2);
        tft.fillRect(400, 125, 31, 31, TFT_LIGHTGREY);
        drawCircleWithDot(415, 140, 15);
      } else if (tp.x > 410 && tp.x < 430 && tp.y > 100 && tp.y < 130) {
        Serial.println("calibrazione touch");
        drawCaricamento(415, 190, 2);
        touch_calibrate();
        esci_dal_loop=0;
        pageImpostazioni();
      }
    }
    if (stato_scroll_bar != 1) {
      Serial.println("ho braikato");
      pageImpostazioni();
    }
  }
}

void stato_scroll_bar2() {
  int knobY = 49.6;

  for (int y = 70; y < 320; y = y + 50) {
    tft.fillRoundRect(15, y, 420, 40, 15, TFT_LIGHTGREY);
  }

  drawScrollBar(450, 20, 300, 0);
  tft.fillRect(453, 150, 21, 20, TFT_GREEN);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(30, 80);
  tft.println("6");
  tft.setCursor(30, 130);
  tft.println("7");
  tft.setCursor(30, 180);
  tft.println("8");
  tft.setCursor(30, 230);
  tft.println("9");
  tft.setCursor(30, 280);
  tft.println("10");
  while (1) {
    stato_scroll_bar = touchMenu(stato_scroll_bar);
    //  Serial.println("stato della scroll bar 2");
    if (stato_scroll_bar != 2) {
      Serial.println("ho braikato");
      pageImpostazioni();
    }
  }
}

void stato_scroll_bar3() {
  for (int y = 70; y < 320; y = y + 50) {
    tft.fillRoundRect(15, y, 420, 40, 15, TFT_LIGHTGREY);
  }

  drawScrollBar(450, 20, 300, 1);
  tft.fillRect(453, 275, 21, 20, TFT_GREEN);
  tft.setTextSize(3);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(30, 80);
  tft.println("11");
  tft.setCursor(30, 130);
  tft.println("12");
  tft.setCursor(30, 180);
  tft.println("13");
  tft.setCursor(30, 230);
  tft.println("14");
  tft.setCursor(30, 280);
  tft.println("15");
  while (1) {
    stato_scroll_bar = touchMenu(stato_scroll_bar);
    // Serial.println("stato della scroll bar 3");
    if (stato_scroll_bar != 3) {
      Serial.println("ho braikato");
      pageImpostazioni();
    }
  }
}

int touchMenu(int stato_scroll_bar) {
  uint16_t x, y;  // coordinate touch
  if (tft.getTouch(&x, &y)) {

    // freccia su
    if (x > 440 && y < 25) {
      if (stato_scroll_bar < 3) {
        Serial.println(stato_scroll_bar);
        stato_scroll_bar++;
        Serial.println("⬇️ Scroll DOWN");
      } else {
        //  Serial.println("limite");
      }
    }

    // freccia giù
    if (x > 440 && y > 270) {
      if (stato_scroll_bar > 1) {
        stato_scroll_bar--;
        Serial.println("⬆️ Scroll UP");
        Serial.println(stato_scroll_bar);
      } else {
        //  Serial.println("limite");
      }
    }
  }

  return stato_scroll_bar;  // Restituisce il valore (modificato o no)
}

void drawGearIcon(int x, int y) {
  uint16_t color = TFT_WHITE;  // Colore fisso bianco per l'ingranaggio

  // Cancella area (40x40 per avere margine)
  tft.fillRect(x - 20, y - 20, 40, 40, sfondo_page0);

  // Centro dell'ingranaggio - cerchio interno (foro centrale)
  tft.drawCircle(x, y, 5, color);
  tft.drawCircle(x, y, 4, color);

  // Corpo principale dell'ingranaggio - cerchio esterno
  tft.drawCircle(x, y, 12, color);
  tft.drawCircle(x, y, 11, color);

  // Disegna 8 denti dell'ingranaggio
  for (int i = 0; i < 8; i++) {
    int angle = i * 45;  // 360/8 = 45 gradi tra ogni dente

    // Calcola posizione del dente esterno
    int x1 = x + 13 * cos(angle * PI / 180.0);
    int y1 = y + 13 * sin(angle * PI / 180.0);
    int x2 = x + 16 * cos(angle * PI / 180.0);
    int y2 = y + 16 * sin(angle * PI / 180.0);

    // Calcola i lati del dente (±10 gradi dall'angolo centrale)
    int x3 = x + 16 * cos((angle - 10) * PI / 180.0);
    int y3 = y + 16 * sin((angle - 10) * PI / 180.0);
    int x4 = x + 16 * cos((angle + 10) * PI / 180.0);
    int y4 = y + 16 * sin((angle + 10) * PI / 180.0);

    int x5 = x + 13 * cos((angle - 10) * PI / 180.0);
    int y5 = y + 13 * sin((angle - 10) * PI / 180.0);
    int x6 = x + 13 * cos((angle + 10) * PI / 180.0);
    int y6 = y + 13 * sin((angle + 10) * PI / 180.0);

    // Disegna il dente con linee
    tft.drawLine(x1, y1, x2, y2, color);  // Linea centrale del dente
    tft.drawLine(x5, y5, x3, y3, color);  // Lato sinistro
    tft.drawLine(x6, y6, x4, y4, color);  // Lato destro
    tft.drawLine(x3, y3, x4, y4, color);  // Punta del dente
  }

  // Rinforza il cerchio interno per maggiore visibilità
  for (int r = 3; r <= 6; r++) {
    tft.drawCircle(x, y, r, color);
  }

  // Rinforza il cerchio esterno
  for (int r = 10; r <= 13; r++) {
    tft.drawCircle(x, y, r, color);
  }
}

void connessioneWiFi() {
  int tentativi = 0;
  const int maxTentativi = 5;

  Serial.print("📶 Connessione al WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED && tentativi < maxTentativi) {
    delay(1000);  // attesa 1 secondo tra i tentativi
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

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
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

void drawCircleWithDot(int x, int y, int radius) {
  int innerRadius = radius / 3;  // Il pallino è 1/3 del raggio del cerchio
  int circleThickness = 5;       // Spessore proporzionale al raggio

  if (circleThickness < 1) circleThickness = 1;  // Minimo 1 pixel

  for (int i = 0; i < circleThickness; i++) {
    tft.drawCircle(x, y, radius - i, TFT_WHITE);
  }

  tft.fillCircle(x, y, innerRadius, TFT_WHITE);
}

void drawHouse() {
  int x = 20;
  int width = 35;
  int y = 20;
  int height = width * 0.8;       // Altezza casa
  int roofHeight = width * 0.4;   // Altezza tetto
  int doorWidth = width * 0.25;   // Larghezza porta
  int doorHeight = height * 0.6;  // Altezza porta
  int windowSize = width * 0.15;  // Dimensione finestre

  // Colori
  uint16_t wallColor = TFT_YELLOW;
  uint16_t roofColor = TFT_RED;
  uint16_t doorColor = TFT_BROWN;
  uint16_t windowColor = TFT_CYAN;
  uint16_t frameColor = TFT_WHITE;

  // 1. Disegna il corpo della casa (rettangolo)
  tft.fillRect(x, y, width, height, wallColor);
  tft.drawRect(x, y, width, height, frameColor);  // Contorno

  // 2. Disegna il tetto (triangolo)
  int roofTopX = x + width / 2;
  int roofTopY = y - roofHeight;

  // Triangolo del tetto (3 linee)
  tft.drawLine(x, y, roofTopX, roofTopY, roofColor);          // Lato sinistro
  tft.drawLine(x + width, y, roofTopX, roofTopY, roofColor);  // Lato destro
  tft.drawLine(x, y, x + width, y, roofColor);                // Base

  // Riempie il tetto con linee orizzontali
  for (int i = 0; i < roofHeight; i++) {
    int lineY = roofTopY + i;
    int lineWidth = (i * width) / roofHeight;
    int lineStartX = roofTopX - lineWidth / 2;
    int lineEndX = roofTopX + lineWidth / 2;
    tft.drawLine(lineStartX, lineY, lineEndX, lineY, roofColor);
  }

  // 3. Disegna la porta
  int doorX = x + (width - doorWidth) / 2;
  int doorY = y + height - doorHeight;
  tft.fillRect(doorX, doorY, doorWidth, doorHeight, doorColor);
  tft.drawRect(doorX, doorY, doorWidth, doorHeight, frameColor);

  // Maniglia della porta
  int handleX = doorX + doorWidth - 4;
  int handleY = doorY + doorHeight / 2;
  tft.fillCircle(handleX, handleY, 2, TFT_DARKGREY);

  // 4. Disegna le finestre
  int windowY = y + height / 4;

  // Finestra sinistra
  int leftWindowX = x + width * 0.15;
  tft.fillRect(leftWindowX, windowY, windowSize, windowSize, windowColor);
  tft.drawRect(leftWindowX, windowY, windowSize, windowSize, frameColor);
  // Croce della finestra
  tft.drawLine(leftWindowX + windowSize / 2, windowY,
               leftWindowX + windowSize / 2, windowY + windowSize, frameColor);
  tft.drawLine(leftWindowX, windowY + windowSize / 2,
               leftWindowX + windowSize, windowY + windowSize / 2, frameColor);

  // Finestra destra
  int rightWindowX = x + width * 0.6;
  tft.fillRect(rightWindowX, windowY, windowSize, windowSize, windowColor);
  tft.drawRect(rightWindowX, windowY, windowSize, windowSize, frameColor);
  // Croce della finestra
  tft.drawLine(rightWindowX + windowSize / 2, windowY,
               rightWindowX + windowSize / 2, windowY + windowSize, frameColor);
  tft.drawLine(rightWindowX, windowY + windowSize / 2,
               rightWindowX + windowSize, windowY + windowSize / 2, frameColor);

  // 5. Camino (opzionale)
  int chimneyWidth = width * 0.1;
  int chimneyHeight = roofHeight * 0.6;
  int chimneyX = x + width * 0.75;
  int chimneyY = roofTopY + roofHeight * 0.3;

  tft.fillRect(chimneyX, chimneyY, chimneyWidth, chimneyHeight, TFT_DARKGREY);
  tft.drawRect(chimneyX, chimneyY, chimneyWidth, chimneyHeight, frameColor);

  // Serial.printf("Casa disegnata a (%d, %d) con larghezza %d\n", x, y, width);
}

void drawCaricamento(int cx, int cy, int num_giri) {
  Serial.println("Pallini del caricamento ...");

  uint16_t dotcolor = TFT_WHITE;
  uint16_t sfondo = TFT_LIGHTGREY;
  int raggio = 15;  // distanza dei pallini dal centro

  // Sfondo
  tft.fillRect(cx - raggio, cy - raggio, 31, 31, sfondo);

  for (int i = 0; i < num_giri; i++) {
    for (int angolo = 0; angolo < 360; angolo += 40) {
      float rad = angolo * PI / 180.0;  // gradi → radianti
      float rad_precedente = (angolo - 80) * PI / 180.0;

      // Disegna il pallino attivo
      int x = cx + raggio * cos(rad);
      int y = cy + raggio * sin(rad);
      tft.fillCircle(x, y, 3, dotcolor);

      // Cancella il precedente
      int x_old = cx + raggio * cos(rad_precedente);
      int y_old = cy + raggio * sin(rad_precedente);
      tft.fillCircle(x_old, y_old, 3, sfondo);

      delay(100);  // tempo di animazione
    }
  }
}
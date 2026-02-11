#ifndef CONFIG_H
#define CONFIG_H

// ============================================================================
// LIBRERIE
// ============================================================================
#include <FS.h>
#include <SPIFFS.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

// ============================================================================
// DEFINIZIONI DISPLAY
// ============================================================================
#define ST7796_DRIVER
#define TFT_WIDTH 320
#define TFT_HEIGHT 480

// ============================================================================
// CONFIGURAZIONE TOUCH
// ============================================================================
#define CALIBRATION_FILE "/TouchCalData2"
#define REPEAT_CAL false

// ============================================================================
// COLORI 
// ============================================================================
#define sfondo_page0 TFT_BLUE
#define sfondo_pageCalendario TFT_YELLOW
#define sfondo_pageTask TFT_BLUE
#define sfondo_page3 TFT_RED
#define sfondo_page4 0x7BEF
#define sfondo_page5 TFT_YELLOW
#define sfondo_page6 TFT_CYAN
#define sfondo_page7 TFT_MAGENTA
#define sfondo_pageImpostazioni TFT_DARKGREY

#define COLOR_ARROW TFT_WHITE
#define COLOR_BG        0x0841 
#define COLOR_CARD      0x10A4 
#define COLOR_TEXT      0xFFFF 
#define COLOR_BLUE      0x34BF 
#define COLOR_ORANGE    0xFB00 
#define COLOR_PURPLE    0x921F 
#define COLOR_GREEN     0x3666 
#define COLOR_YELLOW    0xF600 
#define COLOR_GRAY      0x7BEF 
#define COLOR_RED       0xC800 // Nuovo colore per Scene
#define COLOR_CYAN      0x07FF // Nuovo colore per Rooms
#define COLOR_TEAL      0x0410 // Nuovo colore per Info

// ============================================================================
// CONFIGURAZIONE RETE
// ============================================================================
#define WIFI_SSID "A25 di Matteo"
#define WIFI_PASSWORD "matteo123"
#define NTP_SERVER "pool.ntp.org"
#define ICAL_URL "https://calendar.google.com/calendar/ical/casettamatteo1%40gmail.com/public/basic.ics"

// Timezone Italia
#define GMT_OFFSET_SEC 3600
#define DAYLIGHT_OFFSET_SEC 3600

// ============================================================================
// LIMITI E COSTANTI
// ============================================================================
#define MAX_EVENTS 3
#define MAX_TASKS 10
#define TASKS_PER_PAGE 10
#define INACTIVITY_TIMEOUT 30000  // 30 secondi in millisecondi

// ============================================================================
// STRUTTURE DATI
// ============================================================================

/**
 * Struttura per le coordinate touch
 */
struct TouchPoint {
  uint16_t x;
  uint16_t y;
  bool touched;
};

/**
 * Struttura per gli eventi del calendario
 */
struct Event {
  String summary;
  String description;
  time_t start = 0;
  time_t end = 0;
  bool allDay = false;
};

/**
 * Struttura per le attività (To-Do)
 */
struct Task {
  String title;
  bool done;
  time_t due;  // scadenza opzionale
};

// ============================================================================
// VARIABILI GLOBALI (extern - definite in smart_display.ino)
// ============================================================================

// Display e tempo
extern TFT_eSPI tft;
extern struct tm timeinfo;
extern String dataLunga;
extern String ora;
extern time_t now;

// Eventi e task
extern Event events[];
extern Task tasks[];
extern int eventsCount;
extern int tasksCount;
extern int pageIndex;

// Navigazione
extern int page;
extern int lastPage;
extern int stato_scroll_bar;
extern int esci_dal_loop;
extern unsigned long lastActivity;

// LED RGB (per funzionalità future)
extern int led_r;
extern int led_g;
extern int led_b;
extern bool led_acceso;
extern int luminosita;

// ============================================================================
// FORWARD DECLARATIONS (Prototipi delle funzioni)
// ============================================================================

// GUI Functions
void drawArrows();
void drawWiFiSymbol(int x, int y);
void drawGearIcon(int x, int y);
void drawGearIcon(int x, int y, uint16_t bg);
void drawCircleWithDot(int x, int y, int radius);
void drawHouse();
void drawCaricamento(int cx, int cy, int num_giri);
void drawScrollBar(int x, int y, int h, int posizioni);

// Network & Time Functions
void connessioneWiFi();
void connessioneNTP();
String getDateLong();
String getTime();
String icalUnescape(String s);
time_t parseICalDateToTime(const String &val, bool isUtc, bool dateOnly);
void fetchAndParseICal();
void parseICalStream(WiFiClient *stream);
void processLine(String &line, bool &inEvent, Event &curr);
void printEvents();
void printEventsTFT();
void printTasksTFT();

// Touch Functions
TouchPoint touch_coordinate();
void touch_calibrate();
void test_touch();

// Logic Pages Functions
extern int page;
extern int lastPage;

void disegnaGrigliaHome();
void pageprincipale();
void pageCalendario();
void pageTask();
void page3();
void page4();
void page5();
void page6();
void page7();
void page8();
void pageImpostazioni();
void cambio_pagina();
void checkInactivity();
void stato_scroll_bar1();
void stato_scroll_bar2();
void stato_scroll_bar3();
int touchMenu(int stato_scroll_bar);
void disegnaHome();
void waitRelease();
void switchPage(int p);

#endif // CONFIG_H
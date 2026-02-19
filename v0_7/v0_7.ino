#include "config.h"

// ============================================================================
// DEFINIZIONE VARIABILI GLOBALI
// ============================================================================
TFT_eSPI tft = TFT_eSPI();
struct tm timeinfo;
Event events[MAX_EVENTS];
Task tasks[MAX_TASKS];

int eventsCount = 0;
int tasksCount = 0;
int page = 0;
int lastPage = -1;
int pageIndex = 0;
int stato_scroll_bar = 1;
int esci_dal_loop = 1;

unsigned long lastActivity = 0;
time_t now;
String dataLunga;
String ora;

// LED RGB
int led_r = 255;
int led_g = 255;
int led_b = 255;
bool led_acceso = true;
int luminosita = 100;

// #region agent log
static void dbgLog(const char* location, const char* message, int a = -1, int b = -1, int c = -1, int d = -1) {
  unsigned long ts = millis();
  Serial.print("[DBG] t=");
  Serial.print(ts);
  Serial.print(" loc=");
  Serial.print(location);
  Serial.print(" msg=");
  Serial.print(message);
  Serial.print(" a=");
  Serial.print(a);
  Serial.print(" b=");
  Serial.print(b);
  Serial.print(" c=");
  Serial.print(c);
  Serial.print(" d=");
  Serial.println(d);
}
// #endregion

// ============================================================================
// INCLUDE DEI MODULI (DOPO le definizioni delle variabili!)
// ============================================================================
#include "gui_functions.h"
#include "logic_pages.h"
#include "network_time.h"
#include "touch_calibration.h"

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("✨ Inizializzazione ESP32-S3... ✨");

  // Inizializza SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("Errore montaggio SPIFFS");
  }

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

  // --- FASE 1: Avvio File System ---
  updateLoadingScreen(10, "Inizializzazione File System...");
  delay(200);

  // --- FASE 2: Connessione WiFi ---
  updateLoadingScreen(30, "Connessione al WiFi...");
  connessioneWiFi();
  connessioneNTP();
  delay(200);

  // --- FASE 3: Sincronizzazione Orario ---
  updateLoadingScreen(50, "Sincronizzazione Orario NTP...");
  setenv("TZ", "CET-1CEST,M3.5.0/02:00,M10.5.0/03:00", 1);
  tzset();
  delay(200);

  // --- FASE 4:Lettura Calendario ---
  updateLoadingScreen(80, "Lettura Calendario...");
  // commentati per far eseguire piu velocemente il codice -- per la versine finale togliere commenti
  //fetchAndParseICal();
  //printEvents();
  delay(200);

  // --- FASE 5: Completamento ---
  updateLoadingScreen(100, "Avvio completato!");
  Serial.println("🚀 Setup completato!");
  delay(1000);  // Lascia leggere all'utente "Completato"

  page = 0;
  disegnaHome();
  dbgLog("v0_7.ino:setup", "home.drawn", page, 0, 0, 0);
}

// ============================================================================
// LOOP PRINCIPALE
// ============================================================================
void loop() {
  ora = getTime();
  static String ultimaOra = "";

  if (ora != ultimaOra) {
    ultimaOra = ora;
    dataLunga = getDateLong();

    tft.setTextColor(TFT_WHITE, sfondo_page0);

    tft.setTextSize(4);
    tft.setCursor(130, 130);
    tft.println(ora);

    tft.setTextSize(3);
    tft.setCursor(75, 200);
    tft.println(dataLunga);
  }

  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    dbgLog("v0_7.ino:loop", "touch", x, y, page, 0);
    if (x > 400 && y > 300) {  // Area Ingranaggio
      Serial.println("Apertura Impostazioni");
      delay(200);  // Debounce
      dbgLog("v0_7.ino:loop", "go.settings", x, y, page, 0);
      pageImpostazioni();
    } else {
      dbgLog("v0_7.ino:loop", "go.pageprincipale", x, y, page, 0);
      pageprincipale();
    }
  }
}
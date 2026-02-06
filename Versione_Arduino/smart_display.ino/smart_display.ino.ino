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

// ============================================================================
// INCLUDE DEI MODULI (DOPO le definizioni delle variabili!)
// ============================================================================
#include "gui_functions.h"
#include "network_time.h"
#include "touch_calibration.h"
#include "logic_pages.h"

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

  // Messaggio sul display
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(4);
  tft.setCursor(20, 100);
  tft.println("Stiamo preparando");
  tft.println(" il tuo dispositivo");

  // Connessioni
  connessioneWiFi();
  connessioneNTP();
  touch_calibrate();

  // Imposta timezone (Europe/Rome)
  setenv("TZ", "CET-1CEST,M3.5.0/02:00,M10.5.0/03:00", 1);
  tzset();

  // Opzionale: scarica eventi calendario
  // fetchAndParseICal();
  // printEvents();

  Serial.println("🚀 Setup completato!");

  // Schermata principale
  tft.fillScreen(sfondo_page0);
  drawWiFiSymbol(400, 20);
  drawGearIcon(445, 25);

  tft.setTextColor(TFT_WHITE, sfondo_page0);
  tft.setTextSize(5);
  tft.setCursor(60, 50);
  tft.println("Casa Casetta");
  tft.setTextSize(2);
  tft.setCursor(120, 300);
  tft.println("Premi per proseguire");
}

// ============================================================================
// LOOP PRINCIPALE
// ============================================================================
void loop() {
  uint16_t x, y;
  page = 0;
  dataLunga = getDateLong();
  ora = getTime();

  // Mostra ora e data
  tft.setTextSize(4);
  tft.setCursor(130, 130);
  tft.println(ora);
  tft.setTextSize(3);
  tft.setCursor(75, 200);
  tft.println(dataLunga);

  // Gestione touch
  if (tft.getTouch(&x, &y)) {
    if (x > 440 && y > 280) {
      Serial.println("Apertura Impostazioni");
      pageImpostazioni();
    } else {
      Serial.println("Apertura Pagina 1");
      page1();
    }
  }
}
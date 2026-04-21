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
int eventiPageIndex = 0; // Nuova variabile per la paginazione eventi
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

// Sensore KY-001 (DS18B20)
float roomTemp = 0.0f;
float roomHum  = 0.0f;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);



// ============================================================================
// INCLUDE DEI MODULI (DOPO le definizioni delle variabili!)
// ============================================================================
#include "gui_functions.h"
#include "bus_schedule.h"
#include "logic_pages.h"
#include "network_time.h"
#include "touch_calibration.h"

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(500);
  sensors.begin();  // Inizializza sensore KY-001 (DS18B20)
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
  load_touch_calibration();

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
  fetchAndParseICal();
  printEvents();
  printTasks();
  delay(200);

  // --- FASE 5: Completamento ---
  updateLoadingScreen(100, "Avvio completato!");
  Serial.println("🚀 Setup completato!");
  delay(1000);  

  page = 0;
  disegnaHome();
}

// ============================================================================
// LOOP PRINCIPALE
// ============================================================================
void loop() {
  // Aggiornamento periodico del calendario (ogni 30 minuti = 1800000 ms)
  static unsigned long lastCalendarUpdate = millis();
  if (millis() - lastCalendarUpdate > 1800000) {
    lastCalendarUpdate = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("🔄 Sincronizzazione periodica del calendario in corso...");
      fetchAndParseICal();
      Serial.println("✅ Sincronizzazione calendar completata.");
    }
  }

  ora = getTime();
  static String ultimaOra = "";

  if (ora != ultimaOra) {//ora
    ultimaOra = ora;
    dataLunga = getDateLong();

    if (page == 0) {
      tft.setTextColor(TFT_WHITE, sfondo_page0);

      tft.setTextSize(4);
      tft.setCursor(130, 130);
      tft.println(ora);

      tft.setTextSize(3);
      tft.setCursor(75, 200);
      tft.println(dataLunga);
    }
  }

  uint16_t x, y;//touch
  if (tft.getTouch(&x, &y)) {
    if (x > 400 && y > 260) {  // Area Ingranaggio (touch Y invertito: y alto = display top)
      delay(200);  // Debounce
      page = 9;  // IMPORTANTE: deve essere != 0 prima di entrare, altrimenti la guard if(page==0) esce subito
      pageImpostazioni();
    } else {
      pageprincipale();
    }
  }
}
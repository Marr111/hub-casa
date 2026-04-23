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
char dataLunga[40] = "";
char ora[10] = "";

// ============================================================================
// SINCRONIZZAZIONE CODICE
// ============================================================================
const char* FIRMWARE_VERSION = "1.0"; // La tua versione attuale
String availableVersion = "";
String downloadURL = "";
String expectedSha256 = "";           // Hash SHA256 del firmware da scaricare
bool updateAvailable = false;
unsigned long lastVersionCheck = 0;
const unsigned long VERSION_CHECK_INTERVAL = 86400000; // 24 ore

// LED RGB
int led_r = 255;
int led_g = 255;
int led_b = 255;
bool led_acceso = true;
int luminosita = 100;

// Sensore KY-001 (DS18B20) — misura solo temperatura, NON umidità
float roomTemp = 0.0f;
// roomHum rimossa: KY-001 non misura l'umidità (era sempre 0.0 → mostrava "--" inutilmente)
bool sensorReady = false;   // true dopo la prima lettura valida (evita bug: 0°C reale = "--")
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);



// ============================================================================
// INCLUDE DEI MODULI (DOPO le definizioni delle variabili!)
// ============================================================================
#include "gui_functions.h"
#include "bus_schedule.h"
#include "touch_calibration.h"   // DEVE stare prima di logic_pages.h (force_touch_calibrate)
#include "network_time.h"
#include "logic_pages.h"

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

  // --- FASE 2: Connessione WiFi ---
  updateLoadingScreen(30, "Connessione al WiFi...");
  connessioneWiFi();
  connessioneNTP();

  // --- FASE 3: Sincronizzazione Orario ---
  updateLoadingScreen(50, "Sincronizzazione Orario NTP...");
  setenv("TZ", "CET-1CEST,M3.5.0/02:00,M10.5.0/03:00", 1);
  tzset();

  // --- FASE 4: Lettura Calendario ---
  // IMPORTANTE: fetchAndParseICal() DEVE stare PRIMA di printEvents()/printTasks().
  // Se si decommenta il fetch, decommentare anche print DOPO di esso.
  updateLoadingScreen(80, "Lettura Calendario...");
  //fetchAndParseICal();  // <-- decommentare per abilitare download calendario
  //printEvents();        // <-- chiamare solo DOPO fetchAndParseICal()
  //printTasks();         // <-- idem

  // --- FASE 5: Completamento ---
  updateLoadingScreen(100, "Avvio completato!");
  Serial.println("Setup completato!");
  delay(800);

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

  // Riconnessione automatica WiFi (ogni 10 minuti se disconnesso - fix 4.5)
  static unsigned long lastWiFiCheck = 0;
  if (millis() - lastWiFiCheck > 600000) { // 10 minuti
    lastWiFiCheck = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️ WiFi disconnesso, tentativo di riconnessione...");
      WiFi.disconnect();
      connessioneWiFi();
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("✅ WiFi riconnesso.");
        connessioneNTP(); // Risincronizza anche l'orario
        // Ridisegna l'icona WiFi se siamo sulla home
        if (page == 0) disegnaHome();
      } else {
        Serial.println("❌ Riconnessione WiFi fallita. Prossimo tentativo tra 10 minuti.");
      }
    }
  }

  const char* currentOra = getTime();
  static char ultimaOra[10] = "";

  if (strcmp(currentOra, ultimaOra) != 0) {//ora
    strcpy(ultimaOra, currentOra);
    strcpy(ora, currentOra);
    strcpy(dataLunga, getDateLong());

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
    if (x > GEAR_BTN_X_MIN && y < GEAR_BTN_Y_MAX) {  // Area Ingranaggio (angolo alto-dx)
      delay(DEBOUNCE_MS);
      page = 9;  // IMPORTANTE: deve essere != 0 prima di entrare
      pageImpostazioni();
    } else {
      pageprincipale();
    }
  }
}

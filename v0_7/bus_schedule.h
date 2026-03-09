#ifndef BUS_SCHEDULE_H
#define BUS_SCHEDULE_H

#include "config.h"

// ============================================================================
// PAGINA ORARI PULLMAN / METRO
// Tragitto 1: A piedi → Metro Bengasi → Vinzaglio → A piedi → Politecnico
// Tragitto 2: A piedi → Fermata 3064  → Bus 2014  → Montà
// ============================================================================

// ============================================================================
// STRUTTURE DATI
// ============================================================================

#define MAX_BUS_DEPARTURES  8
#define BUS_CARD_MAX_ROWS   4

// Tempi a piedi (minuti)
#define WALK_TO_METRO_MIN   15   // casa → fermata metro Bengasi
#define WALK_FROM_METRO_MIN 10   // Vinzaglio → Politecnico
#define WALK_TO_BUS_MIN     20   // casa → fermata 3064

// Durata in mezzo (minuti)
#define METRO_RIDE_MIN       8   // Bengasi → Vinzaglio (~6 fermate)
#define BUS_2014_RIDE_MIN   15   // fermata 3064 → Montà (da aggiustare)

struct BusDeparture {
  char line[8];    // es. "2014", "M1"
  char hour[9];    // "HH:MM:SS"
  bool realtime;   // true = GPS live, false = teorico
  bool valid;      // true = slot utilizzato
};

// Stato manuale festivo (sovrascrive il rilevamento automatico)
static bool festivoManuale = false;

// ============================================================================
// LOGICA FERIALE / FESTIVO
// ============================================================================

// Restituisce true se oggi è festivo (weekend + festività italiane fisse)
bool isFestivo() {
  time_t now_t = time(nullptr);
  struct tm* t = localtime(&now_t);
  if (!t) return false;

  int wday = t->tm_wday;   // 0=Dom, 6=Sab
  int day  = t->tm_mday;
  int mon  = t->tm_mon + 1; // 1-12

  if (wday == 0 || wday == 6) return true;

  // Festività italiane fisse
  if (mon ==  1 && day ==  1) return true; // Capodanno
  if (mon ==  1 && day ==  6) return true; // Epifania
  if (mon ==  4 && day == 25) return true; // Liberazione
  if (mon ==  5 && day ==  1) return true; // Festa del Lavoro
  if (mon ==  6 && day ==  2) return true; // Repubblica
  if (mon ==  8 && day == 15) return true; // Ferragosto
  if (mon == 11 && day ==  1) return true; // Ognissanti
  if (mon == 12 && day ==  8) return true; // Immacolata
  if (mon == 12 && day == 25) return true; // Natale
  if (mon == 12 && day == 26) return true; // S. Stefano

  return false;
}

// ============================================================================
// METRO TORINO – ORARI HARDCODED (GTT Linea 1, Bengasi → direzione Fermi)
// Fermata: BENGASI (ultima fermata direzione Lingotto/Fermi)
// L'utente prende metro a Bengasi e scende a VINZAGLIO (6 fermate, ~8 min)
// ============================================================================
//
// GTT Metro M1 frequenze ufficiali (direzione Fermi da Bengasi):
//   Feriale: 05:30-07:00 ogni ~11 min | 07:00-21:00 ogni 5-7 min | 21:00-21:30 ogni 11 min
//   Festivo: 07:00-21:30 ogni 10 min
//
// Per il display mostriamo le prossime partenze calcolate runtime
// in base all'orario attuale + frequenza calcolata

struct MetroSlot {
  int startH, startM; // inizio fascia oraria
  int endH,   endM;   // fine fascia
  int freqMin;         // frequenza in minuti
};

// Metro ogni 4 minuti tutto il giorno (informazione dell'utente)
// Orario servizio: feriale 05:30-21:30, festivo 07:00-21:30
static const MetroSlot metroFeriale[] = {
  { 5, 30, 21, 30, 4 },
};
static const int metroFerialeCount = 1;

static const MetroSlot metroFestivo[] = {
  { 7,  0, 21, 30, 4 },
};
static const int metroFestivoCount = 1;

// Calcola i prossimi N orari metro RAGGIUNGIBILI da adesso
// Tiene conto che ci vogliono WALK_TO_METRO_MIN minuti a piedi
// outHours[], outMins[] = orario partenza metro (non uscita di casa)
// Ritorna quanti orari ha trovato
int getNextMetroDepartures(int* outHours, int* outMins, int maxOut) {
  time_t now_t = time(nullptr);
  struct tm* t = localtime(&now_t);
  if (!t) return 0;

  // Devo arrivare alla fermata tra WALK_TO_METRO_MIN minuti
  int nowMin    = t->tm_hour * 60 + t->tm_min;
  int arriveMin = nowMin + WALK_TO_METRO_MIN;
  // Usa festivoManuale (può essere cambiata dall'utente via badge)
  const MetroSlot* slots = festivoManuale ? metroFestivo : metroFeriale;
  int slotsCount = festivoManuale ? metroFestivoCount : metroFerialeCount;

  int found = 0;

  for (int si = 0; si < slotsCount && found < maxOut; si++) {
    const MetroSlot& s = slots[si];
    int startMin = s.startH * 60 + s.startM;
    int endMin   = s.endH   * 60 + s.endM;

    if (arriveMin >= endMin) continue; // fascia già passata

    // Prima partenza raggiungibile in questa fascia
    int first = startMin;
    if (first < arriveMin) {
      int steps = (arriveMin - first + s.freqMin - 1) / s.freqMin;
      first = startMin + steps * s.freqMin;
    }

    for (int dep = first; dep < endMin && found < maxOut; dep += s.freqMin) {
      outHours[found] = dep / 60;
      outMins[found]  = dep % 60;
      found++;
    }
  }

  return found;
}

// ============================================================================
// BUS LINEA 2014 – FETCH API GPA (Fermata 3064 → Montà)
// ============================================================================

// Scarica i prossimi passaggi dalla fermata 3064, filtra linea "2014"
// Ritorna numero di elementi validi
int fetchBus2014(BusDeparture* out, int maxOut) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[BUS] WiFi non connesso");
    return 0;
  }

  WiFiClientSecure client;
  client.setInsecure(); // Accetta qualsiasi certificato
  HTTPClient http;

  const char* url = "https://gpa.madbob.org/query.php?stop=3064";
  Serial.printf("[BUS] GET %s\n", url);

  if (!http.begin(client, url)) {
    Serial.println("[BUS] http.begin fallito");
    return 0;
  }

  http.setTimeout(8000);
  int code = http.GET();

  if (code != 200) {
    Serial.printf("[BUS] HTTP code: %d\n", code);
    http.end();
    return 0;
  }

  String payload = http.getString();
  http.end();
  Serial.printf("[BUS] payload: %s\n", payload.c_str());

  // Parse JSON – array di oggetti {line, hour, realtime}
  // DynamicJsonDocument: stima 512 bytes per la risposta breve
  DynamicJsonDocument doc(1024);
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("[BUS] JSON err: %s\n", err.c_str());
    return 0;
  }

  int found = 0;
  JsonArray arr = doc.as<JsonArray>();
  for (JsonObject obj : arr) {
    if (found >= maxOut) break;
    const char* line = obj["line"] | "";
    const char* hour = obj["hour"] | "";
    bool rt          = obj["realtime"] | false;

    // Filtra solo linea 2014
    if (strcmp(line, "2014") != 0) continue;

    strncpy(out[found].line, line, sizeof(out[found].line) - 1);
    strncpy(out[found].hour, hour, sizeof(out[found].hour) - 1);
    out[found].realtime = rt;
    out[found].valid    = true;
    found++;
  }

  Serial.printf("[BUS] trovate %d partenze linea 2014\n", found);
  return found;
}

// ============================================================================
// GUI – ICONE
// ============================================================================

// Icona semplice della metro M1 (rettangolo + M1)
void drawMetroIcon(int cx, int cy, uint16_t color) {
  // Rettangolo arrotondato (~metro)
  tft.drawRoundRect(cx, cy, 30, 18, 4, color);
  // Finestrino
  tft.fillRect(cx + 4, cy + 4, 5, 5, color);
  tft.fillRect(cx + 13, cy + 4, 5, 5, color);
  tft.fillRect(cx + 22, cy + 4, 5, 5, color);
  // Ruote
  tft.fillCircle(cx + 7,  cy + 18, 3, color);
  tft.fillCircle(cx + 22, cy + 18, 3, color);
}

// Icona pullman laterale
void drawBusIcon(int cx, int cy, uint16_t color) {
  // Carrozzeria
  tft.drawRoundRect(cx, cy, 38, 20, 3, color);
  // Finestrini
  tft.fillRect(cx + 4,  cy + 4, 7, 8, color);
  tft.fillRect(cx + 14, cy + 4, 7, 8, color);
  tft.fillRect(cx + 24, cy + 4, 7, 8, color);
  // Ruote
  tft.fillCircle(cx + 8,  cy + 20, 4, color);
  tft.fillCircle(cx + 28, cy + 20, 4, color);
  // Faro
  tft.fillRect(cx + 34, cy + 12, 4, 4, TFT_YELLOW);
}

// ============================================================================
// GUI – PAGINA BUS
// ============================================================================

// Dati correnti
static BusDeparture busData[MAX_BUS_DEPARTURES];
static int          busDataCount = 0;
static int          metroHours[MAX_BUS_DEPARTURES];
static int          metroMins[MAX_BUS_DEPARTURES];
static int          metroCount = 0;

// Badge FERIALE / FESTIVO in alto a destra
void drawFerialeFestvoBadge(bool festivo) {
  uint16_t col = festivo ? TFT_RED : TFT_DARKGREEN;
  tft.fillRoundRect(350, 8, 120, 22, 5, col);
  tft.setTextColor(TFT_WHITE, col);
  tft.setTextSize(2);
  tft.setCursor(363, 13);
  tft.print(festivo ? "FESTIVO" : "FERIALE");
}

// Disegna una card con gli orari di un tragitto
// walkMin = minuti a piedi da casa al mezzo (per calcolare "Parti alle")
void drawTragittoCard(int y0, uint16_t accentColor,
                      const char* titolo, const char* subtitle,
                      int* mHours, int* mMins, int mCount,
                      BusDeparture* deps, int dCount,
                      int walkMin) {
  int cardH = 128;
  tft.fillRoundRect(8, y0, 464, cardH, 8, COLOR_CARD);
  tft.fillRect(8, y0, 5, cardH, accentColor);

  // Titolo
  tft.setTextColor(TFT_WHITE, COLOR_CARD);
  tft.setTextSize(2);
  tft.setCursor(20, y0 + 6);
  tft.print(titolo);

  // Sottotitolo
  tft.setTextSize(1);
  tft.setTextColor(0xAD75, COLOR_CARD);
  tft.setCursor(20, y0 + 26);
  tft.print(subtitle);

  // Orari + "Parti alle"
  int rowY  = y0 + 40;
  int shown = 0;

  if (mHours != nullptr) {
    // --- Card metro: "Parti HH:MM --> M1 HH:MM --> Arr HH:MM" ---
    for (int i = 0; i < mCount && shown < BUS_CARD_MAX_ROWS; i++, shown++) {
      int yi = rowY + shown * 18;

      int metroTot  = mHours[i] * 60 + mMins[i];
      int partiTot  = metroTot - walkMin;                              // uscita di casa
      int arrivoTot = metroTot + METRO_RIDE_MIN + WALK_FROM_METRO_MIN; // M1 + a piedi
      if (partiTot < 0) partiTot = 0;

      // "Parti HH:MM"
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE, COLOR_CARD);
      tft.setCursor(14, yi);
      char buf[52];
      snprintf(buf, sizeof(buf), "Parti %02d:%02d --> M1 %02d:%02d --> Arr %02d:%02d",
               partiTot / 60, partiTot % 60,
               mHours[i], mMins[i],
               arrivoTot / 60, arrivoTot % 60);
      tft.print(buf);
    }
  } else if (deps != nullptr) {
    // --- Card bus: "Parti HH:MM --> 2014 HH:MM --> Arr HH:MM" ---
    for (int i = 0; i < dCount && shown < BUS_CARD_MAX_ROWS; i++, shown++) {
      int yi = rowY + shown * 18;

      int bH = 0, bM = 0;
      sscanf(deps[i].hour, "%d:%d", &bH, &bM);

      // Filtra bus non raggiungibili
      time_t now_t = time(nullptr);
      struct tm* nt = localtime(&now_t);
      int nowTot = nt ? (nt->tm_hour * 60 + nt->tm_min) : 0;
      int busTot = bH * 60 + bM;
      if (busTot - nowTot < walkMin) { shown--; continue; }

      int partiTot  = busTot - walkMin;
      int arrivoTot = busTot + BUS_2014_RIDE_MIN;
      if (partiTot < 0) partiTot = 0;

      // Colore riga: verde se realtime, giallo se teorico
      uint16_t txtCol = deps[i].realtime ? TFT_GREEN : TFT_YELLOW;
      tft.setTextSize(1);
      tft.setTextColor(txtCol, COLOR_CARD);
      tft.setCursor(14, yi);
      char buf[52];
      snprintf(buf, sizeof(buf), "Parti %02d:%02d --> 2014 %02d:%02d --> Arr %02d:%02d",
               partiTot / 60, partiTot % 60,
               bH, bM,
               arrivoTot / 60, arrivoTot % 60);
      tft.print(buf);
    }
  }

  if (shown == 0) {
    tft.setTextColor(TFT_DARKGREY, COLOR_CARD);
    tft.setTextSize(1);
    tft.setCursor(20, rowY);
    tft.print("Nessun passaggio raggiungibile");
  }

  // Pulsante aggiorna (solo per bus)
  if (deps != nullptr) {
    tft.fillRoundRect(380, y0 + cardH - 28, 80, 22, 5, accentColor);
    tft.setTextColor(TFT_WHITE, accentColor);
    tft.setTextSize(1);
    tft.setCursor(395, y0 + cardH - 21);
    tft.print("Aggiorna");
  }
}

// Aggiorna solo i dati del bus (con feedback visivo)
void refreshBusData(int cardY) {
  // Mostra "caricamento..." nel pulsante
  tft.fillRoundRect(380, cardY + 100, 80, 22, 5, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  tft.setTextSize(1);
  tft.setCursor(388, cardY + 107);
  tft.print("...");

  lastActivity = millis(); // reset timeout durante la fetch

  busDataCount = fetchBus2014(busData, MAX_BUS_DEPARTURES);

  lastActivity = millis(); // reset dopo fetch
}

// Helper: ridisegna solo la card metro (usa festivoManuale)
void redrawMetroCard() {
  metroCount = getNextMetroDepartures(metroHours, metroMins, MAX_BUS_DEPARTURES);
  drawTragittoCard(
    38, COLOR_PURPLE,
    "Politecnico",
    "a piedi > Metro Bengasi > Vinzaglio > a piedi",
    metroHours, metroMins, metroCount,
    nullptr, 0,
    WALK_TO_METRO_MIN
  );
}

// Helper: ridisegna solo la legenda in fondo
void redrawLegend() {
  // Pulisce l'area legenda
  tft.fillRect(0, 293, 480, 27, 0x0010); // sfondo scurissimo (come gradiente basso)
  tft.setTextSize(1);
  tft.setTextColor(TFT_GREEN);
  tft.setCursor(15, 300);
  tft.print("\u25cf Real-time");
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(105, 300);
  tft.print("\u25cf Programmato");
}

// Disegna l'intera pagina pullman
void drawBusPage() {
  // Sfondo gradiente scuro (blu notte)
  drawGradientBackground(0x0010, 0x1020);

  // Titolo
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(2);
  tft.setCursor(190, 10);
  tft.print("Trasporti");

  // Badge feriale/festivo
  bool festivo = isFestivo();
  drawFerialeFestvoBadge(festivo);

  // -- Tragitto 1: Metro --
  metroCount = getNextMetroDepartures(metroHours, metroMins, MAX_BUS_DEPARTURES);
  drawTragittoCard(
    38, COLOR_PURPLE,
    "Politecnico",
    "a piedi > Metro Bengasi > Vinzaglio > a piedi",
    metroHours, metroMins, metroCount,
    nullptr, 0,
    WALK_TO_METRO_MIN
  );

  // -- Tragitto 2: Bus 2014 --
  busDataCount = fetchBus2014(busData, MAX_BUS_DEPARTURES);
  drawTragittoCard(
    178, COLOR_ORANGE,
    "Monta",
    "a piedi > Ferm.3064 > Bus 2014",
    nullptr, nullptr, 0,
    busData, busDataCount,
    WALK_TO_BUS_MIN
  );

  // Icona casetta (angolo in alto a sinistra)
  drawHouse(5, 5);

  // Legenda
  redrawLegend();
}

// ============================================================================
// LOGICA PAGINA  (chiamata da logic_pages.h)
// ============================================================================

void pageBus() {
  page = 4;
  lastActivity = millis();
  festivoManuale = isFestivo(); // inizializza con il giorno reale
  drawBusPage();
  lastActivity = millis();

  // Coordinate touch badge FERIALE/FESTIVO
  // Badge display: x 350-470, y 8-30
  // Touch: asse y invertito rispetto al display (y_touch = 320 - y_display)
  // => touch y: 290-312, touch x: 350-470
  const int badgeX1 = 340, badgeX2 = 470;
  const int badgeY1 = 290, badgeY2 = 320;

  // Y della card bus (per hit-test pulsante Aggiorna)
  const int busCardY = 178;
  const int aggiornaBtnX1 = 380, aggiornaBtnX2 = 460;
  const int aggiornaBtnY1 = busCardY + 100, aggiornaBtnY2 = busCardY + 122;

  delay(300);

  while (page == 4) {
    uint16_t x, y;
    checkInactivity();
    if (!tft.getTouch(&x, &y)) { delay(10); continue; }
    lastActivity = millis();

    // Casetta → home
    if (x < 60 && y > 260) {
      waitRelease();
      page = 0;
      disegnaHome();
      return;
    }

    // Badge FERIALE/FESTIVO → toggle modalità
    if (x >= badgeX1 && x <= badgeX2 && y >= badgeY1 && y <= badgeY2) {
      waitRelease();
      festivoManuale = !festivoManuale;
      // Ridisegna TUTTO per coerenza (badge, cards, legenda)
      drawFerialeFestvoBadge(festivoManuale);
      redrawMetroCard();
      // Ridisegna anche la card bus (che contiene il pulsante Aggiorna)
      drawTragittoCard(
        178, COLOR_ORANGE,
        "Monta",
        "a piedi > Ferm.3064 > Bus 2014",
        nullptr, nullptr, 0,
        busData, busDataCount,
        WALK_TO_BUS_MIN
      );
      redrawLegend();
      continue;
    }

    // Pulsante "Aggiorna" della card bus
    if (x >= aggiornaBtnX1 && x <= aggiornaBtnX2 &&
        y >= aggiornaBtnY1 && y <= aggiornaBtnY2) {
      waitRelease();
      refreshBusData(busCardY);
      drawTragittoCard(
        busCardY, COLOR_ORANGE,
        "Monta",
        "a piedi > Ferm.3064 > Bus 2014",
        nullptr, nullptr, 0,
        busData, busDataCount,
        WALK_TO_BUS_MIN
      );
      continue;
    }

    delay(10);
  }
}

#endif // BUS_SCHEDULE_H

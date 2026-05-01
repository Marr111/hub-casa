#ifndef TOUCH_CALIBRATION_H
#define TOUCH_CALIBRATION_H

#include "config.h"

// ============================================================================
// MAPPATURA TOUCH HARDWARE (calcolata dai dati RAW misurati)
// ============================================================================
// Dati RAW misurati fisicamente:
//   Alto-sinistra  -> RAW X: 3680, RAW Y: 328
//   Alto-destra    -> RAW X: 3536, RAW Y: 3839
//   Basso-sinistra -> RAW X: 520,  RAW Y: 402
//   Basso-destra   -> RAW X: 383,  RAW Y: 3904
//
// Conclusione: il sensore ha gli assi SCAMBIATI rispetto al display:
//   - RAW Y mappa la coordinata X dello schermo (0-480)
//   - RAW X mappa la coordinata Y dello schermo (0-320), ed è INVERTITO
//
// Valori limite usati per il mapping:
#define TOUCH_RAW_X_MAX 3680   // RAW X quando Y schermo = 0 (in alto)
#define TOUCH_RAW_X_MIN  383   // RAW X quando Y schermo = 320 (in basso)
#define TOUCH_RAW_Y_MIN  328   // RAW Y quando X schermo = 0 (a sinistra)
#define TOUCH_RAW_Y_MAX 3904   // RAW Y quando X schermo = 480 (a destra)

// Dimensioni schermo in landscape
#define SCREEN_W 480
#define SCREEN_H 320

// Soglia minima di pressione per considerare valido il tocco
// Valore alto (600) per eliminare ghost touch che causano flickering della home
#define TOUCH_PRESSURE_MIN 600

// ============================================================================
// FUNZIONE TOUCH PRINCIPALE (sostituisce tft.getTouch)
// Restituisce true e le coordinate schermo corrette se il tocco è valido.
// ============================================================================
bool getTouchMapped(uint16_t *sx, uint16_t *sy) {
  uint16_t rawX, rawY;
  uint16_t z = tft.getTouchRawZ(); // pressione

  if (z < TOUCH_PRESSURE_MIN) return false;
  if (!tft.getTouchRaw(&rawX, &rawY)) return false;

  // Clamp ai valori minimi/massimi per evitare overflow
  rawX = constrain(rawX, TOUCH_RAW_X_MIN, TOUCH_RAW_X_MAX);
  rawY = constrain(rawY, TOUCH_RAW_Y_MIN, TOUCH_RAW_Y_MAX);

  // RAW Y → coordinata X schermo (0-480, crescente verso destra)
  // NOTA: invertito (Y_MAX→0, Y_MIN→SCREEN_W) perché il sensore ha l'asse X specchiato
  *sx = map(rawY, TOUCH_RAW_Y_MAX, TOUCH_RAW_Y_MIN, 0, SCREEN_W);

  // RAW X → coordinata Y schermo (0-320, crescente verso il basso)
  // Nota: RAW X è INVERTITO (grande in alto, piccolo in basso)
  *sy = map(rawX, TOUCH_RAW_X_MAX, TOUCH_RAW_X_MIN, 0, SCREEN_H);

  return true;
}

// ============================================================================
// COMPATIBILITÀ: funzioni esistenti che chiamano tft.getTouch
// Le reindirizza a getTouchMapped senza toccare il resto del codice.
// ============================================================================

void load_touch_calibration() {
  // Nessuna calibrazione necessaria: usiamo getTouchMapped() direttamente.
  Serial.println("Touch: mappatura hardware custom attiva.");
}

TouchPoint touch_coordinate() {
  TouchPoint p = { 0, 0, false };
  if (getTouchMapped(&p.x, &p.y)) {
    lastActivity = millis();
    p.touched = true;
  }
  return p;
}

void touch_calibrate() {
  load_touch_calibration();
}

void force_touch_calibrate() {
  Serial.println("[FORCE] Calibrazione hardware custom: nessuna azione necessaria.");
  load_touch_calibration();

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(30, 130);
  tft.println("Calibrazione hardware OK!");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(80, 170);
  tft.println("Ritorno alle impostazioni...");
  delay(1500);
}



void test_touch() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 20);
  tft.println("TEST TOUCH MAPPATO");
  tft.setTextSize(1);
  tft.setCursor(20, 50);
  tft.println("Tocca lo schermo - vedi le coord schermo su seriale");
  tft.setCursor(20, 65);
  tft.println("Tocca angolo ALTO-SX per uscire (x<60, y<60)");

  drawHouse();

  uint16_t sx, sy;
  unsigned long lastPrint = 0;

  while (true) {
    if (getTouchMapped(&sx, &sy)) {
      if (millis() - lastPrint > 120) {
        Serial.printf("MAPPED -> X: %d, Y: %d\n", sx, sy);

        tft.fillRect(20, 120, 440, 120, TFT_BLACK);
        tft.setTextSize(3);
        tft.setTextColor(TFT_YELLOW);
        tft.setCursor(20, 130);
        tft.printf("X: %d", sx);
        tft.setCursor(20, 170);
        tft.printf("Y: %d", sy);

        // Disegna punto dove viene premuto
        tft.fillCircle(sx, sy, 5, TFT_RED);

        lastPrint = millis();

        // Uscita: angolo alto-sinistra
        if (sx < 60 && sy < 60) {
          Serial.println("Uscita test touch.");
          return;
        }
      }
    }
    delay(10);
  }
}

#endif
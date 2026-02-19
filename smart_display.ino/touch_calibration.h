#ifndef TOUCH_CALIBRATION_H
#define TOUCH_CALIBRATION_H

#include "config.h"

// ============================================================================
// GESTIONE TOUCH E CALIBRAZIONE
// ============================================================================

TouchPoint touch_coordinate() {
  TouchPoint p = { 0, 0, false };

  if (tft.getTouch(&p.x, &p.y)) {
    lastActivity = millis();
    p.touched = true;
  }

  return p;
}

void touch_calibrate() {
  uint16_t calData[5];
  uint8_t calDataOK = 0;

  // Controlla se esiste calibrazione
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

  // Esegui calibrazione
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

  // Salva calibrazione
  File f = SPIFFS.open(CALIBRATION_FILE, "w");
  if (f) {
    f.write((const unsigned char *)calData, 14);
    f.close();
    Serial.println("Calibrazione salvata");
  }

  // Conferma
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 140);
  tft.println("Calibrazione completata!");
  delay(1000);
}

// ============================================================================
// TEST TOUCH (per debugging)
// ============================================================================

void test_touch() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(3);
  tft.setCursor(50, 20);
  tft.println("TEST TOUCH");
  tft.setTextSize(2);
  tft.setCursor(20, 60);
  tft.println("Tocca lo schermo");
  tft.setCursor(20, 90);
  tft.println("Premi HOME per uscire");

  drawHouse();

  unsigned long lastPrint = 0;
  uint16_t lastX = 0, lastY = 0;

  while (1) {
    uint16_t x, y;

    if (tft.getTouch(&x, &y)) {
      if (millis() - lastPrint > 100 || abs(x - lastX) > 5 || abs(y - lastY) > 5) {
        Serial.print("Touch X: ");
        Serial.print(x);
        Serial.print(" | Y: ");
        Serial.println(y);

        tft.fillRect(0, 140, 480, 60, TFT_BLACK);
        tft.setTextSize(3);
        tft.setTextColor(TFT_YELLOW, TFT_BLACK);
        tft.setCursor(20, 150);
        tft.print("X: ");
        tft.print(x);
        tft.setCursor(20, 180);
        tft.print("Y: ");
        tft.print(y);

        tft.fillCircle(x, y, 3, TFT_RED);

        lastPrint = millis();
        lastX = x;
        lastY = y;
      }

      // Check HOME
      if (x < 60 && y < 60) {
        Serial.println("Uscita dal test touch");
        delay(500);
        return;
      }
    }

    delay(10);
  }
}

#endif
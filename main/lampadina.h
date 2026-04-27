#ifndef LAMPADINA_H
#define LAMPADINA_H

#include "config.h"
#include "gui_functions.h"

/**
 * Pagina di controllo della Lampadina RGB
 */
void pageLampadina() {
  page = 4;
  lastActivity = millis();

  auto drawLampUI = []() {
    tft.fillScreen(sfondo_page4);
    
    // Titolo
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.drawCentreString("CONTROLLO LUCE", 240, 20, 1);
    
    // Disegna la lampadina grande che cambia colore in base allo stato
    uint16_t bulbColor = led_acceso ? tft.color565(led_r, led_g, led_b) : TFT_DARKGREY;
    
    // Vetro lampadina
    tft.fillCircle(240, 120, 60, bulbColor);
    tft.drawCircle(240, 120, 62, TFT_WHITE);
    
    // Base lampadina
    tft.fillRect(220, 175, 40, 20, 0x7BEF); // Grigio metallico
    tft.drawRect(220, 175, 40, 20, TFT_WHITE);
    
    // Pulsante ON/OFF
    int btnY = 220;
    uint16_t btnColor = led_acceso ? TFT_RED : TFT_GREEN;
    const char* btnText = led_acceso ? "SPEGNI" : "ACCENDI";
    
    tft.fillRoundRect(160, btnY, 160, 50, 10, btnColor);
    tft.drawRoundRect(160, btnY, 160, 50, 10, TFT_WHITE);
    tft.setTextColor(TFT_WHITE);
    tft.setTextSize(3);
    tft.drawCentreString(btnText, 240, btnY + 12, 1);
    
    // Colori rapidi (se accesa)
    if (led_acceso) {
      int colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_WHITE};
      for (int i = 0; i < 5; i++) {
        tft.fillCircle(80 + i * 80, 300, 25, colors[i]);
        tft.drawCircle(80 + i * 80, 300, 27, TFT_WHITE);
      }
    }
    
    drawHouse();
  };

  drawLampUI();

  while (page == 4) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
      lastActivity = millis();
      
      // Tasto HOME
      if (x < HOME_BTN_X_MAX && y < HOME_BTN_Y_MAX) {
        waitRelease();
        page = 0;
        disegnaHome();
        return;
      }
      
      // Pulsante ON/OFF (160, 220, 160, 50)
      if (x > 160 && x < 320 && y > 220 && y < 270) {
        led_acceso = !led_acceso;
        waitRelease();
        drawLampUI();
      }
      
      // Selezione Colori (y intorno a 300)
      if (led_acceso && y > 270) {
        int colorIdx = -1;
        if (x > 50 && x < 110) colorIdx = 0; // Rosso
        else if (x > 130 && x < 190) colorIdx = 1; // Verde
        else if (x > 210 && x < 270) colorIdx = 2; // Blu
        else if (x > 290 && x < 350) colorIdx = 3; // Giallo
        else if (x > 370 && x < 430) colorIdx = 4; // Bianco
        
        if (colorIdx != -1) {
          uint16_t selectedColor;
          switch (colorIdx) {
            case 0: led_r = 255; led_g = 0;   led_b = 0;   break;
            case 1: led_r = 0;   led_g = 255; led_b = 0;   break;
            case 2: led_r = 0;   led_g = 0;   led_b = 255; break;
            case 3: led_r = 255; led_g = 255; led_b = 0;   break;
            case 4: led_r = 255; led_g = 255; led_b = 255; break;
          }
          waitRelease();
          drawLampUI();
        }
      }
    }
    checkInactivity();
    delay(10);
  }
}

#endif

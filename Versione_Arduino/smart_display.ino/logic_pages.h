#ifndef LOGIC_PAGES_H
#define LOGIC_PAGES_H

#include "config.h"

// ============================================================================
// GESTIONE PAGINE E NAVIGAZIONE
// ============================================================================

void page1() {
  tft.fillScreen(sfondo_page1);
  tft.setTextColor(TFT_WHITE, sfondo_page1);
  tft.setCursor(130, 30);
  tft.setTextSize(4);
  tft.println("Pagina 1");
  drawArrows();
  drawHouse();
  
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
  drawHouse();
  
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
  drawHouse();
  
  while (1) {
    cambio_pagina();
    checkInactivity();
  }
}

void page4() {
  tft.fillScreen(sfondo_page4);
  tft.setTextColor(TFT_WHITE, sfondo_page4);
  tft.setCursor(130, 30);
  tft.setTextSize(3);
  tft.println("LAMPADINA RGB BLUETOOTH");
  drawArrows();
  drawHouse();
  
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
  drawHouse();
  
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
  drawHouse();
  
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
  drawHouse();
  
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

  while (esci_dal_loop == 1) {
    if (stato_scroll_bar == 1) stato_scroll_bar1();
    else if (stato_scroll_bar == 2) stato_scroll_bar2();
    else if (stato_scroll_bar == 3) stato_scroll_bar3();
    delay(100);
  }
  
  loop();
  esci_dal_loop = 1;
}

void cambio_pagina() {
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    lastActivity = millis();

    // Ritorno home
    if (x < 420 && y < 40) {
      Serial.println("Ritorno al loop");
      tft.fillScreen(sfondo_page0);
      tft.setTextColor(TFT_WHITE, sfondo_page0);
      tft.setTextSize(5);
      tft.setCursor(60, 50);
      tft.println("Casa Casetta");
      tft.setTextSize(2);
      tft.setCursor(120, 300);
      tft.println("Premi per proseguire");
      loop();
    }

    // Freccia sinistra
    if (page > 1) {
      if (x > 20 && x < 60 && y > tft.height() / 2 - 30 && y < tft.height() / 2 + 30) {
        Serial.println("Freccia sinistra premuta");
        page = page - 1;
        Serial.print("Pagina ");
        Serial.println(page);
      }
    }

    // Freccia destra
    if (page < 7) {
      if (x > tft.width() - 60 && x < tft.width() - 20 && 
          y > tft.height() / 2 - 30 && y < tft.height() / 2 + 30) {
        Serial.println("Freccia destra premuta");
        page = page + 1;
        Serial.print("Pagina ");
        Serial.println(page);
      }
    }

    if (page != lastPage) {
      lastPage = page;
      switch (page) {
        case 1: page1(); break;
        case 2: page2(); break;
        case 3: page3(); break;
        case 4: page4(); break;
        case 5: page5(); break;
        case 6: page6(); break;
        case 7: page7(); break;
      }
    }
  }
}

void checkInactivity() {
  if ((millis() - lastActivity > INACTIVITY_TIMEOUT) && page != 0) {
    Serial.println("⏳ Timeout inattività! Ritorno a 🏠 Home (page0)");
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
    loop();
  }
}

// ============================================================================
// STATI SCROLL BAR
// ============================================================================

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
      // WiFi
      if (tp.x > 410 && tp.x < 430 && tp.y > 220 && tp.y < 250) {
        drawCaricamento(415, 90, 2);
        connessioneWiFi();
        drawCaricamento(415, 90, 2);
        tft.fillRect(400, 75, 31, 31, TFT_LIGHTGREY);
        drawCircleWithDot(415, 90, 15);
      } 
      // NTP
      else if (tp.x > 410 && tp.x < 430 && tp.y > 150 && tp.y < 180) {
        Serial.println("ricalibrazione ntp");
        drawCaricamento(415, 140, 2);
        connessioneNTP();
        drawCaricamento(415, 140, 2);
        tft.fillRect(400, 125, 31, 31, TFT_LIGHTGREY);
        drawCircleWithDot(415, 140, 15);
      } 
      // Touch
      else if (tp.x > 410 && tp.x < 430 && tp.y > 100 && tp.y < 130) {
        Serial.println("calibrazione touch");
        drawCaricamento(415, 190, 2);
        touch_calibrate();
        return;
      }
    }
    
    if (stato_scroll_bar != 1) {
      Serial.println("ho braikato");
      pageImpostazioni();
    }
  }
}

void stato_scroll_bar2() {
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
    if (stato_scroll_bar != 3) {
      Serial.println("ho braikato");
      pageImpostazioni();
    }
  }
}

int touchMenu(int stato_scroll_bar) {
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    // Freccia su
    if (x > 440 && y < 25) {
      if (stato_scroll_bar < 3) {
        Serial.println(stato_scroll_bar);
        stato_scroll_bar++;
        Serial.println("⬇️ Scroll DOWN");
      }
    }

    // Freccia giù
    if (x > 440 && y > 270) {
      if (stato_scroll_bar > 1) {
        stato_scroll_bar--;
        Serial.println("⬆️ Scroll UP");
        Serial.println(stato_scroll_bar);
      }
    }
  }

  return stato_scroll_bar;
}

#endif
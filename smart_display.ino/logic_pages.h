#ifndef LOGIC_PAGES_H
#define LOGIC_PAGES_H

#include "config.h"

// ============================================================================
// GESTIONE PAGINE E NAVIGAZIONE
// ============================================================================

void waitRelease() {
  uint16_t x, y;
  while (tft.getTouch(&x, &y)) { delay(10); }
  delay(100);
}

void page1() {
  tft.fillScreen(sfondo_page1);
  tft.setTextColor(TFT_WHITE, sfondo_page1);
  tft.setCursor(130, 30);
  tft.setTextSize(4);
  tft.println("Pagina 1");
  drawArrows();
  drawHouse();

  while (page == 1) {
    cambio_pagina();  // Gestisce solo il cambio della variabile 'page'
    checkInactivity();
    printEventsTFT();
    delay(10);
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

  while (page == 2) {
    cambio_pagina();  // Gestisce solo il cambio della variabile 'page'
    checkInactivity();
    printTasksTFT();
    delay(10);
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

  while (page == 3) {
    cambio_pagina();  // Gestisce solo il cambio della variabile 'page'
    checkInactivity();
    delay(10);
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

  while (page == 4) {
    cambio_pagina();  // Gestisce solo il cambio della variabile 'page'
    checkInactivity();
    delay(10);
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

  while (page == 5) {
    cambio_pagina();  // Gestisce solo il cambio della variabile 'page'
    checkInactivity();
    delay(10);
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

  while (page == 6) {
    cambio_pagina();  // Gestisce solo il cambio della variabile 'page'
    checkInactivity();
    delay(10);
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

  while (page == 7) {
    cambio_pagina();  // Gestisce solo il cambio della variabile 'page'
    checkInactivity();
    delay(10);
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

  waitRelease();  // Debounce iniziale

  for (int y = 70; y < 320; y = y + 50) {
    tft.fillRoundRect(15, y, 420, 40, 15, TFT_LIGHTGREY);
  }

  esci_dal_loop = 1;
  while (esci_dal_loop == 1) {
    if (stato_scroll_bar == 1) stato_scroll_bar1();
    else if (stato_scroll_bar == 2) stato_scroll_bar2();
    else if (stato_scroll_bar == 3) stato_scroll_bar3();

    uint16_t tx, ty;
    if (tft.getTouch(&tx, &ty)) {
      // Se premi la casetta (coordinate 0-60)
      if (tx < 60 && ty < 60) {
        Serial.println("Uscita da Impostazioni");
        page = 0;
        esci_dal_loop = 0;
      }
    }
    delay(10);
  }
  waitRelease();
}

void cambio_pagina() {
  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {
    lastActivity = millis();

    // Tasto HOME
    if (x < 60 && y < 420) {
      disegnaHome();
      page = 0;
      delay(200);
    }

    // FRECCIA DESTRA (Esempio coordinate)
    if (x > 400 && y > 100 && y < 200) {
      if (page < 7) {
        page++;
        delay(200);
      }
    }

    // FRECCIA SINISTRA
    if (x < 80 && y > 100 && y < 200) {
      if (page > 1) {
        page--;
        delay(200);
      }
    }
  }
}

// Funzione di smistamento per evitare ricorsione in cambio_pagina
void switchPage(int p) {
  switch (p) {
    case 0:
      disegnaHome();  // Disegna e gestisce il touch della home
      break;
    case 1: page1(); break;
    case 2: page2(); break;
    case 3: page3(); break;
    case 4: page4(); break;
    case 5: page5(); break;
    case 6: page6(); break;
    case 7: page7(); break;
  }
}

void checkInactivity() {
  if ((millis() - lastActivity > INACTIVITY_TIMEOUT) && page != 0) {
    Serial.println("⏳ Timeout!");
    disegnaHome();
    page = 0;                 // Il loop principale si accorgerà del cambio e disegnerà la home
    lastActivity = millis();  // Resetta per evitare loop continui
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

  while (stato_scroll_bar == 1) {
    stato_scroll_bar = touchMenu(stato_scroll_bar);
    TouchPoint tp = touch_coordinate();
    checkInactivity();

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
        stato_scroll_bar1();
      } else if (tp.x > 60 && tp.y < 60) {  //ritorno alla home
        page = 0;
        esci_dal_loop = 0;
        return;
      }
    }
    if (stato_scroll_bar != 1) return;
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
    checkInactivity();
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
    checkInactivity();
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
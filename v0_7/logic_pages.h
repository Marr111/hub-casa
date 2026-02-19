#ifndef LOGIC_PAGES_H
#define LOGIC_PAGES_H

#include "config.h"

// ============================================================================
// GESTIONE PAGINE E NAVIGAZIONE
// ============================================================================

void updateLoadingScreen(int percent, const char* message) {
  // Parametri grafici
  int barX = 40;
  int barY = 150; // Al centro vert. (su 320px)
  int barW = 400; // Larghezza barra (su 480px)
  int barH = 20;
  
  // 1. Disegna il contorno della barra (solo la prima volta o sempre)
  tft.drawRect(barX, barY, barW, barH, TFT_WHITE);

  // 2. Calcola quanto riempire
  int fillW = map(percent, 0, 100, 0, barW - 4); // -4 per il bordo interno
  
  // 3. Riempi la barra (effetto progresso)
  // Lasciamo 2px di padding interno
  tft.fillRect(barX + 2, barY + 2, fillW, barH - 4, 0x07E0); // Verde brillante o altro colore
  
  // 4. Gestione del Testo (Messaggio sotto la barra)
  // Pulisci SOLO l'area del testo per evitare sovrapposizioni
  tft.fillRect(0, barY + 30, 480, 30, 0x0000); // Usa lo stesso colore di sfondo del gradiente in quel punto!
  
  tft.setTextDatum(MC_DATUM); // Allinea al centro
  tft.setTextColor(TFT_WHITE); // Niente sfondo, perché abbiamo pulito l'area col fillRect sopra
  tft.drawString(message, 240, barY + 45); // Scrive al centro (480/2 = 240)
}

void waitRelease() {
  uint16_t x, y;
  while (tft.getTouch(&x, &y)) {
    delay(10);
  }
  delay(100);
}

void pageprincipale() {
  // sfondo gradiente: da Blu scuro/Nero petrolio al Blu acciaio/Grigio azzurro
  drawGradientBackground(0x1926, 0x4B92);
  dbgLog("logic_pages.h:pageprincipale", "enter", page, 0, 0, 0);

  // Disegna la griglia da cui scegliere le pagine
  disegnaGrigliaHome();

  delay(500);
  
  bool running = true;
  while (running) {
    uint16_t x, y;
    if (!tft.getTouch(&x, &y)) {
      continue;
    }
    dbgLog("logic_pages.h:pageprincipale", "touch", x, y, 0, 0);

    // Tasto HOME (angolo in alto a sinistra)
    if (x < 60 && y > 236) {
      waitRelease();
      page = 0;
      dbgLog("logic_pages.h:pageprincipale", "go.home", x, y, page, 0);
      disegnaHome();
      break;
    }

    int col = x / 160;  // 480 / 3 = 160
    // Controllo colonne valido
    if (col < 0 || col > 2) {
      continue;
    }

    int row = 0;

    if (y < 70) {
      dbgLog("logic_pages.h:pageprincipale", "grid.header", y, col, 0, 0);
      row = 2; // parte sotto dello schermo
    } else if (y < 153) {
      row = 1;
    } else if (y < 236) {
      row = 0;  
    } else {// parte sopra dello schermo
      continue;
    }

    dbgLog("logic_pages.h:pageprincipale", "grid", row, col, 0, 0);
    // Indice 1..9: 1 in basso a sinistra, 9 in alto a destra
    int idx = row * 3 + col + 1;

    waitRelease();

    switch (idx) {
      case 1:
        page = 1;
        dbgLog("logic_pages.h:pageprincipale", "go.pageCalendario", row, col, page, 0);
        pageCalendario();
        running = false;
        break;
      case 2:
        page = 2;
        dbgLog("logic_pages.h:pageprincipale", "go.pageTask", row, col, page, 0);
        pageTask();
        running = false;
        break;
      case 3:
        page = 3;
        dbgLog("logic_pages.h:pageprincipale", "go.page3", row, col, page, 0);
        page3();
        running = false;
        break;
      case 4:
        page = 4;
        dbgLog("logic_pages.h:pageprincipale", "go.page4", row, col, page, 0);
        page4();
        running = false;
        break;
      case 5:
        page = 5;
        dbgLog("logic_pages.h:pageprincipale", "go.page5", row, col, page, 0);
        page5();
        running = false;
        break;
      case 6:
        page = 6;
        dbgLog("logic_pages.h:pageprincipale", "go.page6", row, col, page, 0);
        page6();
        running = false;
        break;
      case 7:
        page = 7;
        dbgLog("logic_pages.h:pageprincipale", "go.page7", row, col, page, 0);
        page7();
        running = false;
        break;
      case 8:
        page = 8;
        dbgLog("logic_pages.h:pageprincipale", "go.page8", row, col, page, 0);
        page8();
        running = false;
        break;
      case 9:
        page = 9;
        dbgLog("logic_pages.h:pageprincipale", "go.settings", row, col, page, 0);
        pageImpostazioni();
        running = false;
        break;
      default:
        // Non dovrebbe mai capitare perché row/col sono limitati
        dbgLog("logic_pages.h:pageprincipale", "idx.unexpected", idx, row, col, 0);
        break;
    }
  }
}

void pageCalendario() {
  tft.fillScreen(sfondo_pageCalendario);
  tft.setTextColor(TFT_WHITE, sfondo_pageCalendario);
  tft.setCursor(130, 30);
  tft.setTextSize(4);
  tft.println("Pagina 1");
  drawHouse();

  while (page == 1) {
    uint16_t x, y;
    checkInactivity();
    printEventsTFT();
    delay(10);
    if (tft.getTouch(&x, &y)) {
      if (x < 40 && y > 300) {  // Area casetta
        break;
      }
    }
  }
}

void pageTask() {
  tft.fillScreen(sfondo_pageTask);
  tft.setTextColor(TFT_WHITE, sfondo_pageTask);
  printTasksTFT();
  tft.setCursor(130, 30);
  tft.setTextSize(4);
  tft.println("Pagina 2");
  drawHouse();

  while (page == 2) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
      if (x < 40 && y > 300) {  // Area casetta
        break;
      }
    }
    checkInactivity();
    printTasksTFT();
    delay(10);
  }
}

void page3() {
  tft.fillScreen(sfondo_page3);
  tft.setTextColor(TFT_WHITE, sfondo_page3);
  tft.setCursor(130, 30);
  tft.setTextSize(4);
  tft.println("Pagina 3");
  drawHouse();

  while (page == 3) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
      if (x < 40 && y > 300) {  // Area casetta
        break;
      }
    }
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
  drawHouse();

  while (page == 4) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
      if (x < 40 && y > 300) {  // Area casetta
        break;
      }
    }
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
  drawHouse();

  while (page == 5) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
      if (x < 40 && y > 300) {  // Area casetta
        break;
      }
    }
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
  drawHouse();

  while (page == 6) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
      if (x < 40 && y > 300) {  // Area casetta
        break;
      }
    }
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
  drawHouse();

  while (page == 7) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
      if (x < 40 && y > 300) {  // Area casetta
        break;
      }
    }
    checkInactivity();
    delay(10);
  }
}

void page8() {
  tft.fillScreen(sfondo_page7);
  tft.setCursor(130, 30);
  tft.setTextColor(TFT_WHITE, sfondo_page7);
  tft.setTextSize(4);
  tft.println("Pagina 8");
  drawHouse();

  while (page == 8) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
      if (x < 40 && y > 300) {  // Area casetta
        break;
      }
    }
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
    if (page == 0) break;

    if (stato_scroll_bar == 1)
      stato_scroll_bar1();
    else if (stato_scroll_bar == 2)
      stato_scroll_bar2();
    else if (stato_scroll_bar == 3)
      stato_scroll_bar3();

    uint16_t tx, ty;
    if (tft.getTouch(&tx, &ty)) {
      // Se premi la casetta (coordinate 0-60)
      if (tx > 460 && ty < 40) {  // Area casetta
        Serial.println("Uscita da Impostazioni");
        page = 0;
        esci_dal_loop = 0;
      }
    }
    delay(10);
  }
  waitRelease();

  // Se siamo usciti impostando page = 0, ridisegna la home
  if (page == 0) {
    disegnaHome();
  }
}

void checkInactivity() {
  if ((millis() - lastActivity > INACTIVITY_TIMEOUT) && page != 0) {
    Serial.println("⏳ Timeout!");
    disegnaHome();
    page = 0;                 // Il loop principale si accorgerà del cambio e disegnerà la
    // home
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
  tft.println("4 Aggiornamento codice");
  drawCircleWithDot(415, 190, 15);
  tft.setCursor(30, 280);
  tft.println("5");

  while (stato_scroll_bar == 1) {
    stato_scroll_bar = touchMenu(stato_scroll_bar);
    TouchPoint tp = touch_coordinate();
    
    checkInactivity();
    if (page == 0) return;

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
      }
      //aggiornamento codice
      else if (tp.x > 410 && tp.x < 430 && tp.y > 50 && tp.y < 80){
        Serial.println("cerca nuovo codice");
        drawCaricamento(415, 190, 2);
        if (lastVersionCheck == 0 || (millis() - lastVersionCheck > VERSION_CHECK_INTERVAL)) {//nuovo codice
          checkForUpdate();
          if (updateAvailable) {
            performOTAUpdate(); // Rimuovi questa riga se vuoi aggiornamento manuale
          }
        }
        stato_scroll_bar1();
      } else if (tp.x < 60 && tp.y < 60) {  // ritorno alla home
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
    if (page == 0) return;

    stato_scroll_bar = touchMenu(stato_scroll_bar);
    if (stato_scroll_bar != 2) {
      Serial.println("Cambio pagina scroll 2->altrove");
      return;
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
    if (page == 0) return;

    stato_scroll_bar = touchMenu(stato_scroll_bar);
    if (stato_scroll_bar != 3) {
      Serial.println("Cambio pagina scroll 3->altrove");
      return;
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
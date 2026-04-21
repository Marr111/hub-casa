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
    lastActivity = millis();
    delay(10);
  }
  delay(100);
}

void pageprincipale() {
  page = 10;
  lastActivity = millis();
  // sfondo gradiente: da Blu scuro/Nero petrolio al Blu acciaio/Grigio azzurro
  drawGradientBackground(0x1926, 0x4B92);

  // Disegna la griglia da cui scegliere le pagine
  disegnaGrigliaHome();
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(110, 30);
  tft.setTextSize(3);
  tft.println("Scegli una pagina");
  delay(500);
  
  bool running = true;
  while (running) {
    checkInactivity();
    if (page == 0) return;

    uint16_t x, y;
    if (!tft.getTouch(&x, &y)) {
      delay(10);
      continue;
    }
    lastActivity = millis();
    // Tasto HOME (angolo in alto a sinistra)
    if (x < 60 && y > 236) {
      waitRelease();
      page = 0;
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
      row = 2; // parte sotto dello schermo
    } else if (y < 153) {
      row = 1;
    } else if (y < 236) {
      row = 0;  
    } else {// parte sopra dello schermo
      continue;
    }

    // Indice 1..9: 1 in basso a sinistra, 9 in alto a destra
    int idx = row * 3 + col + 1;

    waitRelease();

    switch (idx) {
      case 1:
        page = 1;
        pageCalendario();
        running = false;
        break;
      case 2:
        page = 2;
        pageTask();
        running = false;
        break;
      case 3:
        page = 3;
        page3();
        running = false;
        break;
      case 4:
        page = 4;
        page4();
        running = false;
        break;
      case 5:
        page = 5;
        pageBus();
        running = false;
        break;
      case 6:
        page = 6;
        page6();
        running = false;
        break;
      case 7:
        page = 7;
        page7();
        running = false;
        break;
      case 8:
        page = 8;
        page8();
        running = false;
        break;
      case 9:
        page = 9;
        pageImpostazioni();
        running = false;
        break;
      default:
        break;
    }
  }
}

// Aggiungi la definizione esterna per la nuova variabile
extern int eventiPageIndex;

void pageCalendario() {
  int weekOffset = 0;  // 0 = settimana corrente

  drawWeekView(weekOffset);
  delay(300);

  int state = 0;  // 0=vista settimanale, 1=dettaglio giorno
  int selDay = -1, selMonth = -1, selYear = -1;

  while (page == 1) {
    uint16_t x, y;
    checkInactivity();
    if (!tft.getTouch(&x, &y)) { delay(10); continue; }
    lastActivity = millis();

    if (state == 1) {
      // --- Vista dettaglio giorno ---
      // Bottone "< Cal." in alto a destra: touch x>380 && y>280
      if (x > 380 && y > 280) {
        waitRelease();
        drawWeekView(weekOffset);
        state = 0; selDay = -1;
        continue;
      }
      // Casetta → home
      if (x < 60 && y > 260) {
        waitRelease();
        page = 0; disegnaHome(); return;
      }
    } else {
      // --- Vista settimanale ---
      // Freccia sinistra ◀ (controlla prima della casetta per evitare overlap)
      if (weekArrowLeftTouched(x, y)) {
        if (weekCanGoLeft(weekOffset)) {
          waitRelease();
          weekOffset--;
          drawWeekView(weekOffset);
        }
        continue;
      }
      // Freccia destra ▶
      if (weekArrowRightTouched(x, y)) {
        if (weekCanGoRight(weekOffset)) {
          waitRelease();
          weekOffset++;
          drawWeekView(weekOffset);
        }
        continue;
      }
      // Casetta → home
      if (x < 60 && y > 260) {
        waitRelease();
        page = 0; disegnaHome(); return;
      }
      // Tap su un giorno della settimana
      if (calGetTouchedDayWeek(x, y, weekOffset, &selDay, &selMonth, &selYear)) {
        waitRelease();
        drawDayDetail(selDay, selMonth, selYear);
        state = 1;
      }
    }
    delay(10);
  }
}

void pageTask() {
  pageIndex = 0;  // Sempre dalla prima pagina quando si entra
  printTasksTFT();
  delay(300);

  while (page == 2) {
    uint16_t x, y;
    checkInactivity();
    if (!tft.getTouch(&x, &y)) { delay(10); continue; }
    lastActivity = millis();

    // Casetta → home (angolo alto-sinistra: touch x<60 && y>260 con Y invertita)
    if (x < 60 && y > 260) {
      waitRelease();
      pageIndex = 0;
      page = 0;
      disegnaHome();
      return;
    }

    // Barra navigazione (y < 40 con Y invertita = area in basso allo schermo)
    if (y < 40) {
      bool hasNext = ((pageIndex + 1) * TASKS_PER_PAGE < tasksCount);
      bool hasPrev = (pageIndex > 0);

      // Pulsante SUCC (destra: touch x > 340)
      if (hasNext && x > 340) {
        waitRelease();
        pageIndex++;
        printTasksTFT();
        continue;
      }
      // Pulsante PREC (sinistra: touch x < 140)
      if (hasPrev && x < 140) {
        waitRelease();
        pageIndex--;
        printTasksTFT();
        continue;
      }
    }

    delay(10);
  }
}


void page3() {
  page = 3;
  lastActivity = millis();  // Reset timer: la fetch HTTP può durare 10-20s
  drawWeatherPage();  // Disegna sfondo + titolo + 4 colonne meteo
  lastActivity = millis();  // Reset di nuovo dopo la fetch, prima del loop

  while (page == 3) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
      lastActivity = millis();
      if (x < 60 && y > 260) {  // Area casetta
        waitRelease();
        page = 0;
        disegnaHome();
        return;
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
      lastActivity = millis();
      if (x < 60 && y > 260) {  // Area casetta
        waitRelease();
        page = 0;
        disegnaHome();
        return;
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
      lastActivity = millis();
      if (x < 60 && y > 260) {  // Area casetta
        waitRelease();
        page = 0;
        disegnaHome();
        return;
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
      lastActivity = millis();
      if (x < 60 && y > 260) {  // Area casetta
        waitRelease();
        page = 0;
        disegnaHome();
        return;
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
      lastActivity = millis();
      if (x < 60 && y > 260) {  // Area casetta
        waitRelease();
        page = 0;
        disegnaHome();
        return;
      }
    }
    checkInactivity();
    delay(10);
  }
}

void page8() {
  tft.fillScreen(sfondo_page8);
  tft.setCursor(130, 30);
  tft.setTextColor(TFT_WHITE, sfondo_page8);
  tft.setTextSize(4);
  tft.println("Pagina 8");
  drawHouse();

  while (page == 8) {
    uint16_t x, y;
    if (tft.getTouch(&x, &y)) {
      lastActivity = millis();
      if (x < 60 && y > 260) {  // Area casetta
        waitRelease();
        page = 0;
        disegnaHome();
        return;
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
    checkInactivity();
    if (page == 0) break;

    if (stato_scroll_bar == 1)
      stato_scroll_bar1();
    else if (stato_scroll_bar == 2)
      stato_scroll_bar2();
    else if (stato_scroll_bar == 3)
      stato_scroll_bar3();

    uint16_t tx, ty;
    if (tft.getTouch(&tx, &ty)) {
      lastActivity = millis();
      // Se premi la casetta (alto-sinistra: touch X piccolo, touch Y grande per Y invertito)
      if (tx < 60 && ty > 260) {  // Area casetta
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
// PAGINA INFO DISPOSITIVO
// ============================================================================

void pageInfoDispositivo() {
  // Sfondo scuro
  tft.fillScreen(0x0820);

  // Titolo centrato
  tft.setTextColor(TFT_CYAN);
  tft.setTextSize(3);
  tft.drawCentreString("Info Dispositivo", 240, 15, 1);

  tft.drawFastHLine(0, 52, 480, TFT_DARKGREY);

  // --- Chip ---
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(20, 66);
  tft.println("Chip");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 90);
  tft.print("Modello: "); tft.println("ESP32-S3");
  tft.setCursor(20, 112);
  tft.print("Frequenza: "); tft.print(ESP.getCpuFreqMHz()); tft.println(" MHz");
  tft.setCursor(20, 134);
  tft.print("Flash: "); tft.print(ESP.getFlashChipSize() / 1024 / 1024); tft.println(" MB");
  tft.setCursor(20, 156);
  tft.print("RAM libera: "); tft.print(ESP.getFreeHeap() / 1024); tft.println(" KB");

  tft.drawFastHLine(0, 178, 480, TFT_DARKGREY);

  // --- Schermo ---
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(20, 188);
  tft.println("Schermo");
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(20, 210);
  tft.println("Driver: ST7796 (TFT_eSPI)");
  tft.setCursor(20, 232);
  tft.println("Risoluzione: 480 x 320");

  tft.drawFastHLine(0, 254, 480, TFT_DARKGREY);

  // --- Uptime (aggiornato in loop) ---
  tft.setTextSize(2);
  tft.setTextColor(TFT_YELLOW);
  tft.setCursor(20, 264);
  tft.println("Uptime");

  // Casetta home (angolo in alto a sinistra)
  drawHouse(5, 20);

  // Loop: aggiorna l'uptime ogni secondo e aspetta tocco
  unsigned long lastDraw = 0;
  while (true) {
    checkInactivity();
    if (page == 0) return;

    // Ridisegna uptime ogni secondo
    if (millis() - lastDraw >= 1000) {
      lastDraw = millis();
      unsigned long sec  = millis() / 1000;
      unsigned long mins = sec / 60;
      unsigned long hrs  = mins / 60;
      unsigned long days = hrs / 24;
      char buf[32];
      snprintf(buf, sizeof(buf), "%lud %02lu:%02lu:%02lu ", days, hrs % 24, mins % 60, sec % 60);
      tft.fillRect(20, 284, 350, 16, 0x0820);
      tft.setTextColor(TFT_WHITE, 0x0820);
      tft.setTextSize(2);
      tft.setCursor(20, 284);
      tft.print(buf);
    }

    uint16_t tx, ty;
    if (tft.getTouch(&tx, &ty)) {
      lastActivity = millis();
      // Casetta → home (angolo in alto a sinistra: touch x piccolo, y grande per Y invertito)
      if (tx < 60 && ty > 260) {
        waitRelease();
        page = 0;
        disegnaHome();
        return;
      }
    }
    delay(20);
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
  tft.println("4 Agg. codice");
  drawCircleWithDot(415, 240, 15);
  tft.setCursor(30, 280);
  tft.println("5 Info dispositivo");
  drawCircleWithDot(415, 290, 15);

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
        // Nessuna chiamata ricorsiva: il while esterno ridisegnerà lo stato corretto
      }
      //aggiornamento codice
      else if (tp.x > 410 && tp.x < 430 && tp.y > 50 && tp.y < 100){
        Serial.println("cerca nuovo codice");
        drawCaricamento(415, 240, 2);
        if (lastVersionCheck == 0 || (millis() - lastVersionCheck > VERSION_CHECK_INTERVAL)) {//nuovo codice
          checkForUpdate();
          if (updateAvailable) {
            performOTAUpdate(); // Rimuovi questa riga se vuoi aggiornamento manuale
          }
        }
        // Nessuna chiamata ricorsiva: il while esterno ridisegnerà lo stato corretto
      } else if (tp.x < 60 && tp.y > 260) {  // ritorno alla home (touch Y invertito)
        page = 0;
        esci_dal_loop = 0;
        return;
      }
      // Info dispositivo (pulsante 5, y display ~280-300 → touch y ~20-40)
      else if (tp.x > 410 && tp.x < 430 && tp.y > 10 && tp.y < 50) {
        pageInfoDispositivo();
        // Al ritorno ridisegna lo stato corrente
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
    lastActivity = millis();
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
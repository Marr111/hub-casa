#ifndef CALENDAR_WEEK_H
#define CALENDAR_WEEK_H

#include "config.h"

// ============================================================================
// CALENDARIO GRAFICO - vista settimanale
// ============================================================================

// Costanti layout vista settimanale
static const int WK_HDR_H  = 40;   // altezza header (frecce + nome settimana)
static const int WK_DN_H   = 18;   // riga nomi giorni
static const int WK_TOP    = WK_HDR_H + WK_DN_H; // 58
static const int WK_COLS   = 7;
static const int WK_CW     = 480 / WK_COLS; // 68
static const int WK_CH     = (320 - WK_TOP); // 262 — unica riga alta per gli eventi

// Aggiorna questa al numero massimo di eventi mostrati per colonna nella vista sett.
static const int WK_MAX_EVENTS_VISIBLE = 5;

// Restituisce il time_t di mezzanotte locale del lunedì della settimana
// che contiene il giorno d+weekOffset*7 a partire da oggi.
time_t wkGetMonday(int weekOffset) {
  time_t now2 = time(nullptr);
  struct tm lt; localtime_r(&now2, &lt);
  // Azzera ore/min/sec → mezzanotte di oggi
  lt.tm_hour = 0; lt.tm_min = 0; lt.tm_sec = 0; lt.tm_isdst = -1;
  time_t todayMidnight = mktime(&lt);
  // Giorno della settimana (0=Dom..6=Sab), vogliamo 0=Lun
  int wday = (lt.tm_wday + 6) % 7; // 0=Lun..6=Dom
  // Lunedì di questa settimana
  time_t monday = todayMidnight - (time_t)wday * 86400;
  // Lunedì della settimana richiesta
  return monday + (time_t)weekOffset * 7 * 86400;
}

bool weekCanGoLeft(int weekOffset) {
  // Può andare a sinistra se weekOffset > 0 (non mostriamo settimane passate)
  return weekOffset > 0;
}

bool weekCanGoRight(int weekOffset) {
  // Può andare a destra se il lunedì della settimana successiva è entro oggi+30gg
  time_t limit = time(nullptr) + 30 * 86400;
  time_t nextMonday = wkGetMonday(weekOffset + 1);
  return nextMonday <= limit;
}

// Disegna la freccia sinistra (◀) nell'header
// Posizionata a x=43..68 (a destra del piccolo spazio casetta) per non sovrapporsi
void drawArrowLeft(bool enabled) {
  uint16_t col = enabled ? TFT_WHITE : 0x39C7;
  // Punta a sinistra: vertice sx in x=43, base destra in x=68
  tft.fillTriangle(43, 20, 68, 6, 68, 34, col);
}

// Disegna la freccia destra (▶) nell'header
// Posizionata a x=412..437 (a sinistra del bordo)
void drawArrowRight(bool enabled) {
  uint16_t col = enabled ? TFT_WHITE : 0x39C7;
  tft.fillTriangle(437, 20, 412, 6, 412, 34, col);
}

bool weekArrowLeftTouched(uint16_t tx, uint16_t ty) {
  // touch x in 40..90, touch y > 275 (Y invertita: header grafico = grande Y touch)
  return (tx >= 40 && tx <= 90 && ty > 275);
}

bool weekArrowRightTouched(uint16_t tx, uint16_t ty) {
  // touch x > 390, touch y > 275
  return (tx > 390 && ty > 275);
}

void drawWeekView(int weekOffset) {
  tft.fillScreen(0x0821);

  // ---- Header ----
  tft.fillRect(0, 0, 480, WK_HDR_H, 0x1082);

  // Calcola lunedì e domenica della settimana
  time_t monday = wkGetMonday(weekOffset);
  time_t sunday = monday + 6 * 86400;
  struct tm tmMon, tmSun;
  localtime_r(&monday, &tmMon);
  localtime_r(&sunday, &tmSun);
  const char* mNS[] = {"Gen","Feb","Mar","Apr","Mag","Giu","Lug","Ago","Set","Ott","Nov","Dic"};
  char weekLabel[32];
  if (tmMon.tm_mon == tmSun.tm_mon) {
    sprintf(weekLabel, "%d - %d %s %d",
            tmMon.tm_mday, tmSun.tm_mday,
            mNS[tmMon.tm_mon], tmMon.tm_year + 1900);
  } else {
    sprintf(weekLabel, "%d %s - %d %s",
            tmMon.tm_mday, mNS[tmMon.tm_mon],
            tmSun.tm_mday, mNS[tmSun.tm_mon]);
  }

  // Frecce
  drawArrowLeft(weekCanGoLeft(weekOffset));
  drawArrowRight(weekCanGoRight(weekOffset));

  // Nome settimana centrato (lascia spazio per frecce e casa)
  tft.setTextSize(2); tft.setTextColor(TFT_WHITE);
  int lblW = strlen(weekLabel) * 12; // ~12px per char a size 2
  // Centra tra x=75 e x=405 per non coprire le frecce
  tft.setCursor(240 - lblW / 2, 12);
  tft.print(weekLabel);

  // Piccola icona casetta nell'angolo in alto a sinistra (home button)
  drawHouse(2, 6);

  // ---- Riga nomi giorni ----
  tft.fillRect(0, WK_HDR_H, 480, WK_DN_H, 0x10A2);
  const char* dn[] = {"L","M","M","G","V","S","D"};
  for (int c = 0; c < WK_COLS; c++) {
    tft.setTextSize(1);
    tft.setTextColor(c >= 5 ? 0xF800 : 0x7BCF);
    tft.setCursor(c * WK_CW + WK_CW / 2 - 3, WK_HDR_H + 4);
    tft.print(dn[c]);
  }

  // ---- 7 colonne giorni ----
  getLocalTime(&timeinfo);
  int todD = timeinfo.tm_mday, todM = timeinfo.tm_mon + 1, todY = timeinfo.tm_year + 1900;
  time_t nowT = time(nullptr);
  time_t limit = nowT + 30 * 86400;

  static const uint16_t EP[] = {0xFFE0, 0x07FF, 0xFD20, 0xF81F, 0x07E0}; // giallo, ciano, arancione, magenta, verde

  int evtIdx[8];
  for (int c = 0; c < WK_COLS; c++) {
    time_t dayT = monday + (time_t)c * 86400;
    struct tm tmD; localtime_r(&dayT, &tmD);
    int d = tmD.tm_mday, m = tmD.tm_mon + 1, y2 = tmD.tm_year + 1900;

    int cx = c * WK_CW;
    int cy = WK_TOP;

    // Fuori finestra 30 giorni?
    bool outOfRange = (dayT > limit);
    bool isToday = (d == todD && m == todM && y2 == todY);

    // Bordo colonna
    tft.drawRect(cx, cy, WK_CW, WK_CH, 0x2104);

    // Sfondo colonna
    uint16_t bg;
    if (outOfRange) bg = 0x0410;       // molto scuro, fuori limite
    else if (isToday) bg = 0x0228;     // blu accent oggi
    else bg = 0x0841;                   // normale

    int n = outOfRange ? 0 : calGetEventsForDay(d, m, y2, evtIdx, 8);
    if (!outOfRange && n > 0 && !isToday) bg = 0x1143;
    tft.fillRect(cx + 1, cy + 1, WK_CW - 2, WK_CH - 2, bg);

    // Bordo oggi
    if (isToday) {
      tft.drawRect(cx, cy, WK_CW, WK_CH, TFT_CYAN);
      tft.drawRect(cx + 1, cy + 1, WK_CW - 2, WK_CH - 2, TFT_CYAN);
    }

    // Numero giorno
    tft.setTextSize(1);
    uint16_t numCol;
    if (outOfRange) numCol = 0x39C7;
    else if (isToday) numCol = TFT_CYAN;
    else if (c >= 5) numCol = 0xFB00;  // rosso sabato/domenica
    else numCol = TFT_WHITE;

    tft.setTextColor(numCol);
    char ds[4]; sprintf(ds, "%d", d);
    tft.setCursor(cx + 3, cy + 3);
    tft.print(ds);

    // Puntino mese cambio (piccolo testo mese se diverso dal corrente)
    if (m != todM) {
      tft.setTextSize(1); tft.setTextColor(0x7BCF);
      tft.setCursor(cx + 3, cy + 13);
      tft.print(mNS[m - 1]);
    }

    // Labels eventi (max WK_MAX_EVENTS_VISIBLE)
    int evY = cy + (m != todM ? 24 : 14);
    for (int e = 0; e < WK_MAX_EVENTS_VISIBLE && e < n; e++) {
      String s = events[evtIdx[e]].summary; s.trim();
      if (s.length() > 7) s = s.substring(0, 7);
      tft.setTextSize(1); tft.setTextColor(EP[e % 5]);
      tft.setCursor(cx + 2, evY + e * 10);
      tft.print(s);
    }
    if (n > WK_MAX_EVENTS_VISIBLE) {
      // indicatore "+N" se ci sono più eventi
      tft.setTextSize(1); tft.setTextColor(0x7BCF);
      char more[6]; sprintf(more, "+%d", n - WK_MAX_EVENTS_VISIBLE);
      tft.setCursor(cx + 2, evY + WK_MAX_EVENTS_VISIBLE * 10);
      tft.print(more);
    }
  }
}

// Rileva il giorno toccato nella vista settimanale.
// Ritorna true e popola *outDay,*outMonth,*outYear se valido.
bool calGetTouchedDayWeek(uint16_t tx, uint16_t ty, int weekOffset,
                           int* outDay, int* outMonth, int* outYear) {
  int gy = 320 - (int)ty; // Y invertita
  if (gy < WK_TOP || gy >= WK_TOP + WK_CH) return false;
  int col = (int)tx / WK_CW;
  if (col < 0 || col >= WK_COLS) return false;

  time_t dayT = wkGetMonday(weekOffset) + (time_t)col * 86400;
  // Non permettere tap su giorni fuori dalla finestra 30 giorni
  time_t nowT = time(nullptr);
  if (dayT > nowT + 30 * 86400) return false;

  struct tm tmD; localtime_r(&dayT, &tmD);
  *outDay   = tmD.tm_mday;
  *outMonth = tmD.tm_mon + 1;
  *outYear  = tmD.tm_year + 1900;
  return true;
}

// Legacy stub (non più usata, rimane per compatibilità)
void printEventsTFT() {
  int y = 70; // Spostato a 70 per non sovrascrivere il titolo "Pagina 1"
  int x = 10;
  int cardWidth = 460;
  
  if (eventsCount == 0) {
    tft.setTextSize(2);
    tft.setTextColor(TFT_LIGHTGREY);
    tft.setCursor(x + 10, y + 20);
    tft.println("Nessun evento in programma.");
    return;
  }

  // Costruisci layout: quante card per pagina? circa 3-4, proviamo a calcolarlo
  // o semplicemente cicliamo finchè c'è spazio, MA partendo dall'indice giusto
  int eventiStart = 0;
  
  // un semplice sistema: ogni evento occupa circa 80-110px. 
  // in 240 px ci stanno 2/3 eventi.
  // Salteremo gli eventi già mostrati
  int skipped = 0;
  int shownOnThisPage = 0;
  bool moreEvents = false;

  for (int i = 0; i < eventsCount; i++) {
    Event &e = events[i];
    
    int cardHeight = 70;
    if (e.description.length() > 0) {
      cardHeight += 30;
    }

    // Se l'evento corrente appartiene a una pagina precedente, saltiamolo
    // Ma come sappiamo quanti eventi c'erano nelle pagine prima?
    // Un modo fisso è "3 eventi per pagina"
    if (i < eventiPageIndex * 3) {
      continue;
    }

    // Se usciamo dallo schermo con questo evento
    if (y + cardHeight > tft.height() - 40) { // Lasciamo 40px per il tasto "Altri eventi"
      moreEvents = true; // ci sono altri eventi da mostrare nella prossima pagina
      break;
    }

    // Disegna la card sfondo (blu scuro trasparente/arrotondato)
    tft.fillRoundRect(x, y, cardWidth, cardHeight, 10, 0x18E3); 
    tft.drawRoundRect(x, y, cardWidth, cardHeight, 10, TFT_CYAN);

    // Titolo evento
    tft.setTextSize(2);
    tft.setTextColor(TFT_YELLOW);
    tft.setCursor(x + 15, y + 15);
    String summaryStr = e.summary;
    if (summaryStr.length() > 30) {
      summaryStr = summaryStr.substring(0, 27) + "...";
    }
    tft.println(summaryStr);

    // Data/Ora dell'evento
    tft.setTextSize(1);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(x + 15, y + 40);
    
    if (e.allDay) {
      struct tm *tm = localtime(&e.start);
      char buf[64];
      strftime(buf, sizeof(buf), "%d/%m/%Y - Tutto il giorno", tm);
      tft.print("\x0F "); 
      tft.println(buf);
    } else {
      struct tm *tm = localtime(&e.start);
      char buf[64];
      strftime(buf, sizeof(buf), "%d/%m/%Y alle %H:%M", tm);
      tft.print("\x09 "); 
      tft.println(buf);
    }

    // Descrizione 
    if (e.description.length() > 0) {
      tft.setTextColor(TFT_LIGHTGREY);
      tft.setCursor(x + 15, y + 60);
      String descStr = e.description;
      if (descStr.length() > 60) {
        descStr = descStr.substring(0, 57) + "...";
      }
      tft.println(descStr);
    }

    y += cardHeight + 15; 
    shownOnThisPage++;
  }

  // Disegna bottone "Altri eventi" o "Torna all'inizio"
  if (eventsCount > 3) {
    tft.fillRoundRect(150, 275, 180, 40, 10, TFT_BLUE);
    tft.drawRoundRect(150, 275, 180, 40, 10, TFT_WHITE);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    
    if (moreEvents) {
      tft.setCursor(165, 287);
      tft.print("Altri Eventi");
    } else {
      tft.setCursor(160, 287);
      tft.print("Torna all'inizio");
      // Resetta per il prossimo click
      eventiPageIndex = -1; // -1 diventerà 0 al prossimo click nel loop
    }
  }
}

void printTasksTFT() {
  tft.fillScreen(0x0821);

  // Header
  tft.fillRect(0, 0, 480, 38, 0x1082);
  drawHouse(8, 6);
  tft.setTextSize(2); tft.setTextColor(TFT_WHITE);
  tft.setCursor(60, 11); tft.print("TO-DO LIST");

  // Contatore completati + pagina attuale
  int done = 0;
  for (int i = 0; i < tasksCount; i++) if (tasks[i].done) done++;
  int totalPages = (tasksCount + TASKS_PER_PAGE - 1) / TASKS_PER_PAGE;
  if (totalPages < 1) totalPages = 1;

  tft.setTextSize(1); tft.setTextColor(0x07E0);
  char prog[32]; sprintf(prog, "%d/%d completati", done, tasksCount);
  tft.setCursor(300, 10); tft.print(prog);
  char pgLabel[16]; sprintf(pgLabel, "Pag %d/%d", pageIndex + 1, totalPages);
  tft.setCursor(300, 22); tft.print(pgLabel);

  if (tasksCount == 0) {
    tft.setTextColor(0x7BCF); tft.setTextSize(2);
    tft.setCursor(120, 160); tft.print("Nessun task presente");
    return;
  }

  int y = 44;
  int start = pageIndex * TASKS_PER_PAGE;
  for (int i = start; i < tasksCount && i < start + TASKS_PER_PAGE && y < 274; i++) {
    Task& t = tasks[i];
    int cH = 44;

    // Card
    uint16_t cardBg = t.done ? 0x0C23 : 0x1143;
    tft.fillRoundRect(6, y, 468, cH, 6, cardBg);
    tft.drawRoundRect(6, y, 468, cH, 6, t.done ? 0x07E0 : TFT_CYAN);

    // Checkbox (sinistra)
    if (t.done) {
      tft.fillRoundRect(14, y+12, 18, 18, 3, 0x07E0);
      tft.setTextSize(1); tft.setTextColor(TFT_BLACK);
      tft.setCursor(18, y+16); tft.print("OK");
    } else {
      tft.drawRoundRect(14, y+12, 18, 18, 3, TFT_WHITE);
    }

    // Titolo task
    tft.setTextSize(1);
    tft.setTextColor(t.done ? 0x7BCF : TFT_WHITE);
    String title = t.title;
    if (title.length() > 52) title = title.substring(0, 49) + "...";
    tft.setCursor(40, y + 10); tft.print(title);

    // Data scadenza (se presente)
    if (t.due != 0) {
      struct tm* td = localtime(&t.due);
      char buf[32]; strftime(buf, sizeof(buf), "Scade: %d/%m/%Y", td);
      time_t nowT = time(nullptr);
      tft.setTextSize(1);
      tft.setTextColor(t.due < nowT ? 0xF800 : 0xFD20);
      tft.setCursor(40, y + 26); tft.print(buf);
    }

    y += cH + 4;
  }

  // ---- Barra di navigazione ----
  tft.fillRect(0, 278, 480, 42, 0x1082);

  bool hasPrev = (pageIndex > 0);
  bool hasNext = ((pageIndex + 1) * TASKS_PER_PAGE < tasksCount);

  // Pulsante PREV (◀ Prec) - lato sinistro
  if (hasPrev) {
    tft.fillRoundRect(6, 282, 110, 30, 6, 0x3186);
    tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
    tft.setCursor(18, 292); tft.print("< Prec.");
  }

  // Pulsante NEXT (Succ ▶) - lato destro
  if (hasNext) {
    tft.fillRoundRect(364, 282, 110, 30, 6, 0x3186);
    tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
    tft.setCursor(376, 292); tft.print("Succ. >");
  }
}

#endif // CALENDAR_WEEK_H

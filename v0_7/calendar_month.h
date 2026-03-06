#ifndef CALENDAR_MONTH_H
#define CALENDAR_MONTH_H

#include "config.h"

// ============================================================================
// CALENDARIO GRAFICO - griglia mensile
// ============================================================================
static const int CAL_HDR_H = 38;
static const int CAL_DN_H  = 20;
static const int CAL_TOP   = CAL_HDR_H + CAL_DN_H; // 58
static const int CAL_ROWS  = 6;
static const int CAL_COLS  = 7;
static const int CAL_CW    = 480 / CAL_COLS; // 68
static const int CAL_CH    = (320 - CAL_TOP) / CAL_ROWS; // 43

int calGetStartWday(int month, int year) {
  struct tm t = {}; t.tm_year=year-1900; t.tm_mon=month-1; t.tm_mday=1; t.tm_isdst=-1;
  mktime(&t);
  return (t.tm_wday + 6) % 7; // 0=Lun .. 6=Dom
}

int calDaysInMonth(int month, int year) {
  struct tm t = {}; t.tm_year=year-1900; t.tm_mon=month; t.tm_mday=0; t.tm_isdst=-1;
  mktime(&t); return t.tm_mday;
}

int calGetEventsForDay(int day, int month, int year, int* outIdx, int maxOut) {
  struct tm ts = {}; ts.tm_year=year-1900; ts.tm_mon=month-1; ts.tm_mday=day; ts.tm_isdst=-1;
  time_t dS = mktime(&ts), dE = dS + 86400;
  int n = 0;
  for (int i = 0; i < eventsCount && n < maxOut; i++) {
    time_t es = events[i].start;
    time_t ee = (events[i].end!=0) ? events[i].end : es+(events[i].allDay?86400:3600);
    if (es < dE && ee > dS) outIdx[n++] = i;
  }
  return n;
}

void drawCalendarGrid(int month, int year) {
  tft.fillScreen(0x0821);
  // Header
  tft.fillRect(0, 0, 480, CAL_HDR_H, 0x1082);
  drawHouse(8, 6);
  const char* mN[] = {"GENNAIO","FEBBRAIO","MARZO","APRILE","MAGGIO","GIUGNO",
                      "LUGLIO","AGOSTO","SETTEMBRE","OTTOBRE","NOVEMBRE","DICEMBRE"};
  char hdr[32]; sprintf(hdr, "%s %d", mN[month-1], year);
  tft.setTextSize(2); tft.setTextColor(TFT_WHITE);
  tft.setCursor(240-(int)strlen(hdr)*6+20, 11);
  tft.print(hdr);
  // Nomi giorni
  tft.fillRect(0, CAL_HDR_H, 480, CAL_DN_H, 0x10A2);
  const char* dn[] = {"L","M","M","G","V","S","D"};
  for (int c=0; c<7; c++) {
    tft.setTextSize(1); tft.setTextColor(c>=5?0xF800:0x7BCF);
    tft.setCursor(c*CAL_CW + CAL_CW/2-3, CAL_HDR_H+6);
    tft.print(dn[c]);
  }
  int startW = calGetStartWday(month, year);
  int dim    = calDaysInMonth(month, year);
  getLocalTime(&timeinfo);
  int todD=timeinfo.tm_mday, todM=timeinfo.tm_mon+1, todY=timeinfo.tm_year+1900;
  int evtIdx[8];
  static const uint16_t EP[]={0xFFE0,0x07FF,0xFD20}; // giallo, ciano, arancione
  for (int row=0; row<CAL_ROWS; row++) {
    for (int col=0; col<CAL_COLS; col++) {
      int d  = row*CAL_COLS + col - startW + 1;
      int cx = col*CAL_CW, cy = CAL_TOP + row*CAL_CH;
      tft.drawRect(cx, cy, CAL_CW, CAL_CH, 0x2104);
      if (d<1 || d>dim) continue;
      bool isToday = (d==todD && month==todM && year==todY);
      int n = calGetEventsForDay(d, month, year, evtIdx, 8);
      uint16_t bg = isToday ? 0x0228 : (n>0 ? 0x1143 : 0x0841);
      tft.fillRect(cx+1, cy+1, CAL_CW-2, CAL_CH-2, bg);
      if (isToday) {
        tft.drawRect(cx, cy, CAL_CW, CAL_CH, TFT_CYAN);
        tft.drawRect(cx+1, cy+1, CAL_CW-2, CAL_CH-2, TFT_CYAN);
      }
      // Numero giorno
      tft.setTextSize(1);
      tft.setTextColor(isToday ? TFT_CYAN : (col>=5 ? 0xFB00 : TFT_WHITE));
      char ds[3]; sprintf(ds,"%d",d);
      tft.setCursor(cx+3, cy+3); tft.print(ds);
      // Labels eventi (max 3)
      for (int e=0; e<3 && e<n; e++) {
        String s = events[evtIdx[e]].summary; s.trim();
        if (s.length()>7) s=s.substring(0,7);
        tft.setTextSize(1); tft.setTextColor(EP[e]);
        tft.setCursor(cx+2, cy+14+e*10); tft.print(s);
      }
    }
  }
}

// Restituisce il giorno (1-31) toccato, oppure -1
int calGetTouchedDay(uint16_t tx, uint16_t ty, int month, int year) {
  int gy = 320 - (int)ty; // Y invertita
  if (gy < CAL_TOP || gy >= CAL_TOP + CAL_ROWS*CAL_CH) return -1;
  int row = (gy - CAL_TOP) / CAL_CH;
  int col = (int)tx / CAL_CW;
  if (col<0 || col>=CAL_COLS) return -1;
  int d = row*CAL_COLS + col - calGetStartWday(month,year) + 1;
  int dim = calDaysInMonth(month, year);
  if (d<1 || d>dim) return -1;
  return d;
}

void drawDayDetail(int day, int month, int year) {
  tft.fillScreen(0x0821);
  // Header
  tft.fillRect(0, 0, 480, CAL_HDR_H, 0x1082);
  const char* mN[] = {"Gen","Feb","Mar","Apr","Mag","Giu","Lug","Ago","Set","Ott","Nov","Dic"};
  char title[32]; sprintf(title, "%d %s %d", day, mN[month-1], year);
  tft.setTextSize(2); tft.setTextColor(TFT_WHITE);
  tft.setCursor(60, 11); tft.print(title);
  // Bottone "< Cal." in alto a DESTRA (touch x>380 && y>280)
  tft.fillRoundRect(410, 5, 66, 28, 6, 0x3186);
  tft.setTextSize(1); tft.setTextColor(TFT_WHITE);
  tft.setCursor(416, 15); tft.print("< Cal.");
  // Lista eventi
  int evtIdx[MAX_EVENTS];
  int n = calGetEventsForDay(day, month, year, evtIdx, MAX_EVENTS);
  if (n==0) {
    tft.setTextColor(0x7BCF); tft.setTextSize(2);
    tft.setCursor(140, 160); tft.print("Nessun evento"); return;
  }
  int y=44;
  for (int i=0; i<n && y<310; i++) {
    Event& e = events[evtIdx[i]];
    bool hasDsc = e.description.length()>0;
    int cH = hasDsc ? 60 : 44;
    tft.fillRoundRect(6, y, 468, cH, 6, 0x1143);
    tft.drawRoundRect(6, y, 468, cH, 6, TFT_CYAN);
    tft.setTextSize(1);
    if (e.allDay) {
      tft.setTextColor(TFT_LIGHTGREY); tft.setCursor(12,y+5); tft.print("Tutto il giorno");
    } else {
      struct tm* st=localtime(&e.start); char tb[12]; strftime(tb,sizeof(tb),"%H:%M",st);
      String ts2=String(tb);
      if (e.end!=0) { struct tm* et=localtime(&e.end); char tb2[12]; strftime(tb2,sizeof(tb2),"-%H:%M",et); ts2+=String(tb2); }
      tft.setTextColor(TFT_CYAN); tft.setCursor(12,y+5); tft.print(ts2);
    }
    tft.setTextSize(1); tft.setTextColor(TFT_YELLOW);
    String summ=e.summary; if(summ.length()>60) summ=summ.substring(0,57)+"...";
    tft.setCursor(12,y+18); tft.print(summ);
    if (hasDsc) {
      tft.setTextColor(0x8C71);
      String desc=e.description; if(desc.length()>60) desc=desc.substring(0,57)+"...";
      tft.setCursor(12,y+32); tft.print(desc);
    }
    y += cH+4;
  }
}

#endif // CALENDAR_MONTH_H

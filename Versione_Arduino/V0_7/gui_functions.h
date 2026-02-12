#ifndef GUI_FUNCTIONS_H
#define GUI_FUNCTIONS_H

#include "config.h"

void drawWiFiSymbol(int x, int y) {
  bool connected = (WiFi.status() == WL_CONNECTED);
  uint16_t color = connected ? TFT_GREEN : TFT_RED;

  tft.fillRect(x - 20, y - 20, 40, 40, TFT_BLUE);

  // Punto centrale
  tft.fillCircle(x, y + 12, 3, color);

  // Semicerchi
  for (int r = 6; r <= 18; r += 4) {
    for (int angle = 230; angle <= 320; angle += 1) {
      int px = x + r * cos(angle * PI / 180.0);
      int py = y + r * sin(angle * PI / 180.0) + 12;
      tft.drawPixel(px, py, color);
    }
  }
}

void drawGearIcon(int x, int y, uint16_t bg) {
  uint16_t color = TFT_WHITE;

  // Centro
  tft.drawCircle(x, y, 5, color);
  tft.drawCircle(x, y, 4, color);

  // Corpo principale
  tft.drawCircle(x, y, 12, color);
  tft.drawCircle(x, y, 11, color);

  // Disegna 8 denti
  for (int i = 0; i < 8; i++) {
    int angle = i * 45;

    int x1 = x + 13 * cos(angle * PI / 180.0);
    int y1 = y + 13 * sin(angle * PI / 180.0);
    int x2 = x + 16 * cos(angle * PI / 180.0);
    int y2 = y + 16 * sin(angle * PI / 180.0);

    int x3 = x + 16 * cos((angle - 10) * PI / 180.0);
    int y3 = y + 16 * sin((angle - 10) * PI / 180.0);
    int x4 = x + 16 * cos((angle + 10) * PI / 180.0);
    int y4 = y + 16 * sin((angle + 10) * PI / 180.0);

    int x5 = x + 13 * cos((angle - 10) * PI / 180.0);
    int y5 = y + 13 * sin((angle - 10) * PI / 180.0);
    int x6 = x + 13 * cos((angle + 10) * PI / 180.0);
    int y6 = y + 13 * sin((angle + 10) * PI / 180.0);

    tft.drawLine(x1, y1, x2, y2, color);
    tft.drawLine(x5, y5, x3, y3, color);
    tft.drawLine(x6, y6, x4, y4, color);
    tft.drawLine(x3, y3, x4, y4, color);
  }

  // Rinforza cerchi
  for (int r = 3; r <= 6; r++) {
    tft.drawCircle(x, y, r, color);
  }
  for (int r = 10; r <= 13; r++) {
    tft.drawCircle(x, y, r, color);
  }
}

// Overload comodo: usa lo sfondo di default della home
void drawGearIcon(int x, int y) {
  drawGearIcon(x, y, sfondo_page0);
}

void drawCircleWithDot(int x, int y, int radius) {
  int innerRadius = radius / 3;
  int circleThickness = 5;

  if (circleThickness < 1) circleThickness = 1;

  for (int i = 0; i < circleThickness; i++) {
    tft.drawCircle(x, y, radius - i, TFT_WHITE);
  }

  tft.fillCircle(x, y, innerRadius, TFT_WHITE);
}

void drawHouse() {
  int x = 20;
  int width = 35;
  int y = 20;
  int height = width * 0.8;
  int roofHeight = width * 0.4;
  int doorWidth = width * 0.25;
  int doorHeight = height * 0.6;
  int windowSize = width * 0.15;

  uint16_t wallColor = TFT_YELLOW;
  uint16_t roofColor = TFT_RED;
  uint16_t doorColor = TFT_BROWN;
  uint16_t windowColor = TFT_CYAN;
  uint16_t frameColor = TFT_WHITE;

  // Corpo casa
  tft.fillRect(x, y, width, height, wallColor);
  tft.drawRect(x, y, width, height, frameColor);

  // Tetto
  int roofTopX = x + width / 2;
  int roofTopY = y - roofHeight;

  tft.drawLine(x, y, roofTopX, roofTopY, roofColor);
  tft.drawLine(x + width, y, roofTopX, roofTopY, roofColor);
  tft.drawLine(x, y, x + width, y, roofColor);

  for (int i = 0; i < roofHeight; i++) {
    int lineY = roofTopY + i;
    int lineWidth = (i * width) / roofHeight;
    int lineStartX = roofTopX - lineWidth / 2;
    int lineEndX = roofTopX + lineWidth / 2;
    tft.drawLine(lineStartX, lineY, lineEndX, lineY, roofColor);
  }

  // Porta
  int doorX = x + (width - doorWidth) / 2;
  int doorY = y + height - doorHeight;
  tft.fillRect(doorX, doorY, doorWidth, doorHeight, doorColor);
  tft.drawRect(doorX, doorY, doorWidth, doorHeight, frameColor);

  int handleX = doorX + doorWidth - 4;
  int handleY = doorY + doorHeight / 2;
  tft.fillCircle(handleX, handleY, 2, TFT_DARKGREY);

  // Finestre
  int windowY = y + height / 4;

  int leftWindowX = x + width * 0.15;
  tft.fillRect(leftWindowX, windowY, windowSize, windowSize, windowColor);
  tft.drawRect(leftWindowX, windowY, windowSize, windowSize, frameColor);
  tft.drawLine(leftWindowX + windowSize / 2, windowY,
               leftWindowX + windowSize / 2, windowY + windowSize,
               frameColor);
  tft.drawLine(leftWindowX, windowY + windowSize / 2,
               leftWindowX + windowSize, windowY + windowSize / 2,
               frameColor);

  int rightWindowX = x + width * 0.6;
  tft.fillRect(rightWindowX, windowY, windowSize, windowSize, windowColor);
  tft.drawRect(rightWindowX, windowY, windowSize, windowSize, frameColor);
  tft.drawLine(rightWindowX + windowSize / 2, windowY,
               rightWindowX + windowSize / 2, windowY + windowSize,
               frameColor);
  tft.drawLine(rightWindowX, windowY + windowSize / 2,
               rightWindowX + windowSize, windowY + windowSize / 2,
               frameColor);

  // Camino
  int chimneyWidth = width * 0.1;
  int chimneyHeight = roofHeight * 0.6;
  int chimneyX = x + width * 0.75;
  int chimneyY = roofTopY + roofHeight * 0.3;

  tft.fillRect(chimneyX, chimneyY, chimneyWidth, chimneyHeight, TFT_DARKGREY);
  tft.drawRect(chimneyX, chimneyY, chimneyWidth, chimneyHeight, frameColor);
}

void drawCaricamento(int cx, int cy, int num_giri) {
  Serial.println("Pallini del caricamento ...");

  uint16_t dotcolor = TFT_WHITE;
  uint16_t sfondo = TFT_LIGHTGREY;
  int raggio = 15;

  tft.fillRect(cx - raggio, cy - raggio, 31, 31, sfondo);

  for (int i = 0; i < num_giri; i++) {
    for (int angolo = 0; angolo < 360; angolo += 40) {
      float rad = angolo * PI / 180.0;
      float rad_precedente = (angolo - 80) * PI / 180.0;

      int x = cx + raggio * cos(rad);
      int y = cy + raggio * sin(rad);
      tft.fillCircle(x, y, 3, dotcolor);

      int x_old = cx + raggio * cos(rad_precedente);
      int y_old = cy + raggio * sin(rad_precedente);
      tft.fillCircle(x_old, y_old, 3, sfondo);

      delay(100);
    }
  }
}

void drawScrollBar(int x, int y, int h, int posizioni) {
  int w = 25;
  int arrowH = 25;
  int trackH = h - (arrowH * 2);

  tft.fillRect(x, y, w, h, TFT_DARKGREY);

  // Freccia SU
  if (posizioni == 0 || posizioni == 1) {
    tft.fillTriangle(x + w / 2, y + 5, x + 5, y + arrowH - 5, x + w - 5,
                     y + arrowH - 5, TFT_WHITE);
  }

  // Freccia GIÙ
  int yb = y + h - arrowH;
  if (posizioni == 0 || posizioni == 2) {
    tft.fillTriangle(x + 5, yb + 5, x + w - 5, yb + 5, x + w / 2,
                     yb + arrowH - 5, TFT_WHITE);
  }

  // Binario centrale
  tft.fillRect(x + w / 3, y + arrowH, w / 3, trackH, TFT_BLACK);
}

void disegnaHome() {
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
}

void disegnaGrigliaHome() {
  int cellW = 160;   // 480 / 3
  int headerH = 70;  // spazio in alto per titolo/disegni
  int gridH = 320 - headerH;
  int cellH = gridH / 3;  // altezza di ogni riga della griglia

  drawHouse();

  // Griglia 3x3 che parte dal basso, lasciando headerH pixel liberi in alto
  for (int row = 0; row < 3; row++) {
    for (int col = 0; col < 3; col++) {
      int x = col * cellW;
      // row 0 = riga più in basso, row 2 = riga più in alto
      int y = headerH + (2 - row) * cellH;

      // Disegna il bordo del rettangolo smussato
      tft.drawRoundRect(x + 5, y + 5, cellW - 10, cellH - 10, 10, TFT_DARKGREY);

      // Testo al centro del quadrato
      tft.setTextColor(TFT_WHITE, TFT_TRANSPARENT);
      tft.setTextSize(2);
      int textX = x + 20;
      int textY = y + cellH / 2;

      if (row == 0 && col == 0) {
        tft.setCursor(textX, textY);
        tft.println("Pagina 7");
      } else if (row == 0 && col == 1) {
        tft.setCursor(textX, textY);
        tft.println("Pagina 8");
      } else if (row == 0 && col == 2) {
        drawGearIcon(x + 80, y + 30, COLOR_CARD);
        tft.setCursor(textX - 10, textY + 15);
        tft.println("Impostazioni");
      } else if (row == 1 && col == 0) {
        tft.setCursor(textX, textY);
        tft.println("Pagina 4");
      } else if (row == 1 && col == 1) {
        tft.setCursor(textX, textY);
        tft.println("Pagina 5");
      } else if (row == 1 && col == 2) {
        tft.setCursor(textX, textY);
        tft.println("Pagina 6");
      } else if (row == 2 && col == 0) {
        tft.setCursor(textX, textY);
        tft.println("Pagina 1");
      } else if (row == 2 && col == 1) {
        tft.setCursor(textX, textY);
        tft.println("Pagina 2");
      } else if (row == 2 && col == 2) {  // riga in alto, colonna destra
        tft.setCursor(textX, textY);
        tft.println("Pagina 3");
      }
    }
  }
}

void drawGradientBackground(uint16_t startColor, uint16_t endColor) {
  // Scomposizione del colore di partenza (RGB565)
  uint8_t r1 = (startColor >> 11) & 0x1F;
  uint8_t g1 = (startColor >> 5) & 0x3F;
  uint8_t b1 = startColor & 0x1F;

  // Scomposizione del colore di arrivo (RGB565)
  uint8_t r2 = (endColor >> 11) & 0x1F;
  uint8_t g2 = (endColor >> 5) & 0x3F;
  uint8_t b2 = endColor & 0x1F;

  for (int y = 0; y < 320; y++) {
    // Calcolo dell'interpolazione lineare per ogni canale
    uint8_t r = r1 + (r2 - r1) * y / 319;
    uint8_t g = g1 + (g2 - g1) * y / 319;
    uint8_t b = b1 + (b2 - b1) * y / 319;

    // Ricomposizione nel formato RGB565
    uint16_t color = (r << 11) | (g << 5) | b;

    // Disegna una linea orizzontale di quel colore
    tft.drawFastHLine(0, y, 480, color);
  }
}

#endif
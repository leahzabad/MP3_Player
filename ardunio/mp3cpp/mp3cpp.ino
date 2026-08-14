#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include "image_data.h"  

#define TFT_CS   10
#define TFT_DC   7
#define TFT_RST  8

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

const int IMG_W = 108;
const int IMG_H = 108;

void setup() {
  Serial.begin(9600);
  delay(1000);
  Serial.println("Starting...");

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  Serial.println("Display initialized");

  int offsetX = ((tft.width()  - IMG_W) / 2);
  int offsetY = ((tft.height() - IMG_H) / 2) - 16;

  for (int y = 0; y < IMG_H; y++) {
    for (int x = 0; x < IMG_W; x++) {
      uint16_t color = pgm_read_word(&albumArt[y * IMG_W + x]);
      tft.drawPixel(offsetX + x, offsetY + y, color);
    }
  }

 for (int x = 0; x < IMG_W; x++) {
    int y = IMG_H + 10;
    uint16_t color = 8418;
    tft.drawPixel(offsetX + x, offsetY + y, color);
  }

  Serial.println("Image drawn");

  Serial.print("width: "); Serial.println(tft.width());
  Serial.print("height: "); Serial.println(tft.height());
  Serial.print("offsetX: "); Serial.println(offsetX);
  Serial.print("offsetY: "); Serial.println(offsetY);
}

void loop() {
}
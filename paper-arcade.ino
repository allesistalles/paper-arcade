#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

#define TFT_BL_PIN 21

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("Paper Arcade", 80, 110, 4);
  Serial.println("Boot OK");
}

void loop() {
  delay(1000);
}

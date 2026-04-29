#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <SPI.h>
#include <SD.h>
#include <Preferences.h>
#include <ArduinoOTA.h>
#include <WiFi.h>

#include "src/core/Game.h"
#include "src/core/InputManager.h"
#include "src/core/AssetManager.h"
#include "src/core/ScoreManager.h"
#include "src/core/Launcher.h"
#include "src/ui/Theme.h"

// ---- Game includes (uncomment as built) ----
// #include "src/games/Snake.h"
// ... etc

#define TOUCH_CS_PIN  33
#define SD_CS_PIN      5
#define TFT_BL_PIN    21

TFT_eSPI      tft;
InputManager  input(TOUCH_CS_PIN);
AssetManager  assets;
ScoreManager  scores;
Launcher      launcher;
Game*         activeGame = nullptr;
Game*         prevActiveGame = nullptr;   // for launch-tap suppression

bool checkOTAHold() {
  XPT2046_Touchscreen probe(TOUCH_CS_PIN);
  probe.begin();
  uint32_t start = millis();
  while (millis() - start < 3000) {
    if (!probe.touched()) return false;
    delay(50);
  }
  return true;
}

void enterOTAMode() {
  tft.fillScreen(tft.color24to16(Theme::BG));
  tft.setTextColor(tft.color24to16(Theme::ACCENT), tft.color24to16(Theme::BG));
  tft.drawString("OTA MODE", 90, 60, 4);

  WiFi.mode(WIFI_STA);
  WiFi.begin();
  uint32_t t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) delay(200);

  tft.setTextColor(tft.color24to16(Theme::TEXT), tft.color24to16(Theme::BG));
  if (WiFi.status() == WL_CONNECTED) {
    tft.drawString(WiFi.localIP().toString(), 70, 120, 2);
    ArduinoOTA.setHostname("paper-arcade");
    ArduinoOTA.begin();
    tft.setTextColor(tft.color24to16(Theme::DIM), tft.color24to16(Theme::BG));
    tft.drawString("Ready for upload", 70, 150, 2);
    while (true) ArduinoOTA.handle();
  } else {
    tft.setTextColor(tft.color24to16(Theme::ACCENT), tft.color24to16(Theme::BG));
    tft.drawString("WiFi failed", 90, 120, 2);
    delay(2000);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(tft.color24to16(Theme::BG));

  tft.setTextColor(tft.color24to16(Theme::ACCENT), tft.color24to16(Theme::BG));
  tft.drawString("PAPER ARCADE", 50, 100, 4);
  delay(800);

  if (checkOTAHold()) enterOTAMode();

  input.begin();
  SPI.begin();
  assets.begin(SD_CS_PIN);
  scores.begin();

  // ---- Game registry (uncomment as each game is built) ----
  // launcher.addGame(new Snake());
  // ... etc

  launcher.begin(tft, assets, scores);
}

void loop() {
  ArduinoOTA.handle();

  InputEvent evt = input.read();
  activeGame = launcher.update(evt);

  bool justLaunched = (activeGame && !prevActiveGame);

  if (activeGame) {
    // Don't forward the launch tap to the game — it would trigger a spurious
    // first-frame action (fire, jump, etc.). Once the game is running, the
    // user's next touch is theirs.
    if (!justLaunched) {
      activeGame->update(evt);
    }
    if (activeGame->isDone() && evt.type == InputEvent::TAP && !justLaunched) {
      launcher.returnToMenu();
      activeGame = nullptr;
    }
  }
  launcher.draw();
  prevActiveGame = activeGame;
}

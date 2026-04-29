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

// ---- Game includes ----
#include "src/games/Snake.h"
#include "src/games/Pong.h"
#include "src/games/SimonSays.h"
#include "src/games/Minesweeper.h"
#include "src/games/Game2048.h"
#include "src/games/Breakout.h"
#include "src/games/FlappyBird.h"
#include "src/games/Tetris.h"
#include "src/games/SpaceInvaders.h"
#include "src/games/PacMan.h"

// CYD pin map:
//   TFT (HSPI):   MOSI=13 MISO=12 SCK=14 CS=15 DC=2 BL=21
//   Touch (VSPI): MOSI=32 MISO=39 SCK=25 CS=33 IRQ=36
//   SD  (HSPI):   shares TFT bus, CS=5
#define TOUCH_CS_PIN   33
#define TOUCH_IRQ_PIN  36
#define TOUCH_SCK      25
#define TOUCH_MOSI     32
#define TOUCH_MISO     39
#define SD_CS_PIN       5
#define TFT_BL_PIN     21

TFT_eSPI      tft;
SPIClass      touchSPI(VSPI);                   // Touch on its own VSPI bus
SPIClass      sdSPI(HSPI);                      // SD on its own HSPI instance (TFT_eSPI owns the other one)
InputManager  input(TOUCH_CS_PIN, TOUCH_IRQ_PIN);
AssetManager  assets;
ScoreManager  scores;
Launcher      launcher;
Game*         activeGame = nullptr;
Game*         prevActiveGame = nullptr;          // for launch-tap suppression

bool checkOTAHold() {
  // Use the same VSPI bus + IRQ pin as InputManager so touch reads are reliable.
  XPT2046_Touchscreen probe(TOUCH_CS_PIN, TOUCH_IRQ_PIN);
  probe.begin(touchSPI);
  uint32_t start = millis();
  while (millis() - start < 3000) {
    if (!probe.touched()) return false;
    delay(50);
  }
  return true;
}

void enterOTAMode() {
  // Layout for portrait 240×320: center text horizontally
  tft.fillScreen(tft.color24to16(Theme::BG));
  tft.setTextColor(tft.color24to16(Theme::ACCENT), tft.color24to16(Theme::BG));
  tft.drawString("OTA MODE", 50, 80, 4);

  WiFi.mode(WIFI_STA);
  WiFi.begin();
  uint32_t t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) delay(200);

  tft.setTextColor(tft.color24to16(Theme::TEXT), tft.color24to16(Theme::BG));
  if (WiFi.status() == WL_CONNECTED) {
    tft.drawString(WiFi.localIP().toString(), 30, 150, 2);
    ArduinoOTA.setHostname("paper-arcade");
    ArduinoOTA.begin();
    tft.setTextColor(tft.color24to16(Theme::DIM), tft.color24to16(Theme::BG));
    tft.drawString("Ready for upload", 30, 180, 2);
    while (true) ArduinoOTA.handle();
  } else {
    tft.setTextColor(tft.color24to16(Theme::ACCENT), tft.color24to16(Theme::BG));
    tft.drawString("WiFi failed", 50, 150, 2);
    tft.setTextColor(tft.color24to16(Theme::DIM), tft.color24to16(Theme::BG));
    tft.drawString("Set creds via Serial", 25, 180, 2);
    delay(3000);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TFT_BL_PIN, OUTPUT);
  digitalWrite(TFT_BL_PIN, HIGH);

  tft.init();
  tft.invertDisplay(true);   // CYD ILI9341_2 panel ships with inverted polarity
  tft.setRotation(0);        // portrait, USB plug at bottom (240w x 320h)
  tft.fillScreen(tft.color24to16(Theme::BG));

  // Bring up the touch VSPI bus before any touched() probe is called.
  touchSPI.begin(TOUCH_SCK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS_PIN);

  tft.setTextColor(tft.color24to16(Theme::ACCENT), tft.color24to16(Theme::BG));
  tft.drawString("PAPER", 65, 130, 6);
  tft.drawString("ARCADE", 60, 180, 6);
  delay(800);

  if (checkOTAHold()) enterOTAMode();

  input.begin(touchSPI);
  // SD on its own HSPI instance bound to the TFT's pins (the chip has multiplexer-free
  // sharing here — TFT_eSPI talks to its own internal SPI, this one is for SD only).
  sdSPI.begin(/*SCK=*/14, /*MISO=*/12, /*MOSI=*/13, /*SS=*/SD_CS_PIN);
  if (!assets.begin(SD_CS_PIN, sdSPI)) {
    Serial.println("SD init failed (continuing without SD assets)");
  }
  scores.begin();

  // ---- Game registry ----
  launcher.addGame(new Snake());
  launcher.addGame(new Pong());
  launcher.addGame(new SimonSays());
  launcher.addGame(new Minesweeper());
  launcher.addGame(new Game2048());
  launcher.addGame(new Breakout());
  launcher.addGame(new FlappyBird());
  launcher.addGame(new Tetris());
  launcher.addGame(new SpaceInvaders());
  launcher.addGame(new PacMan());

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
    // Also don't forward LONG_PRESS — it's consumed by the Launcher pause overlay.
    bool isLongPress = (evt.type == InputEvent::LONG_PRESS);
    if (!justLaunched && !isLongPress) {
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

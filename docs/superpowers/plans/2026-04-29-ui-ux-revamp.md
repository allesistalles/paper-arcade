# UI/UX Revamp Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the Synthwave UI with a Minimal Premium system — OLED-black background, electric-blue accent, per-game bold-text as visual, a unified vertical-list launcher, and a shared HUD/pause/game-over chrome composited by the Launcher over every game.

**Architecture:** Three layers: (1) `Theme.h` defines the new palette, (2) `Launcher` owns the homepage list, the in-game HUD strip, and the pause/game-over overlay, (3) each game renders its play area from `y=22..319`, exposes `score()`, and renders a standard game-over overlay when `_done == true`. The Launcher composites the HUD strip (y=0..21) on top after calling `_active->draw()`.

**Tech Stack:** Arduino / C++17, TFT_eSPI, ESP32 CYD (240×320 portrait, rotation 0). Compile via `arduino-cli compile --fqbn esp32:esp32:esp32 .` from the project root.

---

## File Map

| File | Change |
|------|--------|
| `src/ui/Theme.h` | Full palette replacement |
| `src/ui/GameOver.h` | **New** — shared `drawGameOverOverlay()` helper |
| `src/core/Game.h` | Add `virtual uint32_t score() = 0` |
| `src/core/Launcher.h` | Remove carousel state; add `gameAccentColor()`, `drawHUD()` |
| `src/core/Launcher.cpp` | Full rewrite — list homepage + HUD compositing + pause |
| `paper-arcade.ino` | Splash screen only |
| `src/games/Snake.cpp/.h` | Add `score()`, shift grid y+22, remove own HUD/game-over |
| `src/games/Pong.cpp/.h` | Add `score()`, adjust AI paddle y, remove own HUD/game-over |
| `src/games/SimonSays.cpp/.h` | Add `score()`, remove own game-over |
| `src/games/Minesweeper.cpp/.h` | Add `score()`, shift grid y+22, remove own HUD/game-over |
| `src/games/Game2048.cpp/.h` | Add `score()`, remove own score/game-over |
| `src/games/Breakout.cpp/.h` | Add `score()`, shift bricks y+22, remove own HUD/game-over |
| `src/games/FlappyBird.cpp/.h` | Add `score()`, remove own score display/game-over |
| `src/games/Tetris.cpp/.h` | Add `score()`, shift OFFY+22, remove own sidebar score/game-over |
| `src/games/SpaceInvaders.cpp/.h` | Add `score()`, adjust swarm start y, remove own HUD/game-over |
| `src/games/PacMan.cpp/.h` | Add `score()`, remove own score header, adjust MAZE_Y |

---

### Task 1: New Theme palette

**Files:**
- Modify: `src/ui/Theme.h`

- [ ] **Step 1: Replace Theme.h entirely**

```cpp
#pragma once
#include <cstdint>

// Minimal Premium palette — OLED black base, electric blue accent.
// 24-bit values are canonical; RGB565 computed with ((r>>3)<<11)|((g>>2)<<5)|(b>>3).
namespace Theme {
  // System colours
  constexpr uint32_t BG     = 0x000000;  // pure OLED black
  constexpr uint32_t TEXT   = 0xFFFFFF;  // sharp white
  constexpr uint32_t ACCENT = 0x0A84FF;  // electric blue
  constexpr uint32_t MUTED  = 0x8E8E93;  // grey — labels, hints, unplayed scores
  constexpr uint32_t CARD   = 0x1C1C1E;  // elevated surface (pause modal)
  constexpr uint32_t SEP    = 0x2C2C2E;  // separator lines
  constexpr uint32_t DANGER = 0xFF453A;  // quit / error red

  // Pre-computed RGB565
  constexpr uint16_t BG565     = 0x0000;
  constexpr uint16_t TEXT565   = 0xFFFF;
  constexpr uint16_t ACCENT565 = 0x0C3F;  // #0A84FF
  constexpr uint16_t MUTED565  = 0x8C72;  // #8E8E93
  constexpr uint16_t CARD565   = 0x18E3;  // #1C1C1E
  constexpr uint16_t SEP565    = 0x2965;  // #2C2C2E
  constexpr uint16_t DANGER565 = 0xFA27;  // #FF453A

  // Per-game accent colours (RGB565)
  constexpr uint16_t SNAKE565    = 0x368B;  // #30D158 green
  constexpr uint16_t PONG565     = 0x0C3F;  // #0A84FF blue
  constexpr uint16_t SIMON565    = 0xFA27;  // #FF453A red
  constexpr uint16_t MINES565    = 0xFCE1;  // #FF9F0A amber
  constexpr uint16_t G2048565    = 0xF9AB;  // #FF375F rose
  constexpr uint16_t BREAKOUT565 = 0x5AFC;  // #5E5CE6 indigo
  constexpr uint16_t FLAPPY565   = 0xFEA1;  // #FFD60A yellow
  constexpr uint16_t TETRIS565   = 0x669F;  // #64D2FF sky
  constexpr uint16_t INVADERS565 = 0xFB4C;  // #FF6961 salmon
  constexpr uint16_t PACMAN565   = 0xBADE;  // #BF5AF2 purple
}
```

- [ ] **Step 2: Verify compile — Theme.h has no hardware deps, should compile for native too**

```bash
cd "/Users/RoyKorkomaz/all cloned from github/paper-arcade"
make test_input  # just exercises test_runner.h + Game.h, quick sanity
```

Expected: `8/8 passed` (Theme.h change doesn't affect logic tests)

- [ ] **Step 3: Commit**

```bash
git add src/ui/Theme.h
git commit -m "feat: replace Synthwave palette with Minimal Premium OLED theme"
```

---

### Task 2: GameOver helper + Game interface score()

**Files:**
- Create: `src/ui/GameOver.h`
- Modify: `src/core/Game.h`

- [ ] **Step 1: Create src/ui/GameOver.h**

```cpp
#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../ui/Theme.h"

// Draws the unified game-over overlay (full 240x320 screen).
// Call from game->draw() when _done == true.
// gameColor: per-game RGB565 accent colour (e.g. Theme::SNAKE565)
// won: true shows "YOU WIN!" in green, false shows "GAME OVER" in white
inline void drawGameOverOverlay(TFT_eSPI& s, const char* gameName,
                                uint32_t finalScore, uint16_t gameColor,
                                bool won = false) {
  uint16_t bg  = Theme::BG565;
  uint16_t txt = Theme::TEXT565;
  uint16_t mut = Theme::MUTED565;

  s.fillScreen(bg);

  // Game name in accent colour, font 6
  s.setTextColor(gameColor, bg);
  int nw = s.textWidth(gameName, 6);
  s.drawString(gameName, 120 - nw / 2, 80, 6);

  // "GAME OVER" / "YOU WIN!" in white (or green for wins)
  const char* msg = won ? "YOU WIN!" : "GAME OVER";
  uint16_t msgCol = won ? Theme::SNAKE565 : txt;  // green for wins
  s.setTextColor(msgCol, bg);
  int mw = s.textWidth(msg, 4);
  s.drawString(msg, 120 - mw / 2, 148, 4);

  // "YOUR SCORE" label
  s.setTextColor(mut, bg);
  int yw = s.textWidth("YOUR SCORE", 1);
  s.drawString("YOUR SCORE", 120 - yw / 2, 200, 1);

  // Score value
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)finalScore);
  s.setTextColor(txt, bg);
  int sw = s.textWidth(buf, 6);
  s.drawString(buf, 120 - sw / 2, 215, 6);

  // "TAP ANYWHERE"
  s.setTextColor(mut, bg);
  int tw = s.textWidth("TAP ANYWHERE", 1);
  s.drawString("TAP ANYWHERE", 120 - tw / 2, 295, 1);
}
#endif
```

- [ ] **Step 2: Add score() to Game.h**

In `src/core/Game.h`, add `score()` after `highScore()`:

```cpp
  virtual uint32_t    highScore() = 0;
  // score() returns the live in-game score for the HUD strip.
  virtual uint32_t    score()     = 0;
  // isDone() returning true means: the next user TAP returns to launcher.
```

- [ ] **Step 3: Compile to confirm the new pure-virtual breaks all games (expected)**

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 . 2>&1 | grep "error:" | head -20
```

Expected: errors like `'score' is not declared` for each game — confirms the interface is in place and needs to be fulfilled.

- [ ] **Step 4: Commit (games broken, will fix per-game in Tasks 6–15)**

```bash
git add src/ui/GameOver.h src/core/Game.h
git commit -m "feat: GameOver overlay helper + score() to Game interface"
```

---

### Task 3: Launcher rewrite

**Files:**
- Modify: `src/core/Launcher.h`
- Modify: `src/core/Launcher.cpp`

- [ ] **Step 1: Replace src/core/Launcher.h**

```cpp
#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "Game.h"
#include "AssetManager.h"
#include "ScoreManager.h"

class Launcher {
public:
  static const int MAX_GAMES = 10;

  void  begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores);
  // Caller retains ownership of the Game*; Launcher does not delete.
  void  addGame(Game* game);
  Game* update(const InputEvent& evt);  // returns active game (nullptr = in menu)
  void  draw();
  void  returnToMenu();

  bool     inGame()           const { return _inGame; }
  uint16_t gameAccentColor(int idx) const;  // RGB565 per-game accent

private:
  TFT_eSPI*     _tft    = nullptr;
  AssetManager* _assets = nullptr;
  ScoreManager* _scores = nullptr;
  Game*         _games[MAX_GAMES] = {};
  int           _count  = 0;
  bool          _inGame = false;
  bool          _paused = false;
  Game*         _active = nullptr;
  int           _activeIdx = -1;         // index of active game (for accent colour)
  bool          _needsRedraw = true;

  void drawHomepage();
  void drawHUD();
  void drawPauseOverlay();
};
#endif
```

- [ ] **Step 2: Replace src/core/Launcher.cpp**

```cpp
#include "Launcher.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"

// Row layout: header=24px + divider=1px + 10×28px rows + footer=15px = 320px exact
static const int ROW_H  = 28;
static const int ROW_Y0 = 25;   // y of first row top edge (0..24 = header)
static const int HUD_H  = 22;   // height of in-game HUD strip

// Per-game accent colours in registration order
static const uint16_t GAME_COLORS[10] = {
  Theme::SNAKE565,    // 0 Snake
  Theme::PONG565,     // 1 Pong
  Theme::SIMON565,    // 2 Simon
  Theme::MINES565,    // 3 Minesweeper
  Theme::G2048565,    // 4 2048
  Theme::BREAKOUT565, // 5 Breakout
  Theme::FLAPPY565,   // 6 Flappy Bird
  Theme::TETRIS565,   // 7 Tetris
  Theme::INVADERS565, // 8 Space Invaders
  Theme::PACMAN565,   // 9 Pac-Man
};

uint16_t Launcher::gameAccentColor(int idx) const {
  if (idx < 0 || idx >= 10) return Theme::ACCENT565;
  return GAME_COLORS[idx];
}

void Launcher::begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores) {
  _tft = &tft; _assets = &assets; _scores = &scores;
  _needsRedraw = true;
}

void Launcher::addGame(Game* g) {
  if (_count < MAX_GAMES) _games[_count++] = g;
}

Game* Launcher::update(const InputEvent& evt) {
  // Always return _active while in-game so main loop can route input.
  if (_inGame) {
    // HUD strip tap (y < HUD_H) → pause
    if (evt.type == InputEvent::TAP && evt.y < HUD_H) {
      _paused = !_paused;
      _needsRedraw = true;
      return _active;
    }
    // Long-press anywhere → pause
    if (evt.type == InputEvent::LONG_PRESS) {
      _paused = true;
      _needsRedraw = true;
      return _active;
    }
    // Pause modal interaction
    if (_paused) {
      if (evt.type == InputEvent::TAP) {
        if (evt.y >= 130 && evt.y <= 162) {
          // RESUME
          _paused = false;
          _needsRedraw = false;
        } else if (evt.y >= 170 && evt.y <= 202) {
          // QUIT
          returnToMenu();
          return nullptr;
        }
      }
      return _active;
    }
    return _active;
  }

  if (_count == 0) return nullptr;

  if (evt.type == InputEvent::TAP) {
    int row = (int)(evt.y - ROW_Y0) / ROW_H;
    if (evt.y >= ROW_Y0 && row >= 0 && row < _count) {
      _active    = _games[row];
      _activeIdx = row;
      _inGame    = true;
      _paused    = false;
      _active->begin(*_tft, *_assets, *_scores);
      return _active;
    }
  }
  return nullptr;
}

void Launcher::draw() {
  if (_inGame) {
    if (_active) _active->draw();
    drawHUD();
    if (_paused) drawPauseOverlay();
    return;
  }
  if (!_needsRedraw) return;
  _needsRedraw = false;
  drawHomepage();
}

void Launcher::drawHomepage() {
  TFT_eSPI& t = *_tft;
  t.fillScreen(Theme::BG565);

  // Header
  t.setTextColor(Theme::TEXT565, Theme::BG565);
  t.drawString("PAPER ARCADE", 8, 6, 2);
  t.setTextColor(Theme::MUTED565, Theme::BG565);
  char countBuf[12];
  snprintf(countBuf, sizeof(countBuf), "%d GAMES", _count);
  int cw = t.textWidth(countBuf, 1);
  t.drawString(countBuf, 232 - cw, 8, 1);
  // Blue divider
  t.drawFastHLine(0, 23, 240, Theme::ACCENT565);

  // Game rows
  for (int i = 0; i < _count; i++) {
    int rowY = ROW_Y0 + i * ROW_H;
    uint16_t gc = gameAccentColor(i);

    // Game name in its accent colour
    t.setTextColor(gc, Theme::BG565);
    t.drawString(_games[i]->name(), 8, rowY + 4, 4);

    // Score: white if > 0, muted "--" if 0
    uint32_t hi = _games[i]->highScore();
    char buf[16];
    if (hi > 0) {
      snprintf(buf, sizeof(buf), "%lu", (unsigned long)hi);
      t.setTextColor(Theme::TEXT565, Theme::BG565);
    } else {
      snprintf(buf, sizeof(buf), "--");
      t.setTextColor(Theme::MUTED565, Theme::BG565);
    }
    int bw = t.textWidth(buf, 2);
    t.drawString(buf, 232 - bw, rowY + 7, 2);

    // Row separator
    t.drawFastHLine(0, rowY + ROW_H - 1, 240, Theme::SEP565);
  }

  // Footer hint
  t.setTextColor(Theme::MUTED565, Theme::BG565);
  int fw = t.textWidth("TAP || TO PAUSE", 1);
  t.drawString("TAP || TO PAUSE", 120 - fw / 2, 308, 1);
}

void Launcher::drawHUD() {
  if (!_active || !_inGame) return;
  TFT_eSPI& t = *_tft;
  uint16_t gc = gameAccentColor(_activeIdx);

  // Clear HUD zone
  t.fillRect(0, 0, 240, HUD_H, Theme::BG565);

  // Game name in accent colour
  t.setTextColor(gc, Theme::BG565);
  t.drawString(_active->name(), 6, 5, 2);

  // Live score in white
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)_active->score());
  t.setTextColor(Theme::TEXT565, Theme::BG565);
  int sw = t.textWidth(buf, 2);
  t.drawString(buf, 200 - sw, 5, 2);

  // Pause icon "||" in muted
  t.setTextColor(Theme::MUTED565, Theme::BG565);
  t.drawString("||", 218, 5, 2);

  // Accent underline
  t.drawFastHLine(0, HUD_H - 2, 240, gc);
}

void Launcher::drawPauseOverlay() {
  TFT_eSPI& t = *_tft;
  // Dark fill below HUD
  t.fillRect(0, HUD_H, 240, 320 - HUD_H, Theme::CARD565);

  // Modal card
  t.fillRoundRect(30, 100, 180, 110, 8, Theme::CARD565);
  t.drawRoundRect(30, 100, 180, 110, 8, Theme::ACCENT565);

  // "PAUSED"
  t.setTextColor(Theme::MUTED565, Theme::CARD565);
  int pw = t.textWidth("PAUSED", 1);
  t.drawString("PAUSED", 120 - pw / 2, 113, 1);

  // RESUME button
  t.fillRoundRect(46, 130, 148, 32, 5, Theme::ACCENT565);
  t.setTextColor(Theme::TEXT565, Theme::ACCENT565);
  int rw = t.textWidth("RESUME", 2);
  t.drawString("RESUME", 120 - rw / 2, 139, 2);

  // QUIT button
  t.drawRoundRect(46, 170, 148, 32, 5, Theme::SEP565);
  t.setTextColor(Theme::DANGER565, Theme::CARD565);
  int qw = t.textWidth("QUIT TO MENU", 2);
  t.drawString("QUIT TO MENU", 120 - qw / 2, 179, 2);
}

void Launcher::returnToMenu() {
  if (_active) { _active->end(); _active = nullptr; }
  _inGame    = false;
  _paused    = false;
  _activeIdx = -1;
  _needsRedraw = true;
}

#endif
```

- [ ] **Step 3: Update paper-arcade.ino loop to not forward HUD-tap to games**

The main loop currently guards on `justLaunched`. Add a guard for `isLongPress` (already exists). The HUD-tap is now consumed inside `Launcher::update()` — it never returns a null when in-game, so no change to main loop needed. Verify the current loop handles this correctly:

```cpp
// In loop(): this is the critical path.
// launcher.update() returns _active on HUD tap (pause handled inside).
// So activeGame is still non-null; justLaunched is false (previous frame had activeGame).
// The HUD-tap event is consumed inside Launcher::update() — active->update() is NOT called
// because Launcher returns early when _paused == true.
// No change to main loop needed.
```

- [ ] **Step 4: Verify compile (will still fail due to missing score() in games)**

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 . 2>&1 | grep "error:" | head -5
```

Expected: errors about `score()` not declared — that's fine, games will fix this in Tasks 6–15.

- [ ] **Step 5: Commit**

```bash
git add src/core/Launcher.h src/core/Launcher.cpp
git commit -m "feat: Launcher — vertical list homepage, composited HUD strip, tap-|| pause"
```

---

### Task 4: Splash screen

**Files:**
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Update splash in setup()**

Find the splash block in `setup()` and replace:

```cpp
  tft.setTextColor(tft.color24to16(Theme::ACCENT), tft.color24to16(Theme::BG));
  tft.drawString("PAPER", 65, 130, 6);
  tft.drawString("ARCADE", 60, 180, 6);
  delay(800);
```

Replace with:

```cpp
  tft.fillScreen(Theme::BG565);
  tft.setTextColor(Theme::TEXT565, Theme::BG565);
  int sw = tft.textWidth("PAPER ARCADE", 4);
  tft.drawString("PAPER ARCADE", 120 - sw / 2, 146, 4);
  // Short blue underline
  tft.fillRect(105, 178, 30, 2, Theme::ACCENT565);
  delay(800);
```

Also update the OTA mode and WiFi failure screens to use new colour names (`Theme::BG565`, `Theme::TEXT565`, etc. instead of `tft.color24to16(Theme::BG)` etc):

```cpp
void enterOTAMode() {
  tft.fillScreen(Theme::BG565);
  tft.setTextColor(Theme::ACCENT565, Theme::BG565);
  tft.drawString("OTA MODE", 50, 80, 4);
  WiFi.mode(WIFI_STA);
  WiFi.begin();
  uint32_t t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 10000) delay(200);
  tft.setTextColor(Theme::TEXT565, Theme::BG565);
  if (WiFi.status() == WL_CONNECTED) {
    tft.drawString(WiFi.localIP().toString(), 30, 150, 2);
    ArduinoOTA.setHostname("paper-arcade");
    ArduinoOTA.begin();
    tft.setTextColor(Theme::MUTED565, Theme::BG565);
    tft.drawString("Ready for upload", 30, 180, 2);
    while (true) ArduinoOTA.handle();
  } else {
    tft.setTextColor(Theme::DANGER565, Theme::BG565);
    tft.drawString("WiFi failed", 50, 150, 2);
    tft.setTextColor(Theme::MUTED565, Theme::BG565);
    tft.drawString("Set creds via Serial", 25, 180, 2);
    delay(3000);
    ESP.restart();
  }
}
```

- [ ] **Step 2: Commit**

```bash
git add paper-arcade.ino
git commit -m "feat: splash screen and OTA mode with new Minimal Premium palette"
```

---

### Task 5: Snake — score(), play area shift, unified game-over

**Files:**
- Modify: `src/games/Snake.h`
- Modify: `src/games/Snake.cpp`

- [ ] **Step 1: Add score() to Snake.h**

In the `Snake` class private section, `_hiScore` is already there. Add the override to the public section after `highScore()`:

```cpp
  const char* name()      override { return "SNAKE"; }
  uint32_t    highScore() override { return _hiScore; }
  uint32_t    score()     override { return _logic.score(); }
  bool        isDone()    override { return _done; }
```

Also remove `bool _doneScreenShown = false;` — no longer needed.

- [ ] **Step 2: Update Snake.cpp draw()**

The grid currently starts at y=0 (`body[i].y * CELL`). Add `+22` to all y-coordinates, and replace the HUD and game-over rendering with the unified helper.

Full new `Snake::draw()`:

```cpp
void Snake::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_done) {
    drawGameOverOverlay(s, "SNAKE", _logic.score(), Theme::SNAKE565);
    return;
  }

  s.fillScreen(Theme::BG565);

  // Food — shifted +22 in y
  Point f = _logic.foodPos();
  s.fillRect(f.x * CELL + 2, f.y * CELL + 2 + 22, CELL - 4, CELL - 4, Theme::ACCENT565);

  // Snake body — head in accent, body in a slightly dimmer green
  const auto& body = _logic.body();
  for (size_t i = 0; i < body.size(); i++) {
    uint16_t c = (i == 0) ? Theme::SNAKE565 : 0x1B05;  // head bright, body dark green
    s.fillRect(body[i].x * CELL + 1, body[i].y * CELL + 1 + 22, CELL - 2, CELL - 2, c);
  }
}
```

Also update `Snake::begin()` — remove `_doneScreenShown = false;`.

Also add the include at the top of Snake.cpp (after `#include "../ui/Theme.h"`):

```cpp
#include "../ui/GameOver.h"
```

- [ ] **Step 3: Run native tests — no change to logic**

```bash
cd "/Users/RoyKorkomaz/all cloned from github/paper-arcade"
make test_snake
```

Expected: `20/20 passed`

- [ ] **Step 4: Commit**

```bash
git add src/games/Snake.h src/games/Snake.cpp
git commit -m "feat(snake): score(), play area y+22, unified game-over overlay"
```

---

### Task 6: Pong — score(), play area adjust, unified game-over

**Files:**
- Modify: `src/games/Pong.h`
- Modify: `src/games/Pong.cpp`

- [ ] **Step 1: Add score() to Pong.h**

In `PongLogic`, `playerScore()` returns uint8_t. The in-game HUD shows a score; for Pong we'll use `playerScore() * 100`.

Add to `Pong` public section:

```cpp
  uint32_t    score()     override { return (uint32_t)_logic.playerScore() * 100; }
```

Also change `AI_Y` in `PongLogic` from 18 to 24 (moves AI paddle below the HUD strip):

In `Pong.h`, change:
```cpp
  static const int16_t PLAYER_Y = 290, AI_Y = 18;
```
to:
```cpp
  static const int16_t PLAYER_Y = 290, AI_Y = 24;
```

- [ ] **Step 2: Update Pong.cpp draw()**

Add `#include "../ui/GameOver.h"` after the Theme include.

Replace `Pong::draw()`:

```cpp
void Pong::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_done) {
    bool won = _logic.playerScore() >= PongLogic::WIN_SCORE;
    drawGameOverOverlay(s, "PONG", (uint32_t)_logic.playerScore() * 100, Theme::PONG565, won);
    return;
  }

  s.fillScreen(Theme::BG565);

  // Centre dashed line (horizontal for portrait Pong)
  for (int x = 0; x < 240; x += 14)
    s.drawFastHLine(x, 160, 8, Theme::SEP565);

  // AI paddle (top, sky blue / game colour)
  s.fillRect(_logic.aiX(), PongLogic::AI_Y, PongLogic::PAD_W, PongLogic::PAD_H, Theme::PONG565);
  // Player paddle (bottom, white)
  s.fillRect(_logic.playerX(), PongLogic::PLAYER_Y, PongLogic::PAD_W, PongLogic::PAD_H, Theme::TEXT565);
  // Ball
  s.fillCircle(_logic.ballX(), _logic.ballY(), PongLogic::BALL_R, Theme::TEXT565);

  // Scores centred
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", _logic.aiScore());
  s.setTextColor(Theme::MUTED565, Theme::BG565);
  int aw = s.textWidth(buf, 4);
  s.drawString(buf, 60 - aw / 2, 130, 4);
  snprintf(buf, sizeof(buf), "%d", _logic.playerScore());
  s.setTextColor(Theme::TEXT565, Theme::BG565);
  int pw = s.textWidth(buf, 4);
  s.drawString(buf, 180 - pw / 2, 130, 4);
}
```

- [ ] **Step 3: Run tests**

```bash
make test_pong
```

Expected: `6/6 passed`

- [ ] **Step 4: Commit**

```bash
git add src/games/Pong.h src/games/Pong.cpp
git commit -m "feat(pong): score(), AI paddle below HUD, unified game-over"
```

---

### Task 7: Simon Says — score(), unified game-over

**Files:**
- Modify: `src/games/SimonSays.h`
- Modify: `src/games/SimonSays.cpp`

- [ ] **Step 1: Add score() to SimonSays.h**

```cpp
  uint32_t    score()     override { return (uint32_t)_logic.score(); }
```

- [ ] **Step 2: Update SimonSays.cpp draw()**

Add `#include "../ui/GameOver.h"` after Theme include.

Replace the `_phase == FAIL` overlay in `draw()` with the unified overlay, and let full 0..319 paint for the quadrants (HUD will composite on top):

```cpp
void SimonSays::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_done || _phase == FAIL) {
    drawGameOverOverlay(s, "SIMON", (uint32_t)_logic.score(), Theme::SIMON565);
    return;
  }

  // Draw 4 quadrants — full screen including y<22 (HUD will composite on top)
  for (int8_t i = 0; i < 4; i++) {
    int qx = (i % 2) * 120;
    int qy = (i / 2) * 160;
    uint16_t col = (_flashColor == i) ? QUAD_BRIGHT[i] : QUAD_DIM[i];
    s.fillRect(qx + 2, qy + 2, 116, 156, col);
  }

  // Score in centre circle
  s.fillCircle(120, 160, 28, Theme::BG565);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", _logic.score());
  s.setTextColor(Theme::TEXT565, Theme::BG565);
  int sw = s.textWidth(buf, 4);
  s.drawString(buf, 120 - sw / 2, 150, 4);
}
```

Also update `update()`: when `_phase == FAIL && isDone tap` set `_done = true` instead of existing pattern.

And update `isDone()` — currently `_done` — now also trigger on FAIL + tap. In `update()`:

```cpp
  if (_phase == FAIL && input.type == InputEvent::TAP && now - _phaseStart > 500) {
    _scores->setHighScore("simon", (uint32_t)_logic.score());
    _hiScore = _scores->getHighScore("simon");
    _done = true;
    _dirty = true;
  }
```

- [ ] **Step 3: Run tests**

```bash
make test_simon
```

Expected: `6/6 passed`

- [ ] **Step 4: Commit**

```bash
git add src/games/SimonSays.h src/games/SimonSays.cpp
git commit -m "feat(simon): score(), unified game-over, full-screen quadrants"
```

---

### Task 8: Minesweeper — score(), grid shift y+22, unified game-over

**Files:**
- Modify: `src/games/Minesweeper.h`
- Modify: `src/games/Minesweeper.cpp`

- [ ] **Step 1: Add score() to Minesweeper.h**

```cpp
  uint32_t    score()     override { return (uint32_t)_logic.revealedCount() * 10; }
```

- [ ] **Step 2: Update Minesweeper.cpp draw()**

Grid currently at `y = row * CELL` (CELL=24). Shift to `y = row * CELL + 22`.

Add `#include "../ui/GameOver.h"`.

Replace `Minesweeper::draw()`:

```cpp
void Minesweeper::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_done || _logic.isExploded() || _logic.isWon()) {
    bool won = _logic.isWon();
    uint32_t finalScore = won ? 1000 + (uint32_t)_logic.revealedCount() * 10
                               : (uint32_t)_logic.revealedCount() * 10;
    drawGameOverOverlay(s, "MINES", finalScore, Theme::MINES565, won);
    return;
  }

  s.fillScreen(Theme::BG565);

  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      int px = x * CELL;
      int py = y * CELL + 22;  // shifted +22
      if (_logic.isRevealed(x, y)) {
        s.fillRect(px + 1, py + 1, CELL - 2, CELL - 2, Theme::SEP565);
        if (_logic.isMine(x, y)) {
          s.fillCircle(px + CELL / 2, py + CELL / 2, 7, Theme::DANGER565);
        } else {
          uint8_t n = _logic.neighborCount(x, y);
          if (n > 0) {
            static const uint16_t NUM_COLS[9] = {
              0x0000, Theme::ACCENT565, 0x07E0, Theme::DANGER565,
              0x780F, 0x7980, 0x4E5E, 0x0000, 0x7BEF
            };
            char c[2] = { (char)('0' + n), 0 };
            s.setTextColor(NUM_COLS[n], Theme::SEP565);
            s.drawString(c, px + 7, py + 4, 2);
          }
        }
      } else {
        s.fillRect(px + 1, py + 1, CELL - 2, CELL - 2, Theme::CARD565);
        if (_logic.isFlagged(x, y))
          s.fillTriangle(px + 7, py + 6, px + 18, py + 11, px + 7, py + 16, Theme::MINES565);
      }
      s.drawRect(px, py, CELL, CELL, Theme::SEP565);
    }
  }

  // Double-tap hint at bottom
  s.setTextColor(Theme::MUTED565, Theme::BG565);
  int hw = s.textWidth("DBL-TAP = FLAG", 1);
  s.drawString("DBL-TAP = FLAG", 120 - hw / 2, 296, 1);
}
```

Also update `update()` to save score and set `_done = true` when game over (instead of `_done = true` directly):

```cpp
  if (_logic.isExploded() || _logic.isWon()) {
    if (input.type == InputEvent::TAP) {
      uint32_t sc = _logic.isWon() ? 1000 + (uint32_t)_logic.revealedCount() * 10
                                    : (uint32_t)_logic.revealedCount() * 10;
      _scores->setHighScore("mines", sc);
      _hiScore = _scores->getHighScore("mines");
      _done = true;
    }
    return;
  }
```

- [ ] **Step 3: Run tests**

```bash
make test_minesweeper
```

Expected: `6/6 passed`

- [ ] **Step 4: Commit**

```bash
git add src/games/Minesweeper.h src/games/Minesweeper.cpp
git commit -m "feat(minesweeper): score(), grid y+22, unified game-over"
```

---

### Task 9: 2048 — score(), remove own HUD, unified game-over

**Files:**
- Modify: `src/games/Game2048.h`
- Modify: `src/games/Game2048.cpp`

- [ ] **Step 1: Add score() to Game2048.h**

```cpp
  uint32_t    score()     override { return _logic.score(); }
```

- [ ] **Step 2: Update Game2048.cpp draw()**

Add `#include "../ui/GameOver.h"`.

The grid currently has `OX=4, OY=40`. Move `OY` to `62` (40+22) so the grid clears the HUD zone.

Replace `Game2048::draw()`:

```cpp
void Game2048::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_gameOver) {
    drawGameOverOverlay(s, "2048", _logic.score(), Theme::G2048565);
    return;
  }

  s.fillScreen(Theme::BG565);

  // Grid: CELL=54, GAP=3, OX=4, OY=62
  const int CELL = 54, GAP = 3, OX = 4, OY = 62;
  s.fillRoundRect(OX - 3, OY - 3, 4 * (CELL + GAP) + GAP, 4 * (CELL + GAP) + GAP, 5, Theme::SEP565);

  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      int px = OX + GAP + x * (CELL + GAP);
      int py = OY + GAP + y * (CELL + GAP);
      uint16_t v = _logic.cell(x, y);
      uint16_t col = v ? tileColor(v) : Theme::CARD565;
      s.fillRoundRect(px, py, CELL, CELL, 3, col);
      if (v) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", v);
        s.setTextColor(Theme::TEXT565, col);
        int font = (v < 1000) ? 4 : 2;
        int tw = s.textWidth(buf, font);
        s.drawString(buf, px + (CELL - tw) / 2, py + CELL / 2 - 10, font);
      }
    }
  }

  // Swipe hint
  s.setTextColor(Theme::MUTED565, Theme::BG565);
  int hw = s.textWidth("swipe to move", 1);
  s.drawString("swipe to move", 120 - hw / 2, 310, 1);
}
```

- [ ] **Step 3: Run tests**

```bash
make test_2048
```

Expected: `10/10 passed`

- [ ] **Step 4: Commit**

```bash
git add src/games/Game2048.h src/games/Game2048.cpp
git commit -m "feat(2048): score(), grid OY+22, unified game-over"
```

---

### Task 10: Breakout — score(), bricks shift, lives dots in play area, unified game-over

**Files:**
- Modify: `src/games/Breakout.h`
- Modify: `src/games/Breakout.cpp`

- [ ] **Step 1: Add score() to Breakout.h**

```cpp
  uint32_t    score()     override { return _score; }
```

Change `BRICK_TOP` from 30 to 52 (shifted +22):

```cpp
  static const int BRICK_TOP = 52;
```

- [ ] **Step 2: Update Breakout.cpp draw()**

Add `#include "../ui/GameOver.h"`.

Replace `Breakout::draw()`:

```cpp
void Breakout::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_gameOver) {
    bool won = (_remaining == 0);
    drawGameOverOverlay(s, "BREAKOUT", _score, Theme::BREAKOUT565, won);
    return;
  }

  s.fillScreen(Theme::BG565);

  // Bricks (BRICK_TOP = 52 now)
  static const uint16_t ROW_COLS[6] = {
    Theme::DANGER565, Theme::MINES565, Theme::G2048565,
    Theme::FLAPPY565, Theme::SNAKE565, Theme::BREAKOUT565
  };
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      if (!_bricks[r * COLS + c]) continue;
      s.fillRect(c * (BRICK_W + 2), BRICK_TOP + r * BRICK_H, BRICK_W, BRICK_H - 2, ROW_COLS[r]);
    }
  }

  // Paddle
  s.fillRoundRect(_paddleX, 295, PADDLE_W, PADDLE_H, 3, Theme::BREAKOUT565);

  // Ball
  s.fillCircle((int)_bx, (int)_by, 5, Theme::TEXT565);

  // Lives dots at y=310
  for (int i = 0; i < 3; i++) {
    uint16_t col = (i < (int)_lives) ? Theme::BREAKOUT565 : Theme::SEP565;
    s.fillCircle(108 + i * 14, 310, 4, col);
  }
}
```

- [ ] **Step 3: Commit**

```bash
git add src/games/Breakout.h src/games/Breakout.cpp
git commit -m "feat(breakout): score(), bricks y+22, lives dots, unified game-over"
```

---

### Task 11: Flappy Bird — score(), unified game-over

**Files:**
- Modify: `src/games/FlappyBird.h`
- Modify: `src/games/FlappyBird.cpp`

- [ ] **Step 1: Add score() to FlappyBird.h**

```cpp
  uint32_t    score()     override { return _score; }
```

- [ ] **Step 2: Update FlappyBird.cpp draw()**

Add `#include "../ui/GameOver.h"`.

Replace `FlappyBird::draw()`:

```cpp
void FlappyBird::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_gameOver) {
    drawGameOverOverlay(s, "FLAPPY", _score, Theme::FLAPPY565);
    return;
  }

  s.fillScreen(Theme::BG565);

  // Pipes — full height, HUD will composite on top
  for (int i = 0; i < PIPE_COUNT; i++) {
    Pipe& p = _pipes[i];
    s.fillRect(p.x, 0,            PIPE_W, p.gapY,                       Theme::SNAKE565);
    s.fillRect(p.x, p.gapY + GAP_H, PIPE_W, 320 - (p.gapY + GAP_H),   Theme::SNAKE565);
    s.drawRect(p.x, 0,            PIPE_W, p.gapY,                       Theme::FLAPPY565);
    s.drawRect(p.x, p.gapY + GAP_H, PIPE_W, 320 - (p.gapY + GAP_H),   Theme::FLAPPY565);
  }

  // Bird
  s.fillCircle(BIRD_X, (int)_birdY, 8, Theme::FLAPPY565);
  s.fillCircle(BIRD_X + 3, (int)_birdY - 2, 2, Theme::BG565);
}
```

- [ ] **Step 3: Commit**

```bash
git add src/games/FlappyBird.h src/games/FlappyBird.cpp
git commit -m "feat(flappy): score(), unified game-over"
```

---

### Task 12: Tetris — score(), OFFY shift, unified game-over

**Files:**
- Modify: `src/games/Tetris.h`
- Modify: `src/games/Tetris.cpp`

- [ ] **Step 1: Add score() to Tetris.h**

```cpp
  uint32_t    score()     override { return _logic.score(); }
```

Change `OFFY` from 30 to 52:

```cpp
  static const int CELL = 11, OFFX = 65, OFFY = 52;
```

- [ ] **Step 2: Update Tetris.cpp draw()**

Add `#include "../ui/GameOver.h"`.

Replace `Tetris::draw()`:

```cpp
void Tetris::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_logic.gameOver()) {
    drawGameOverOverlay(s, "TETRIS", _logic.score(), Theme::TETRIS565);
    return;
  }

  s.fillScreen(Theme::BG565);

  // Board border
  s.drawRect(OFFX - 1, OFFY - 1,
             TetrisLogic::W * CELL + 2, TetrisLogic::H * CELL + 2, Theme::TETRIS565);

  // Stack
  for (int y = 0; y < TetrisLogic::H; y++)
    for (int x = 0; x < TetrisLogic::W; x++) {
      uint8_t v = _logic.cell(x, y);
      if (v) s.fillRect(OFFX + x * CELL, OFFY + y * CELL, CELL - 1, CELL - 1, PIECE_COLORS[v]);
    }

  // Active piece
  int bx[4], by[4];
  _logic.pieceBlocks(bx, by);
  uint16_t pc = PIECE_COLORS[_logic.activePieceType() + 1];
  for (int i = 0; i < 4; i++) {
    if (by[i] < 0) continue;
    s.fillRect(OFFX + bx[i] * CELL, OFFY + by[i] * CELL, CELL - 1, CELL - 1, pc);
  }

  // Sidebar score
  s.setTextColor(Theme::MUTED565, Theme::BG565);
  s.drawString("SCORE", 178, 52, 2);
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)_logic.score());
  s.setTextColor(Theme::TEXT565, Theme::BG565);
  s.drawString(buf, 178, 70, 2);

  // Control hints
  s.setTextColor(Theme::SEP565, Theme::BG565);
  s.drawString("ROT", 4, 55, 2);
  s.drawString("<", 10, 140, 4);
  s.drawString(">", 180, 140, 4);
  s.drawString("DROP", 88, 280, 2);
}
```

- [ ] **Step 3: Run tests**

```bash
make test_tetris
```

Expected: `7/7 passed`

- [ ] **Step 4: Commit**

```bash
git add src/games/Tetris.h src/games/Tetris.cpp
git commit -m "feat(tetris): score(), OFFY+22, unified game-over"
```

---

### Task 13: Space Invaders — score(), adjust swarm start, lives dots, unified game-over

**Files:**
- Modify: `src/games/SpaceInvaders.h`
- Modify: `src/games/SpaceInvaders.cpp`

- [ ] **Step 1: Add score() to SpaceInvaders.h**

```cpp
  uint32_t    score()     override { return _score; }
```

Change `_swarmY` initial value from 25 to 35 in `begin()` so the swarm starts below the HUD strip.

- [ ] **Step 2: Update SpaceInvaders.cpp draw()**

Add `#include "../ui/GameOver.h"`.

Replace `SpaceInvaders::draw()`:

```cpp
void SpaceInvaders::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_gameOver) {
    bool won = (_aliveCount == 0);
    drawGameOverOverlay(s, "INVADERS", _score, Theme::INVADERS565, won);
    return;
  }

  s.fillScreen(Theme::BG565);

  // Invaders
  static const uint16_t ROW_COLS[4] = {
    Theme::DANGER565, Theme::MINES565, Theme::SNAKE565, Theme::TETRIS565
  };
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      if (!_alive[r * COLS + c]) continue;
      int ix = _swarmX + c * (INV_W + INV_GAP_X);
      int iy = _swarmY + r * (INV_H + INV_GAP_Y);
      s.fillRect(ix, iy, INV_W, INV_H, ROW_COLS[r]);
      s.fillRect(ix + 4, iy + 3, 3, 3, Theme::BG565);
      s.fillRect(ix + 15, iy + 3, 3, 3, Theme::BG565);
    }
  }

  // Player ship
  s.fillRect(_playerX, PLAYER_Y, PLAYER_W, 10, Theme::INVADERS565);
  s.fillRect(_playerX + 9, PLAYER_Y - 5, 4, 5, Theme::INVADERS565);

  // Bullets
  for (int i = 0; i < MAX_BULLETS; i++)
    if (_bullets[i].active)
      s.fillRect(_bullets[i].x, _bullets[i].y, 2, 7, Theme::TEXT565);
  for (int i = 0; i < MAX_INV_BULLETS; i++)
    if (_invBullets[i].active)
      s.fillRect(_invBullets[i].x, _invBullets[i].y, 2, 7, Theme::DANGER565);

  // Lives dots at y=310
  for (int i = 0; i < 3; i++) {
    uint16_t col = (i < (int)_lives) ? Theme::INVADERS565 : Theme::SEP565;
    s.fillCircle(108 + i * 14, 310, 4, col);
  }

  // Control zone hints (barely visible)
  s.setTextColor(Theme::SEP565, Theme::BG565);
  s.drawString("<", 14, 290, 4);
  s.drawString(">", 210, 290, 4);
  s.drawString("FIRE", 98, 292, 2);
}
```

Also update `begin()`: change `_swarmY = 25;` to `_swarmY = 35;`.

- [ ] **Step 3: Commit**

```bash
git add src/games/SpaceInvaders.h src/games/SpaceInvaders.cpp
git commit -m "feat(invaders): score(), swarm y+10, lives dots, unified game-over"
```

---

### Task 14: Pac-Man — score(), remove header, unified game-over

**Files:**
- Modify: `src/games/PacMan.h`
- Modify: `src/games/PacMan.cpp`

- [ ] **Step 1: Add score() to PacMan.h**

```cpp
  uint32_t    score()     override { return _score; }
```

`MAZE_Y` is already 48. The HUD is at y=0..21, so the maze starting at y=48 is safely below it. No shift needed.

- [ ] **Step 2: Update PacMan.cpp draw()**

Add `#include "../ui/GameOver.h"`.

Pac-Man currently draws a score/lives header at y=0..45. Remove it — the Launcher HUD handles score now.

Replace `PacMan::draw()`:

```cpp
void PacMan::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_gameOver) {
    bool won = (_dotsLeft == 0);
    drawGameOverOverlay(s, "PAC-MAN", _score, Theme::PACMAN565, won);
    return;
  }

  s.fillScreen(Theme::BG565);

  // Maze (MAZE_Y = 48, below Launcher HUD at y=0..21)
  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int px = x * TILE;
      int py = MAZE_Y + y * TILE;
      uint8_t v = _grid[y][x];
      if (v == 1)      s.fillRect(px, py, TILE, TILE, Theme::PACMAN565);
      else if (v == 0) s.fillCircle(px + TILE / 2, py + TILE / 2, 2, Theme::MUTED565);
      else if (v == 2) s.fillCircle(px + TILE / 2, py + TILE / 2, 5, Theme::ACCENT565);
    }
  }

  // Pac-Man
  int px = _pac.x * TILE + dirDx(_pac.dir) * _pac.pix + TILE / 2;
  int py = MAZE_Y + _pac.y * TILE + dirDy(_pac.dir) * _pac.pix + TILE / 2;
  s.fillCircle(px, py, 6, Theme::FLAPPY565);

  // Ghosts
  static const uint16_t GH[3] = { Theme::DANGER565, Theme::G2048565, Theme::TETRIS565 };
  for (int i = 0; i < 3; i++) {
    int gx = _ghosts[i].x * TILE + dirDx(_ghosts[i].dir) * _ghosts[i].pix + TILE / 2;
    int gy = MAZE_Y + _ghosts[i].y * TILE + dirDy(_ghosts[i].dir) * _ghosts[i].pix + TILE / 2;
    s.fillCircle(gx, gy, 6, _frightened ? Theme::ACCENT565 : GH[i]);
  }

  // Lives dots at y=310
  for (int i = 0; i < 3; i++) {
    uint16_t col = (i < (int)_lives) ? Theme::PACMAN565 : Theme::SEP565;
    s.fillCircle(108 + i * 14, 310, 4, col);
  }

  // D-pad arrows at bottom
  s.setTextColor(Theme::SEP565, Theme::BG565);
  s.fillTriangle(120, 226, 112, 238, 128, 238, Theme::SEP565);
  s.fillTriangle(120, 318, 112, 306, 128, 306, Theme::SEP565);
  s.fillTriangle(14, 272, 26, 264, 26, 280, Theme::SEP565);
  s.fillTriangle(226, 272, 214, 264, 214, 280, Theme::SEP565);
}
```

- [ ] **Step 3: Commit**

```bash
git add src/games/PacMan.h src/games/PacMan.cpp
git commit -m "feat(pacman): score(), remove header, lives dots, unified game-over"
```

---

### Task 15: Final compile + flash

**Files:** None modified — verify everything compiles and works.

- [ ] **Step 1: Compile**

```bash
cd "/Users/RoyKorkomaz/all cloned from github/paper-arcade"
arduino-cli compile --fqbn esp32:esp32:esp32 . 2>&1 | tail -5
```

Expected:
```
Sketch uses XXXXXXX bytes (XX%) of program storage space. Maximum is 1310720 bytes.
Global variables use XXXXX bytes (XX%) of dynamic memory...
```
No errors.

- [ ] **Step 2: Run all native tests**

```bash
make test_input test_snake test_pong test_simon test_minesweeper test_2048 test_tetris
```

Expected: all pass.

- [ ] **Step 3: Flash**

```bash
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/cu.wchusbserial1120 .
```

- [ ] **Step 4: Manual hardware verification checklist**

On device:
- [ ] Splash: "PAPER ARCADE" white centred, short blue underline, black bg
- [ ] Launcher: all 10 game names visible in bold accent colours on black, score right
- [ ] Tap Snake → game plays below the HUD strip; "SNAKE" + live score in top strip
- [ ] Tap `||` in top strip → pause overlay appears; RESUME (blue) / QUIT (red)
- [ ] Long-press anywhere in-game → same pause overlay
- [ ] Game over: full black screen, game name big in accent colour, score centred, "TAP ANYWHERE"
- [ ] Tap anywhere on game-over → returns to launcher, high score updated

- [ ] **Step 5: Push to GitHub**

```bash
git push origin main
```

---

## Self-Review Notes

- All 10 games add `score()` override — spec requirement ✓
- `gameAccentColor()` uses registration order (0=Snake, 1=Pong, ...) — matches `addGame()` call order in `paper-arcade.ino` ✓
- GameOver overlay draws from y=0..319 (full screen). Launcher composites HUD (y=0..21) on top. For game-over, the HUD still shows (last score, game name). This is correct ✓
- Pause overlay: `_dirty = false` is NOT called when paused — Launcher re-draws the overlay each frame. Since it's done directly in `draw()` with no dirty check, it stays visible ✓
- `isDone()` tap handling in `paper-arcade.ino` main loop: currently `if (activeGame->isDone() && evt.type == InputEvent::TAP && !justLaunched)`. When game is done, `_active->draw()` draws the game-over overlay, then Launcher draws HUD on top. User sees game-over + HUD. Tap anywhere → Launcher catches it in update() → `returnToMenu()`. The `isDone()` path in main loop fires simultaneously — this could double-trigger. Fix: `Launcher::update()` should handle `isDone()` returns directly. Add to `Launcher::update()` after `_active->draw()`:

Actually looking at main loop more carefully — `Launcher::update()` returns `_active` while in-game. Main loop then does `activeGame->update(evt)` and `if (activeGame->isDone() && evt.type == InputEvent::TAP && !justLaunched) { launcher.returnToMenu(); }`. This is fine — Launcher.update() doesn't get to see the same tap. No double trigger.

- Per-game `score()` implementations: verify each returns the LIVE score during gameplay (not just high score):
  - Snake: `_logic.score()` ✓ (updates during play)
  - Pong: `playerScore() * 100` — resets on `resetBall()`. Minor: will show 0..400 not continuous. Acceptable.
  - Simon: `_logic.score()` = `_len - 1` ✓
  - Minesweeper: `revealedCount() * 10` ✓
  - 2048: `_logic.score()` ✓
  - Breakout/Flappy/Invaders: `_score` field ✓
  - Tetris: `_logic.score()` ✓
  - PacMan: `_score` field ✓

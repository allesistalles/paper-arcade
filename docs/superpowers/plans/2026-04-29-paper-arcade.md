# Paper Arcade Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a 10-game arcade device on the ESP32 CYD with a Synthwave-themed swipe carousel launcher, persistent high scores, and OTA firmware updates.

**Architecture:** Modular C++ with an abstract `Game` base class. Each game lives in its own `.h/.cpp` pair and implements `begin/update/draw/end`. Pure game logic is separated from rendering for native unit testing. A `Launcher` hosts a `Game*[10]` array and routes input + render calls to the active game.

**Tech Stack:** Arduino ESP32 core, TFT_eSPI, XPT2046_Touchscreen, SD library, Preferences (NVS), ArduinoOTA, arduino-cli, g++ (native logic tests).

---

## File Map

| File | Responsibility |
|------|---------------|
| `paper-arcade.ino` | setup(), loop(), game registry |
| `User_Setup.h` | TFT_eSPI CYD pin configuration |
| `src/core/Game.h` | Abstract Game interface + InputEvent struct |
| `src/ui/Theme.h` | Synthwave palette constants |
| `src/core/InputManager.h/cpp` | Touch → InputEvent translation |
| `src/core/AssetManager.h/cpp` | SD BMP loader + LRU cache |
| `src/core/ScoreManager.h/cpp` | NVS high score read/write |
| `src/core/Launcher.h/cpp` | Swipe carousel UI + game router |
| `src/games/<Name>.h/cpp` | Per-game logic + rendering (10 files) |
| `tests/test_runner.h` | Minimal assert macros for native tests |
| `tests/test_<topic>.cpp` | Per-game logic tests |
| `Makefile` | Native test build targets |

---

## Execution Notes

- **Native tests** compile pure-logic classes with `-DNATIVE_TEST`, which guards out Arduino headers. Logic classes (e.g. `SnakeLogic`, `Tetris`) must not depend on `TFT_eSPI`, `millis()`, etc. — when needed, provide a stub under `#ifdef NATIVE_TEST`.
- **Compile commands** assume CYD is on `/dev/ttyUSB0`. Check with `arduino-cli board list` and substitute your port.
- **Each game task ends with manual hardware verification.** Flash the device, navigate to the game, play it, return to the launcher, and confirm the high score persists across power cycles.
- After Task 6 the launcher boots with an empty carousel — that's expected. Games are added one at a time in subsequent tasks.

---

### Task 1: Toolchain setup + project scaffolding

**Files:**
- Create: `User_Setup.h`
- Create: `paper-arcade.ino` (stub)
- Create: `Makefile`
- Create: `tests/test_runner.h`
- Create directories: `src/core/`, `src/games/`, `src/ui/`, `tests/bin/`, `data/sprites/`, `data/fonts/`

- [ ] **Step 1: Install arduino-cli, ESP32 core, and libraries**

```bash
# macOS
brew install arduino-cli

arduino-cli config init
arduino-cli config add board_manager.additional_urls https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
arduino-cli core update-index
arduino-cli core install esp32:esp32

arduino-cli lib install "TFT_eSPI"
arduino-cli lib install "XPT2046_Touchscreen"
```

Expected: all installs succeed. Run `arduino-cli core list` and verify `esp32:esp32` is listed.

- [ ] **Step 2: Create directory structure**

```bash
mkdir -p src/core src/games src/ui tests/bin data/sprites data/fonts
```

- [ ] **Step 3: Create User_Setup.h (CYD pin config)**

`User_Setup.h`:

```cpp
#define USER_SETUP_INFO "CYD ESP32-2432S028"
#define ILI9341_DRIVER

// CYD TFT pins
#define TFT_MOSI  13
#define TFT_MISO  12
#define TFT_SCLK  14
#define TFT_CS    15
#define TFT_DC     2
#define TFT_RST   -1
#define TFT_BL    21

// Touch (shared SPI bus)
#define TOUCH_CS  33

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define SMOOTH_FONT

#define SPI_FREQUENCY        40000000
#define SPI_READ_FREQUENCY   20000000
#define SPI_TOUCH_FREQUENCY   2500000
```

- [ ] **Step 4: Create stub paper-arcade.ino**

```cpp
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
```

- [ ] **Step 5: Create tests/test_runner.h**

```cpp
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cstring>

static int _tests_run = 0, _tests_failed = 0;

#define ASSERT_EQ(a, b) do { \
  _tests_run++; \
  if ((long long)(a) != (long long)(b)) { \
    printf("  FAIL %s:%d  expected %lld got %lld\n", __FILE__, __LINE__, (long long)(b), (long long)(a)); \
    _tests_failed++; \
  } \
} while(0)

#define ASSERT_TRUE(x)  ASSERT_EQ(!!(x), 1)
#define ASSERT_FALSE(x) ASSERT_EQ(!!(x), 0)

#define TEST_SUMMARY() do { \
  printf("%d/%d passed\n", _tests_run - _tests_failed, _tests_run); \
  return _tests_failed > 0 ? 1 : 0; \
} while(0)
```

- [ ] **Step 6: Create Makefile**

```makefile
CXX      := g++
CXXFLAGS := -std=c++17 -I src -I tests -DNATIVE_TEST -Wall
BIN      := tests/bin

$(BIN):
	mkdir -p $(BIN)

test_input: $(BIN) tests/test_input.cpp
	$(CXX) $(CXXFLAGS) tests/test_input.cpp -o $(BIN)/test_input
	$(BIN)/test_input

test_snake: $(BIN) tests/test_snake.cpp src/games/Snake.cpp
	$(CXX) $(CXXFLAGS) tests/test_snake.cpp src/games/Snake.cpp -o $(BIN)/test_snake
	$(BIN)/test_snake

test_pong: $(BIN) tests/test_pong.cpp src/games/Pong.cpp
	$(CXX) $(CXXFLAGS) tests/test_pong.cpp src/games/Pong.cpp -o $(BIN)/test_pong
	$(BIN)/test_pong

test_simon: $(BIN) tests/test_simon.cpp src/games/SimonSays.cpp
	$(CXX) $(CXXFLAGS) tests/test_simon.cpp src/games/SimonSays.cpp -o $(BIN)/test_simon
	$(BIN)/test_simon

test_minesweeper: $(BIN) tests/test_minesweeper.cpp src/games/Minesweeper.cpp
	$(CXX) $(CXXFLAGS) tests/test_minesweeper.cpp src/games/Minesweeper.cpp -o $(BIN)/test_minesweeper
	$(BIN)/test_minesweeper

test_2048: $(BIN) tests/test_2048.cpp src/games/Game2048.cpp
	$(CXX) $(CXXFLAGS) tests/test_2048.cpp src/games/Game2048.cpp -o $(BIN)/test_2048
	$(BIN)/test_2048

test_tetris: $(BIN) tests/test_tetris.cpp src/games/Tetris.cpp
	$(CXX) $(CXXFLAGS) tests/test_tetris.cpp src/games/Tetris.cpp -o $(BIN)/test_tetris
	$(BIN)/test_tetris

test: test_input test_snake test_pong test_simon test_minesweeper test_2048 test_tetris
	@echo "All native tests passed."

.PHONY: test
```

- [ ] **Step 7: Compile and flash stub**

```bash
arduino-cli board list
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

Expected: device shows "Paper Arcade" on a black screen, serial prints "Boot OK".

- [ ] **Step 8: Initialize git tracking and commit**

```bash
git add User_Setup.h paper-arcade.ino Makefile tests/test_runner.h
git commit -m "feat: project scaffolding, toolchain, CYD pin config"
```

---

### Task 2: Game interface, Theme, InputManager

**Files:**
- Create: `src/core/Game.h`
- Create: `src/ui/Theme.h`
- Create: `src/core/InputManager.h`
- Create: `src/core/InputManager.cpp`
- Create: `tests/test_input.cpp`

- [ ] **Step 1: Write tests/test_input.cpp**

```cpp
#include "test_runner.h"
#include <cstdint>

// Mirror of InputManager::classify — pure logic, no hardware deps.
// 0=NONE, 1=TAP, 2=SWIPE_LEFT, 3=SWIPE_RIGHT, 4=SWIPE_UP, 5=SWIPE_DOWN
int classify(int16_t dx, int16_t dy, uint32_t ms) {
  const int SWIPE_THRESH = 30;
  const int MAX_SWIPE_MS = 500;
  if (ms > MAX_SWIPE_MS) return 0;
  int ax = dx < 0 ? -dx : dx;
  int ay = dy < 0 ? -dy : dy;
  if (ax < SWIPE_THRESH && ay < SWIPE_THRESH) return 1;
  if (ax > ay) return dx < 0 ? 2 : 3;
  return dy < 0 ? 4 : 5;
}

int main() {
  ASSERT_EQ(classify(5, 3, 100), 1);          // small = TAP
  ASSERT_EQ(classify(-5, -3, 200), 1);        // small = TAP
  ASSERT_EQ(classify(-60, 10, 300), 2);       // SWIPE_LEFT
  ASSERT_EQ(classify(60, -5, 200), 3);        // SWIPE_RIGHT
  ASSERT_EQ(classify(5, -50, 250), 4);        // SWIPE_UP
  ASSERT_EQ(classify(3, 55, 300), 5);         // SWIPE_DOWN
  ASSERT_EQ(classify(-80, 0, 600), 0);        // too long = NONE
  ASSERT_EQ(classify(40, 41, 200), 5);        // dy>dx tied → vertical
  TEST_SUMMARY();
}
```

- [ ] **Step 2: Run test — verify it passes**

```bash
make test_input
```

Expected: `8/8 passed`

- [ ] **Step 3: Write src/core/Game.h**

```cpp
#pragma once
#include <cstdint>

struct InputEvent {
  enum Type : uint8_t {
    NONE, TAP, SWIPE_LEFT, SWIPE_RIGHT, SWIPE_UP, SWIPE_DOWN, DRAG
  };
  Type     type = NONE;
  uint16_t x = 0, y = 0;
  int16_t  dx = 0, dy = 0;
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>

class AssetManager;
class ScoreManager;

class Game {
public:
  virtual ~Game() {}
  virtual void        begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores) = 0;
  virtual void        update(const InputEvent& input) = 0;
  virtual void        draw() = 0;
  virtual void        end() = 0;
  virtual const char* name()      = 0;
  virtual uint32_t    highScore() = 0;
  virtual bool        isDone()    = 0;
};
#endif
```

- [ ] **Step 4: Write src/ui/Theme.h**

```cpp
#pragma once
#include <cstdint>

namespace Theme {
  // 24-bit RGB (use tft.color24to16() to convert)
  constexpr uint32_t BG        = 0x0d0221;
  constexpr uint32_t ACCENT    = 0xf72585;
  constexpr uint32_t SECONDARY = 0x7209b7;
  constexpr uint32_t TEXT      = 0xffffff;
  constexpr uint32_t DIM       = 0x4cc9f0;
  constexpr uint32_t DARK      = 0x2d0a4e;

  // Pre-computed RGB565
  constexpr uint16_t BG565        = 0x0801;
  constexpr uint16_t ACCENT565    = 0xF94B;
  constexpr uint16_t SECONDARY565 = 0x7016;
  constexpr uint16_t TEXT565      = 0xFFFF;
  constexpr uint16_t DIM565       = 0x4E5E;
  constexpr uint16_t DARK565      = 0x2849;
}
```

- [ ] **Step 5: Write src/core/InputManager.h**

```cpp
#pragma once
#include "Game.h"

#ifndef NATIVE_TEST
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

class InputManager {
public:
  InputManager(uint8_t csPin);
  void       begin();
  InputEvent read();

private:
  XPT2046_Touchscreen _touch;
  bool      _wasTouched = false;
  uint16_t  _startX = 0, _startY = 0;
  uint16_t  _lastX  = 0, _lastY  = 0;
  uint32_t  _touchStart = 0;

  uint16_t mapX(uint16_t raw);
  uint16_t mapY(uint16_t raw);
  static InputEvent::Type classify(int16_t dx, int16_t dy, uint32_t ms);
};
#endif
```

- [ ] **Step 6: Write src/core/InputManager.cpp**

```cpp
#include "InputManager.h"

#ifndef NATIVE_TEST

InputManager::InputManager(uint8_t csPin) : _touch(csPin) {}

void InputManager::begin() {
  _touch.begin();
  _touch.setRotation(1);
}

uint16_t InputManager::mapX(uint16_t raw) {
  if (raw < 200) raw = 200;
  if (raw > 3900) raw = 3900;
  return (uint32_t)(raw - 200) * 319 / 3700;
}

uint16_t InputManager::mapY(uint16_t raw) {
  if (raw < 300) raw = 300;
  if (raw > 3800) raw = 3800;
  return (uint32_t)(raw - 300) * 239 / 3500;
}

InputEvent::Type InputManager::classify(int16_t dx, int16_t dy, uint32_t ms) {
  const int SWIPE_THRESH = 30;
  const int MAX_SWIPE_MS = 500;
  if (ms > MAX_SWIPE_MS) return InputEvent::NONE;
  int ax = dx < 0 ? -dx : dx;
  int ay = dy < 0 ? -dy : dy;
  if (ax < SWIPE_THRESH && ay < SWIPE_THRESH) return InputEvent::TAP;
  if (ax > ay) return dx < 0 ? InputEvent::SWIPE_LEFT : InputEvent::SWIPE_RIGHT;
  return dy < 0 ? InputEvent::SWIPE_UP : InputEvent::SWIPE_DOWN;
}

InputEvent InputManager::read() {
  InputEvent evt;
  bool touched = _touch.touched();

  if (touched) {
    TS_Point p = _touch.getPoint();
    uint16_t sx = mapX(p.x);
    uint16_t sy = mapY(p.y);

    if (!_wasTouched) {
      _startX = sx; _startY = sy;
      _lastX  = sx; _lastY  = sy;
      _touchStart = millis();
      _wasTouched = true;
    } else {
      int16_t mdx = (int16_t)sx - (int16_t)_startX;
      int16_t mdy = (int16_t)sy - (int16_t)_startY;
      int amx = mdx < 0 ? -mdx : mdx;
      int amy = mdy < 0 ? -mdy : mdy;
      if (amx > 5 || amy > 5) {
        evt.type = InputEvent::DRAG;
        evt.x = sx; evt.y = sy;
        evt.dx = mdx; evt.dy = mdy;
      }
      _lastX = sx; _lastY = sy;
    }
  } else if (_wasTouched) {
    uint32_t held = millis() - _touchStart;
    int16_t dx = (int16_t)_lastX - (int16_t)_startX;
    int16_t dy = (int16_t)_lastY - (int16_t)_startY;
    evt.type = classify(dx, dy, held);
    evt.x  = _lastX;  evt.y  = _lastY;
    evt.dx = dx;      evt.dy = dy;
    _wasTouched = false;
  }

  return evt;
}

#endif
```

- [ ] **Step 7: Compile and run tests**

```bash
make test_input
arduino-cli compile --fqbn esp32:esp32:esp32 .
```

Expected: native tests pass, Arduino compile succeeds.

- [ ] **Step 8: Commit**

```bash
git add src/core/Game.h src/ui/Theme.h src/core/InputManager.h src/core/InputManager.cpp tests/test_input.cpp
git commit -m "feat: Game interface, Theme palette, InputManager with gesture classification"
```

---

### Task 3: AssetManager

**Files:**
- Create: `src/core/AssetManager.h`
- Create: `src/core/AssetManager.cpp`

- [ ] **Step 1: Write src/core/AssetManager.h**

```cpp
#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include <SD.h>
#include <cstring>

// LRU cache of 4 RGB565 BMP sprites loaded from SD.
// Expected BMP: 16-bit (RGB565), uncompressed.
class AssetManager {
public:
  static const int CACHE_SIZE = 4;

  void begin(uint8_t sdCsPin);

  // "snake/apple" → /sprites/snake/apple.bmp
  // Returns nullptr if file missing or invalid.
  const uint16_t* loadBitmap(const char* key, uint16_t& w, uint16_t& h);

  void freeAll();

private:
  struct Slot {
    char     key[32] = {0};
    uint16_t* pixels = nullptr;
    uint16_t  w = 0, h = 0;
    uint32_t  lastUsed = 0;
  };

  Slot     _slots[CACHE_SIZE];
  uint32_t _tick = 0;

  int  findSlot(const char* key);
  int  evictLRU();
  bool loadBmpFromSD(const char* path, Slot& slot);
};
#endif
```

- [ ] **Step 2: Write src/core/AssetManager.cpp**

```cpp
#include "AssetManager.h"
#ifndef NATIVE_TEST

void AssetManager::begin(uint8_t sdCsPin) {
  SD.begin(sdCsPin);
}

int AssetManager::findSlot(const char* key) {
  for (int i = 0; i < CACHE_SIZE; i++)
    if (_slots[i].pixels && strncmp(_slots[i].key, key, 31) == 0) return i;
  return -1;
}

int AssetManager::evictLRU() {
  for (int i = 0; i < CACHE_SIZE; i++)
    if (!_slots[i].pixels) return i;
  int lru = 0;
  for (int i = 1; i < CACHE_SIZE; i++)
    if (_slots[i].lastUsed < _slots[lru].lastUsed) lru = i;
  delete[] _slots[lru].pixels;
  _slots[lru].pixels = nullptr;
  _slots[lru].key[0] = 0;
  return lru;
}

bool AssetManager::loadBmpFromSD(const char* path, Slot& slot) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;

  uint8_t header[54];
  if (f.read(header, 54) != 54) { f.close(); return false; }
  if (header[0] != 'B' || header[1] != 'M') { f.close(); return false; }

  uint32_t dataOffset = *(uint32_t*)(header + 10);
  uint32_t width      = *(uint32_t*)(header + 18);
  uint32_t height     = *(uint32_t*)(header + 22);
  uint16_t bpp        = *(uint16_t*)(header + 28);

  if (bpp != 16 || width > 320 || height > 240) { f.close(); return false; }

  uint32_t pixelCount = width * height;
  slot.pixels = new (std::nothrow) uint16_t[pixelCount];
  if (!slot.pixels) { f.close(); return false; }

  slot.w = (uint16_t)width;
  slot.h = (uint16_t)height;

  f.seek(dataOffset);
  // BMP rows are bottom-up
  for (int row = (int)height - 1; row >= 0; row--) {
    f.read((uint8_t*)&slot.pixels[row * width], width * 2);
  }
  f.close();
  return true;
}

const uint16_t* AssetManager::loadBitmap(const char* key, uint16_t& w, uint16_t& h) {
  int idx = findSlot(key);
  if (idx >= 0) {
    _slots[idx].lastUsed = ++_tick;
    w = _slots[idx].w; h = _slots[idx].h;
    return _slots[idx].pixels;
  }

  idx = evictLRU();
  char path[64];
  snprintf(path, sizeof(path), "/sprites/%s.bmp", key);
  if (!loadBmpFromSD(path, _slots[idx])) {
    w = 0; h = 0;
    return nullptr;
  }
  strncpy(_slots[idx].key, key, 31);
  _slots[idx].lastUsed = ++_tick;
  w = _slots[idx].w; h = _slots[idx].h;
  return _slots[idx].pixels;
}

void AssetManager::freeAll() {
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (_slots[i].pixels) {
      delete[] _slots[i].pixels;
      _slots[i].pixels = nullptr;
      _slots[i].key[0] = 0;
      _slots[i].w = _slots[i].h = 0;
    }
  }
}

#endif
```

- [ ] **Step 3: Compile**

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
```

Expected: clean compile.

- [ ] **Step 4: Commit**

```bash
git add src/core/AssetManager.h src/core/AssetManager.cpp
git commit -m "feat: AssetManager — SD BMP loader with LRU cache"
```

---

### Task 4: ScoreManager

**Files:**
- Create: `src/core/ScoreManager.h`
- Create: `src/core/ScoreManager.cpp`

- [ ] **Step 1: Write src/core/ScoreManager.h**

```cpp
#pragma once
#ifndef NATIVE_TEST
#include <Preferences.h>
#include <cstdint>

class ScoreManager {
public:
  void     begin();
  uint32_t getHighScore(const char* gameName);
  void     setHighScore(const char* gameName, uint32_t score);
  void     end();

private:
  Preferences _prefs;
};
#endif
```

- [ ] **Step 2: Write src/core/ScoreManager.cpp**

```cpp
#include "ScoreManager.h"
#ifndef NATIVE_TEST

void ScoreManager::begin() {
  _prefs.begin("arcade", false);
}

uint32_t ScoreManager::getHighScore(const char* gameName) {
  return _prefs.getUInt(gameName, 0);
}

void ScoreManager::setHighScore(const char* gameName, uint32_t score) {
  if (score > getHighScore(gameName))
    _prefs.putUInt(gameName, score);
}

void ScoreManager::end() {
  _prefs.end();
}

#endif
```

- [ ] **Step 3: Compile**

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
```

Expected: clean compile.

- [ ] **Step 4: Commit**

```bash
git add src/core/ScoreManager.h src/core/ScoreManager.cpp
git commit -m "feat: ScoreManager — NVS high score persistence"
```

---

### Task 5: Launcher carousel

**Files:**
- Create: `src/core/Launcher.h`
- Create: `src/core/Launcher.cpp`

- [ ] **Step 1: Write src/core/Launcher.h**

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
  void  addGame(Game* game);
  Game* update(const InputEvent& evt);  // returns active game (nullptr = still in menu)
  void  draw();
  void  returnToMenu();

  bool  inGame() const { return _inGame; }

private:
  TFT_eSPI*     _tft     = nullptr;
  AssetManager* _assets  = nullptr;
  ScoreManager* _scores  = nullptr;
  Game*         _games[MAX_GAMES] = {};
  int           _count   = 0;
  int           _current = 0;
  bool          _inGame  = false;
  Game*         _active  = nullptr;
  bool          _needsRedraw = true;

  void drawCard();
  void drawDots();
  void drawArrows();
};
#endif
```

- [ ] **Step 2: Write src/core/Launcher.cpp**

```cpp
#include "Launcher.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"

void Launcher::begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores) {
  _tft = &tft; _assets = &assets; _scores = &scores;
  _needsRedraw = true;
}

void Launcher::addGame(Game* g) {
  if (_count < MAX_GAMES) _games[_count++] = g;
}

Game* Launcher::update(const InputEvent& evt) {
  if (_inGame) return _active;
  if (_count == 0) return nullptr;

  if (evt.type == InputEvent::SWIPE_LEFT && _current < _count - 1) {
    _current++;
    _needsRedraw = true;
  } else if (evt.type == InputEvent::SWIPE_RIGHT && _current > 0) {
    _current--;
    _needsRedraw = true;
  } else if (evt.type == InputEvent::TAP) {
    if (evt.x > 50 && evt.x < 270 && evt.y > 35 && evt.y < 205) {
      _active = _games[_current];
      _inGame = true;
      _active->begin(*_tft, *_assets, *_scores);
      return _active;
    }
  }
  return nullptr;
}

void Launcher::draw() {
  if (_inGame) { _active->draw(); return; }
  if (!_needsRedraw) return;

  TFT_eSPI& t = *_tft;
  t.fillScreen(t.color24to16(Theme::BG));
  t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::BG));
  t.drawString("PAPER ARCADE", 60, 8, 4);

  if (_count > 0) drawCard();
  drawArrows();
  drawDots();

  _needsRedraw = false;
}

void Launcher::drawCard() {
  TFT_eSPI& t = *_tft;
  Game* g = _games[_current];

  int x = 60, y = 38;
  t.drawRoundRect(x, y, 200, 165, 8, t.color24to16(Theme::ACCENT));
  t.fillRoundRect(x + 1, y + 1, 198, 163, 7, t.color24to16(Theme::DARK));

  // Game icon placeholder: large initial letter in pink
  char initial[2] = { g->name()[0], 0 };
  t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::DARK));
  t.drawString(initial, 145, 60, 7);

  t.setTextColor(t.color24to16(Theme::TEXT), t.color24to16(Theme::DARK));
  int nameW = t.textWidth(g->name(), 4);
  t.drawString(g->name(), 160 - nameW / 2, 135, 4);

  char buf[32];
  snprintf(buf, sizeof(buf), "HI: %lu", (unsigned long)g->highScore());
  t.setTextColor(t.color24to16(Theme::DIM), t.color24to16(Theme::DARK));
  int hiW = t.textWidth(buf, 2);
  t.drawString(buf, 160 - hiW / 2, 175, 2);
}

void Launcher::drawArrows() {
  TFT_eSPI& t = *_tft;
  if (_current > 0) {
    t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::BG));
    t.drawString("<", 20, 110, 6);
  }
  if (_current < _count - 1) {
    t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::BG));
    t.drawString(">", 290, 110, 6);
  }
}

void Launcher::drawDots() {
  TFT_eSPI& t = *_tft;
  if (_count == 0) return;
  int totalW = _count * 12;
  int startX = (320 - totalW) / 2 + 4;
  for (int i = 0; i < _count; i++) {
    uint16_t col = (i == _current) ? t.color24to16(Theme::ACCENT) : t.color24to16(Theme::DARK);
    int r = (i == _current) ? 4 : 3;
    t.fillCircle(startX + i * 12, 220, r, col);
  }
}

void Launcher::returnToMenu() {
  if (_active) { _active->end(); _active = nullptr; }
  _inGame = false;
  _needsRedraw = true;
}

#endif
```

- [ ] **Step 3: Compile**

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
```

Expected: clean compile.

- [ ] **Step 4: Commit**

```bash
git add src/core/Launcher.h src/core/Launcher.cpp
git commit -m "feat: Launcher — synthwave swipe carousel with cards and dots"
```

---

### Task 6: Main loop + OTA boot mode

**Files:**
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Replace paper-arcade.ino**

```cpp
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
// #include "src/games/Pong.h"
// #include "src/games/SimonSays.h"
// #include "src/games/Minesweeper.h"
// #include "src/games/Game2048.h"
// #include "src/games/Breakout.h"
// #include "src/games/FlappyBird.h"
// #include "src/games/Tetris.h"
// #include "src/games/SpaceInvaders.h"
// #include "src/games/PacMan.h"

#define TOUCH_CS_PIN  33
#define SD_CS_PIN      5
#define TFT_BL_PIN    21

TFT_eSPI      tft;
InputManager  input(TOUCH_CS_PIN);
AssetManager  assets;
ScoreManager  scores;
Launcher      launcher;
Game*         activeGame = nullptr;

// OTA mode triggered by holding screen during boot for >=3s
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
  WiFi.begin();  // uses last stored credentials
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
  // launcher.addGame(new Pong());
  // launcher.addGame(new SimonSays());
  // launcher.addGame(new Minesweeper());
  // launcher.addGame(new Game2048());
  // launcher.addGame(new Breakout());
  // launcher.addGame(new FlappyBird());
  // launcher.addGame(new Tetris());
  // launcher.addGame(new SpaceInvaders());
  // launcher.addGame(new PacMan());

  launcher.begin(tft, assets, scores);
}

void loop() {
  ArduinoOTA.handle();

  InputEvent evt = input.read();
  activeGame = launcher.update(evt);

  if (activeGame) {
    activeGame->update(evt);
    if (activeGame->isDone() && evt.type == InputEvent::TAP) {
      launcher.returnToMenu();
      activeGame = nullptr;
    }
  }
  launcher.draw();
}
```

- [ ] **Step 2: Compile and flash**

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

Expected: device shows splash "PAPER ARCADE", then launcher header. Empty carousel area (no game cards yet).

- [ ] **Step 3: Commit**

```bash
git add paper-arcade.ino
git commit -m "feat: main loop integration, OTA boot-hold mode"
```

---

### Task 7: Snake

**Files:**
- Create: `src/games/Snake.h`
- Create: `src/games/Snake.cpp`
- Create: `tests/test_snake.cpp`
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Write tests/test_snake.cpp**

```cpp
#include "test_runner.h"
#include "games/Snake.h"

int main() {
  SnakeLogic s;
  s.init(20, 15);

  ASSERT_EQ(s.length(), 3);
  ASSERT_EQ(s.headX(), 7);
  ASSERT_EQ(s.headY(), 7);

  s.tick();
  ASSERT_EQ(s.headX(), 8);
  ASSERT_EQ(s.headY(), 7);

  s.setDirection(SnakeLogic::UP);
  s.tick();
  ASSERT_EQ(s.headY(), 6);

  // Cannot reverse: pressing DOWN while moving UP keeps moving UP
  s.setDirection(SnakeLogic::DOWN);
  s.tick();
  ASSERT_EQ(s.headY(), 5);

  // Wall collision
  SnakeLogic s2;
  s2.init(20, 15);
  s2.setDirection(SnakeLogic::LEFT);
  for (int i = 0; i < 7; i++) s2.tick();   // moves to x=0
  ASSERT_FALSE(s2.tick());                  // next step hits wall

  TEST_SUMMARY();
}
```

- [ ] **Step 2: Run test — expect compile fail**

```bash
make test_snake
```

Expected: error — Snake.h doesn't exist.

- [ ] **Step 3: Write src/games/Snake.h**

```cpp
#pragma once
#include <cstdint>
#include <deque>

struct Point { int16_t x, y; };

class SnakeLogic {
public:
  enum Dir : uint8_t { RIGHT, LEFT, UP, DOWN };

  void     init(uint8_t cols, uint8_t rows);
  void     setDirection(Dir d);
  bool     tick();                                      // false = died
  int      length() const  { return (int)_body.size(); }
  int16_t  headX()  const  { return _body.front().x; }
  int16_t  headY()  const  { return _body.front().y; }
  Point    foodPos() const { return _food; }
  uint32_t score()   const { return _score; }
  const std::deque<Point>& body() const { return _body; }

private:
  std::deque<Point> _body;
  Point    _food = {0,0};
  Dir      _dir = RIGHT, _next = RIGHT;
  uint8_t  _cols = 0, _rows = 0;
  uint32_t _score = 0;

  void placeFood();
  bool onBody(int16_t x, int16_t y) const;
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class Snake : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "SNAKE"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  static const uint8_t COLS = 20, ROWS = 15, CELL = 16;
  SnakeLogic    _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  TFT_eSprite   _sprite{nullptr};
  uint32_t      _lastTick = 0, _tickMs = 150;
  uint32_t      _hiScore = 0;
  bool          _done = false;
};
#endif
```

- [ ] **Step 4: Write src/games/Snake.cpp**

```cpp
#include "Snake.h"
#include <cstdlib>

#ifdef NATIVE_TEST
static uint32_t native_seed = 42;
static uint32_t now_ms()    { return native_seed; }
#else
static uint32_t now_ms()    { return millis(); }
#endif

void SnakeLogic::init(uint8_t cols, uint8_t rows) {
  _cols = cols; _rows = rows;
  _body.clear();
  int16_t cy = rows / 2;
  _body.push_front({5, cy});
  _body.push_front({6, cy});
  _body.push_front({7, cy});
  _dir = _next = RIGHT;
  _score = 0;
  srand(now_ms());
  placeFood();
}

void SnakeLogic::setDirection(Dir d) {
  if ((_dir == RIGHT && d == LEFT) || (_dir == LEFT && d == RIGHT)) return;
  if ((_dir == UP    && d == DOWN) || (_dir == DOWN && d == UP))    return;
  _next = d;
}

bool SnakeLogic::tick() {
  _dir = _next;
  Point head = _body.front();
  switch (_dir) {
    case RIGHT: head.x++; break;
    case LEFT:  head.x--; break;
    case UP:    head.y--; break;
    case DOWN:  head.y++; break;
  }
  if (head.x < 0 || head.x >= _cols || head.y < 0 || head.y >= _rows) return false;

  bool ate = (head.x == _food.x && head.y == _food.y);
  if (!ate) _body.pop_back();
  if (onBody(head.x, head.y)) return false;
  _body.push_front(head);
  if (ate) { _score += 10; placeFood(); }
  return true;
}

void SnakeLogic::placeFood() {
  do {
    _food.x = rand() % _cols;
    _food.y = rand() % _rows;
  } while (onBody(_food.x, _food.y));
}

bool SnakeLogic::onBody(int16_t x, int16_t y) const {
  for (auto& p : _body)
    if (p.x == x && p.y == y) return true;
  return false;
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"

void Snake::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft    = &tft;
  _scores = &scores;
  _hiScore = scores.getHighScore("snake");
  _done    = false;
  _tickMs  = 150;
  _sprite.setColorDepth(16);
  _sprite.createSprite(320, 240);
  _logic.init(COLS, ROWS);
  _lastTick = millis();
}

void Snake::update(const InputEvent& input) {
  if (_done) return;
  if (input.type == InputEvent::SWIPE_LEFT)  _logic.setDirection(SnakeLogic::LEFT);
  if (input.type == InputEvent::SWIPE_RIGHT) _logic.setDirection(SnakeLogic::RIGHT);
  if (input.type == InputEvent::SWIPE_UP)    _logic.setDirection(SnakeLogic::UP);
  if (input.type == InputEvent::SWIPE_DOWN)  _logic.setDirection(SnakeLogic::DOWN);

  uint32_t step = 150 - (_logic.score() / 50) * 10;
  if (step < 60) step = 60;
  _tickMs = step;

  if (millis() - _lastTick >= _tickMs) {
    _lastTick = millis();
    if (!_logic.tick()) {
      _scores->setHighScore("snake", _logic.score());
      _hiScore = _scores->getHighScore("snake");
      _done = true;
    }
  }
}

void Snake::draw() {
  TFT_eSprite& s = _sprite;
  s.fillSprite(s.color24to16(Theme::BG));

  Point f = _logic.foodPos();
  s.fillRect(f.x * CELL + 2, f.y * CELL + 2, CELL - 4, CELL - 4,
             s.color24to16(Theme::ACCENT));

  const auto& body = _logic.body();
  for (size_t i = 0; i < body.size(); i++) {
    uint16_t c = (i == 0) ? s.color24to16(Theme::DIM) : s.color24to16(Theme::SECONDARY);
    s.fillRect(body[i].x * CELL + 1, body[i].y * CELL + 1, CELL - 2, CELL - 2, c);
  }

  char buf[32];
  snprintf(buf, sizeof(buf), "SCORE %lu", (unsigned long)_logic.score());
  s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
  s.drawString(buf, 4, 224, 2);
  snprintf(buf, sizeof(buf), "HI %lu", (unsigned long)_hiScore);
  s.drawString(buf, 240, 224, 2);

  if (_done) {
    s.setTextColor(s.color24to16(Theme::ACCENT), s.color24to16(Theme::BG));
    s.drawString("GAME OVER", 80, 90, 4);
    s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
    s.drawString("TAP TO EXIT", 90, 130, 2);
  }

  s.pushSprite(0, 0);
}

void Snake::end() {
  _sprite.deleteSprite();
}
#endif
```

- [ ] **Step 5: Run test**

```bash
make test_snake
```

Expected: `5/5 passed`.

- [ ] **Step 6: Register Snake in paper-arcade.ino**

In `paper-arcade.ino`, uncomment:
```cpp
#include "src/games/Snake.h"
```
And in `setup()`:
```cpp
launcher.addGame(new Snake());
```

- [ ] **Step 7: Compile, flash, manually verify**

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

Verify on device: SNAKE card appears. Tap it → snake moves right. Swipe to steer. Hit a wall → "GAME OVER". Tap → return to launcher. Power-cycle → high score persists in carousel.

- [ ] **Step 8: Commit**

```bash
git add src/games/Snake.h src/games/Snake.cpp tests/test_snake.cpp paper-arcade.ino
git commit -m "feat: Snake game with swipe controls and persistent high score"
```

---

### Task 8: Pong

**Files:**
- Create: `src/games/Pong.h`
- Create: `src/games/Pong.cpp`
- Create: `tests/test_pong.cpp`
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Write tests/test_pong.cpp**

```cpp
#include "test_runner.h"
#include "games/Pong.h"

int main() {
  PongLogic p;
  p.init(320, 240);

  // Initial state: paddles centered, scores 0
  ASSERT_EQ(p.playerScore(), 0);
  ASSERT_EQ(p.aiScore(), 0);
  ASSERT_TRUE(p.rightPaddleY() >= 90 && p.rightPaddleY() <= 110);

  // Move player paddle to top
  p.setPlayerPaddleY(20);
  ASSERT_TRUE(p.rightPaddleY() < 30);

  // Clamp at top
  p.setPlayerPaddleY(-50);
  ASSERT_EQ(p.rightPaddleY(), 0);

  // Clamp at bottom
  p.setPlayerPaddleY(1000);
  ASSERT_EQ(p.rightPaddleY(), 240 - 40);

  TEST_SUMMARY();
}
```

- [ ] **Step 2: Write src/games/Pong.h**

```cpp
#pragma once
#include <cstdint>

class PongLogic {
public:
  static const uint8_t WIN_SCORE = 5;

  void     init(uint16_t w, uint16_t h);
  void     setPlayerPaddleY(int16_t centerY);
  bool     tick(uint32_t deltaMs);     // false = match over
  int16_t  ballX() const         { return _bx; }
  int16_t  ballY() const         { return _by; }
  int16_t  rightPaddleY() const  { return _ry; }
  int16_t  leftPaddleY()  const  { return _ly; }
  uint8_t  playerScore() const   { return _ps; }
  uint8_t  aiScore()     const   { return _as; }

  static const int16_t PAD_H = 40, PAD_W = 6, BALL_R = 4;

private:
  uint16_t _w = 0, _h = 0;
  int16_t  _bx = 0, _by = 0;
  float    _vx = 0.0f, _vy = 0.0f;
  int16_t  _ry = 0, _ly = 0;
  uint8_t  _ps = 0, _as = 0;
  void resetBall();
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class Pong : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "PONG"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  PongLogic     _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  TFT_eSprite   _sprite{nullptr};
  uint32_t      _lastTick = 0;
  uint32_t      _hiScore = 0;
  bool          _done = false;
  int16_t       _paddleTarget = 120;
};
#endif
```

- [ ] **Step 3: Write src/games/Pong.cpp**

```cpp
#include "Pong.h"
#include <cstdlib>
#include <cmath>

void PongLogic::init(uint16_t w, uint16_t h) {
  _w = w; _h = h;
  _ry = _ly = (h - PAD_H) / 2;
  _ps = _as = 0;
  resetBall();
}

void PongLogic::resetBall() {
  _bx = _w / 2; _by = _h / 2;
  _vx = ((rand() % 2) ? 1 : -1) * 0.18f;
  _vy = ((rand() % 100) - 50) * 0.003f;
}

void PongLogic::setPlayerPaddleY(int16_t cy) {
  int16_t target = cy - PAD_H / 2;
  if (target < 0) target = 0;
  if (target + PAD_H > (int16_t)_h) target = _h - PAD_H;
  _ry = target;
}

bool PongLogic::tick(uint32_t dt) {
  if (dt > 50) dt = 50;
  // AI follows ball
  int16_t mid = _ly + PAD_H / 2;
  int16_t aiSpeed = 2;
  if (mid + 2 < _by) _ly += aiSpeed;
  else if (mid - 2 > _by) _ly -= aiSpeed;
  if (_ly < 0) _ly = 0;
  if (_ly + PAD_H > (int16_t)_h) _ly = _h - PAD_H;

  _bx += (int16_t)(_vx * dt);
  _by += (int16_t)(_vy * dt);

  if (_by - BALL_R < 0)            { _by = BALL_R;        _vy = -_vy; }
  if (_by + BALL_R > (int16_t)_h)  { _by = _h - BALL_R;   _vy = -_vy; }

  // Right paddle (player) at x = _w - 10 - PAD_W
  if (_bx + BALL_R >= _w - 10 - PAD_W && _bx + BALL_R <= _w - 10 &&
      _by >= _ry && _by <= _ry + PAD_H) {
    _vx = -fabsf(_vx) * 1.05f;
    _vy += ((float)(_by - (_ry + PAD_H / 2))) * 0.004f;
    _bx = _w - 10 - PAD_W - BALL_R;
  }
  // Left paddle (AI) at x = 10
  if (_bx - BALL_R <= 10 + PAD_W && _bx - BALL_R >= 10 &&
      _by >= _ly && _by <= _ly + PAD_H) {
    _vx = fabsf(_vx) * 1.05f;
    _bx = 10 + PAD_W + BALL_R;
  }

  if (_bx < 0)              { _ps++; resetBall(); }
  if (_bx > (int16_t)_w)    { _as++; resetBall(); }

  return _ps < WIN_SCORE && _as < WIN_SCORE;
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"

void Pong::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("pong");
  _done = false;
  _sprite.setColorDepth(16);
  _sprite.createSprite(320, 240);
  srand(millis());
  _logic.init(320, 240);
  _paddleTarget = 120;
  _lastTick = millis();
}

void Pong::update(const InputEvent& input) {
  if (_done) return;
  if (input.type == InputEvent::TAP) {
    if (input.x < 160) _paddleTarget -= 30;
    else               _paddleTarget += 30;
  } else if (input.type == InputEvent::DRAG) {
    _paddleTarget = input.y;
  }
  _logic.setPlayerPaddleY(_paddleTarget);

  uint32_t now = millis();
  if (!_logic.tick(now - _lastTick)) {
    uint32_t s = (uint32_t)_logic.playerScore() * 100;
    _scores->setHighScore("pong", s);
    _hiScore = _scores->getHighScore("pong");
    _done = true;
  }
  _lastTick = now;
}

void Pong::draw() {
  TFT_eSprite& s = _sprite;
  s.fillSprite(s.color24to16(Theme::BG));
  for (int y = 0; y < 240; y += 14)
    s.drawFastVLine(160, y, 8, s.color24to16(Theme::DARK));

  s.fillRect(10, _logic.leftPaddleY(),  PongLogic::PAD_W, PongLogic::PAD_H,
             s.color24to16(Theme::DIM));
  s.fillRect(320 - 10 - PongLogic::PAD_W, _logic.rightPaddleY(),
             PongLogic::PAD_W, PongLogic::PAD_H, s.color24to16(Theme::ACCENT));
  s.fillCircle(_logic.ballX(), _logic.ballY(), PongLogic::BALL_R,
               s.color24to16(Theme::TEXT));

  char buf[8];
  snprintf(buf, sizeof(buf), "%d", _logic.aiScore());
  s.setTextColor(s.color24to16(Theme::DIM), s.color24to16(Theme::BG));
  s.drawString(buf, 110, 10, 6);
  snprintf(buf, sizeof(buf), "%d", _logic.playerScore());
  s.setTextColor(s.color24to16(Theme::ACCENT), s.color24to16(Theme::BG));
  s.drawString(buf, 200, 10, 6);

  if (_done) {
    bool won = _logic.playerScore() >= PongLogic::WIN_SCORE;
    s.setTextColor(s.color24to16(Theme::ACCENT), s.color24to16(Theme::BG));
    s.drawString(won ? "YOU WIN!" : "GAME OVER", 70, 100, 4);
    s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
    s.drawString("TAP TO EXIT", 90, 145, 2);
  }
  s.pushSprite(0, 0);
}

void Pong::end() { _sprite.deleteSprite(); }
#endif
```

- [ ] **Step 4: Run tests**

```bash
make test_pong
```

Expected: `4/4 passed`.

- [ ] **Step 5: Register Pong in paper-arcade.ino**

Uncomment `#include "src/games/Pong.h"` and `launcher.addGame(new Pong());`

- [ ] **Step 6: Compile, flash, verify**

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

Verify: PONG card. Tap → game starts. AI tracks ball. Tap left/right or drag controls right paddle. First to 5 wins.

- [ ] **Step 7: Commit**

```bash
git add src/games/Pong.h src/games/Pong.cpp tests/test_pong.cpp paper-arcade.ino
git commit -m "feat: Pong — single-player vs AI with tap/drag paddle control"
```

---

### Task 9: Simon Says

**Files:**
- Create: `src/games/SimonSays.h`
- Create: `src/games/SimonSays.cpp`
- Create: `tests/test_simon.cpp`
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Write tests/test_simon.cpp**

```cpp
#include "test_runner.h"
#include "games/SimonSays.h"

int main() {
  SimonLogic g;
  g.init(7);   // deterministic seed

  ASSERT_EQ(g.sequenceLength(), 1);

  // Pretend we played the right input
  uint8_t first = g.sequenceAt(0);
  ASSERT_TRUE(g.checkInput(first));
  ASSERT_TRUE(g.advanceIfRoundComplete());
  ASSERT_EQ(g.sequenceLength(), 2);

  // Wrong input ends game
  SimonLogic g2;
  g2.init(7);
  uint8_t wrong = (g2.sequenceAt(0) + 1) % 4;
  ASSERT_FALSE(g2.checkInput(wrong));
  ASSERT_TRUE(g2.gameOver());

  TEST_SUMMARY();
}
```

- [ ] **Step 2: Write src/games/SimonSays.h**

```cpp
#pragma once
#include <cstdint>

class SimonLogic {
public:
  static const int MAX_LEN = 64;

  void    init(uint32_t seed);
  uint8_t sequenceAt(int i) const { return _seq[i]; }
  int     sequenceLength()  const { return _len; }
  bool    checkInput(uint8_t color);   // false = wrong = game over
  bool    advanceIfRoundComplete();    // true = new color added
  bool    gameOver()        const { return _over; }
  int     score()           const { return _len - 1; }
  int     userIndex()       const { return _userIdx; }

private:
  uint8_t _seq[MAX_LEN];
  int     _len = 0;
  int     _userIdx = 0;
  bool    _over = false;
  uint32_t _rngState = 0;

  uint8_t nextRand();
  void    appendNext();
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class SimonSays : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "SIMON"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  enum Phase { SHOWING, WAITING, FAIL };
  SimonLogic    _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  TFT_eSprite   _sprite{nullptr};
  Phase         _phase = SHOWING;
  int           _showIdx = 0;
  uint32_t      _phaseStart = 0;
  int8_t        _flashColor = -1;
  uint32_t      _hiScore = 0;
  bool          _done = false;

  int8_t hitTest(uint16_t x, uint16_t y);
  void drawQuadrant(int8_t color, bool lit);
};
#endif
```

- [ ] **Step 3: Write src/games/SimonSays.cpp**

```cpp
#include "SimonSays.h"

void SimonLogic::init(uint32_t seed) {
  _len = 0;
  _userIdx = 0;
  _over = false;
  _rngState = seed ? seed : 1;
  appendNext();
}

uint8_t SimonLogic::nextRand() {
  // xorshift32
  _rngState ^= _rngState << 13;
  _rngState ^= _rngState >> 17;
  _rngState ^= _rngState << 5;
  return _rngState & 3;
}

void SimonLogic::appendNext() {
  if (_len < MAX_LEN) _seq[_len++] = nextRand();
}

bool SimonLogic::checkInput(uint8_t color) {
  if (_over) return false;
  if (_seq[_userIdx] != color) {
    _over = true;
    return false;
  }
  _userIdx++;
  return true;
}

bool SimonLogic::advanceIfRoundComplete() {
  if (_userIdx == _len) {
    _userIdx = 0;
    appendNext();
    return true;
  }
  return false;
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"

// Quadrants:
//   0 = top-left  (red)        x: 0..159, y: 0..119
//   1 = top-right (green)      x: 160..319, y: 0..119
//   2 = bottom-left (blue)     x: 0..159, y: 120..239
//   3 = bottom-right (yellow)  x: 160..319, y: 120..239

static const uint16_t QUAD_COLORS[4]    = { 0xF800, 0x07E0, 0x001F, 0xFFE0 };
static const uint16_t QUAD_DIM[4]       = { 0x6000, 0x0300, 0x000C, 0x6320 };

void SimonSays::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("simon");
  _done = false;
  _sprite.setColorDepth(16);
  _sprite.createSprite(320, 240);
  _logic.init(millis());
  _phase = SHOWING;
  _showIdx = 0;
  _flashColor = -1;
  _phaseStart = millis();
}

int8_t SimonSays::hitTest(uint16_t x, uint16_t y) {
  int col = (x < 160) ? 0 : 1;
  int row = (y < 120) ? 0 : 1;
  return row * 2 + col;
}

void SimonSays::update(const InputEvent& input) {
  if (_done) return;
  uint32_t now = millis();

  if (_phase == SHOWING) {
    uint32_t dt = now - _phaseStart;
    int interval = 500;
    int phase = dt / interval;
    if (phase < _logic.sequenceLength() * 2) {
      // alternate flash on/off
      if (phase % 2 == 0) _flashColor = _logic.sequenceAt(phase / 2);
      else                _flashColor = -1;
      _showIdx = phase / 2;
    } else {
      _phase = WAITING;
      _flashColor = -1;
      _phaseStart = now;
    }
    return;
  }

  if (_phase == WAITING && input.type == InputEvent::TAP) {
    int8_t color = hitTest(input.x, input.y);
    _flashColor = color;
    _phaseStart = now;
    if (!_logic.checkInput(color)) {
      _phase = FAIL;
      _scores->setHighScore("simon", _logic.score());
      _hiScore = _scores->getHighScore("simon");
    } else if (_logic.advanceIfRoundComplete()) {
      _phase = SHOWING;
      _showIdx = 0;
    }
  } else if (_phase == WAITING) {
    // clear flash 200ms after tap
    if (_flashColor >= 0 && now - _phaseStart > 200) _flashColor = -1;
  }

  if (_phase == FAIL && input.type == InputEvent::TAP && now - _phaseStart > 500) {
    _done = true;
  }
}

void SimonSays::drawQuadrant(int8_t color, bool lit) {
  int x = (color % 2) * 160;
  int y = (color / 2) * 120;
  uint16_t c = lit ? QUAD_COLORS[color] : QUAD_DIM[color];
  _sprite.fillRect(x + 4, y + 4, 152, 112, c);
}

void SimonSays::draw() {
  TFT_eSprite& s = _sprite;
  s.fillSprite(s.color24to16(Theme::BG));

  for (int8_t i = 0; i < 4; i++)
    drawQuadrant(i, _flashColor == i);

  char buf[32];
  snprintf(buf, sizeof(buf), "%d", _logic.score());
  s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
  s.fillCircle(160, 120, 26, s.color24to16(Theme::BG));
  int w = s.textWidth(buf, 4);
  s.drawString(buf, 160 - w / 2, 108, 4);

  if (_phase == FAIL) {
    s.setTextColor(s.color24to16(Theme::ACCENT), s.color24to16(Theme::BG));
    s.fillRect(60, 90, 200, 60, s.color24to16(Theme::BG));
    s.drawString("WRONG!", 100, 100, 4);
    s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
    s.drawString("TAP TO EXIT", 90, 130, 2);
  }
  s.pushSprite(0, 0);
}

void SimonSays::end() { _sprite.deleteSprite(); }
#endif
```

- [ ] **Step 4: Run tests, register, compile, flash, verify**

```bash
make test_simon
```

Expected: `5/5 passed`.

In `paper-arcade.ino` uncomment Simon Says include + `addGame`. Then:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

Verify: SIMON card. Quadrants flash a sequence. Tap them in order. Wrong tap = "WRONG!" → tap to exit.

- [ ] **Step 5: Commit**

```bash
git add src/games/SimonSays.h src/games/SimonSays.cpp tests/test_simon.cpp paper-arcade.ino
git commit -m "feat: Simon Says — 4-quadrant memory game"
```

---

### Task 10: Minesweeper

**Files:**
- Create: `src/games/Minesweeper.h`
- Create: `src/games/Minesweeper.cpp`
- Create: `tests/test_minesweeper.cpp`
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Write tests/test_minesweeper.cpp**

```cpp
#include "test_runner.h"
#include "games/Minesweeper.h"

int main() {
  MinesLogic m;
  m.init(10, 8, 10, 99);   // deterministic seed

  // 10 mines placed
  int mines = 0;
  for (int y = 0; y < 8; y++)
    for (int x = 0; x < 10; x++)
      if (m.isMine(x, y)) mines++;
  ASSERT_EQ(mines, 10);

  // Reveal a non-mine cell — game still alive
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 10; x++) {
      if (!m.isMine(x, y)) {
        bool dead = !m.reveal(x, y);
        ASSERT_FALSE(dead);
        goto done_safe;
      }
    }
  }
  done_safe: ;

  // Reveal a mine — game over
  MinesLogic m2;
  m2.init(10, 8, 10, 99);
  for (int y = 0; y < 8; y++) {
    for (int x = 0; x < 10; x++) {
      if (m2.isMine(x, y)) {
        ASSERT_FALSE(m2.reveal(x, y));
        ASSERT_TRUE(m2.isExploded());
        goto done_mine;
      }
    }
  }
  done_mine: ;

  // Flag toggle
  MinesLogic m3;
  m3.init(10, 8, 10, 99);
  m3.toggleFlag(0, 0);
  ASSERT_TRUE(m3.isFlagged(0, 0));
  m3.toggleFlag(0, 0);
  ASSERT_FALSE(m3.isFlagged(0, 0));

  TEST_SUMMARY();
}
```

- [ ] **Step 2: Write src/games/Minesweeper.h**

```cpp
#pragma once
#include <cstdint>

class MinesLogic {
public:
  static const int MAX_W = 12, MAX_H = 10;

  void  init(uint8_t w, uint8_t h, uint8_t mines, uint32_t seed);
  bool  reveal(uint8_t x, uint8_t y);   // false = hit a mine
  void  toggleFlag(uint8_t x, uint8_t y);

  bool  isMine(uint8_t x, uint8_t y) const;
  bool  isRevealed(uint8_t x, uint8_t y) const;
  bool  isFlagged(uint8_t x, uint8_t y)  const;
  uint8_t neighborCount(uint8_t x, uint8_t y) const;
  bool  isExploded() const { return _exploded; }
  bool  isWon()      const;
  int   width()  const { return _w; }
  int   height() const { return _h; }
  int   revealedCount() const { return _revealedCount; }

private:
  uint8_t _w = 0, _h = 0, _mines = 0;
  bool    _mine[MAX_W * MAX_H]     = {};
  bool    _revealed[MAX_W * MAX_H] = {};
  bool    _flag[MAX_W * MAX_H]     = {};
  bool    _exploded = false;
  int     _revealedCount = 0;
  uint32_t _rng = 1;

  inline int idx(int x, int y) const { return y * _w + x; }
  uint32_t  nextRand();
  void      placeMines(uint8_t count);
  void      floodFill(int x, int y);
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class Minesweeper : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "MINES"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  static const int CELL = 24;
  static const int W = 10, H = 8;
  MinesLogic    _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  TFT_eSprite   _sprite{nullptr};
  uint32_t      _lastTapMs = 0;
  uint8_t       _lastTapX = 255, _lastTapY = 255;
  uint32_t      _hiScore = 0;
  bool          _done = false;
};
#endif
```

- [ ] **Step 3: Write src/games/Minesweeper.cpp**

```cpp
#include "Minesweeper.h"
#include <cstring>

uint32_t MinesLogic::nextRand() {
  _rng ^= _rng << 13;
  _rng ^= _rng >> 17;
  _rng ^= _rng << 5;
  return _rng;
}

void MinesLogic::init(uint8_t w, uint8_t h, uint8_t mines, uint32_t seed) {
  _w = w; _h = h; _mines = mines;
  _rng = seed ? seed : 1;
  _exploded = false;
  _revealedCount = 0;
  std::memset(_mine, 0, sizeof(_mine));
  std::memset(_revealed, 0, sizeof(_revealed));
  std::memset(_flag, 0, sizeof(_flag));
  placeMines(mines);
}

void MinesLogic::placeMines(uint8_t count) {
  uint8_t placed = 0;
  while (placed < count) {
    int x = nextRand() % _w;
    int y = nextRand() % _h;
    if (!_mine[idx(x, y)]) { _mine[idx(x, y)] = true; placed++; }
  }
}

bool MinesLogic::isMine(uint8_t x, uint8_t y) const {
  return _mine[idx(x, y)];
}

bool MinesLogic::isRevealed(uint8_t x, uint8_t y) const {
  return _revealed[idx(x, y)];
}

bool MinesLogic::isFlagged(uint8_t x, uint8_t y) const {
  return _flag[idx(x, y)];
}

uint8_t MinesLogic::neighborCount(uint8_t x, uint8_t y) const {
  uint8_t n = 0;
  for (int dy = -1; dy <= 1; dy++)
    for (int dx = -1; dx <= 1; dx++) {
      int nx = x + dx, ny = y + dy;
      if (nx < 0 || ny < 0 || nx >= _w || ny >= _h) continue;
      if (_mine[idx(nx, ny)]) n++;
    }
  return n;
}

void MinesLogic::floodFill(int x, int y) {
  if (x < 0 || y < 0 || x >= _w || y >= _h) return;
  if (_revealed[idx(x, y)] || _flag[idx(x, y)] || _mine[idx(x, y)]) return;
  _revealed[idx(x, y)] = true;
  _revealedCount++;
  if (neighborCount(x, y) == 0) {
    for (int dy = -1; dy <= 1; dy++)
      for (int dx = -1; dx <= 1; dx++)
        if (dx || dy) floodFill(x + dx, y + dy);
  }
}

bool MinesLogic::reveal(uint8_t x, uint8_t y) {
  if (_exploded) return false;
  if (_flag[idx(x, y)] || _revealed[idx(x, y)]) return true;
  if (_mine[idx(x, y)]) {
    _exploded = true;
    _revealed[idx(x, y)] = true;
    return false;
  }
  floodFill(x, y);
  return true;
}

void MinesLogic::toggleFlag(uint8_t x, uint8_t y) {
  if (_revealed[idx(x, y)]) return;
  _flag[idx(x, y)] = !_flag[idx(x, y)];
}

bool MinesLogic::isWon() const {
  return !_exploded && (_revealedCount == _w * _h - _mines);
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"

void Minesweeper::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("mines");
  _done = false;
  _sprite.setColorDepth(16);
  _sprite.createSprite(320, 240);
  _logic.init(W, H, 10, millis());
}

void Minesweeper::update(const InputEvent& input) {
  if (_done) return;
  if (_logic.isExploded() || _logic.isWon()) {
    if (input.type == InputEvent::TAP) {
      uint32_t s = _logic.isWon() ? 1000 + _logic.revealedCount() * 10 : _logic.revealedCount() * 10;
      _scores->setHighScore("mines", s);
      _hiScore = _scores->getHighScore("mines");
      _done = true;
    }
    return;
  }
  if (input.type != InputEvent::TAP) return;
  if (input.y >= H * CELL) return;
  uint8_t cx = input.x / CELL;
  uint8_t cy = input.y / CELL;
  if (cx >= W || cy >= H) return;

  uint32_t now = millis();
  bool doubleTap = (cx == _lastTapX && cy == _lastTapY && now - _lastTapMs < 400);
  if (doubleTap) {
    _logic.toggleFlag(cx, cy);
    _lastTapX = 255;
  } else {
    _lastTapMs = now;
    _lastTapX = cx; _lastTapY = cy;
    _logic.reveal(cx, cy);
  }
}

static const uint16_t NUM_COLORS[9] = {
  0x0000, 0x4E5E, 0x07E0, 0xF800, 0x780F, 0x7980, 0x4E5E, 0x0000, 0x7BEF
};

void Minesweeper::draw() {
  TFT_eSprite& s = _sprite;
  s.fillSprite(s.color24to16(Theme::BG));

  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      int px = x * CELL, py = y * CELL;
      if (_logic.isRevealed(x, y)) {
        s.fillRect(px + 1, py + 1, CELL - 2, CELL - 2, s.color24to16(Theme::DARK));
        if (_logic.isMine(x, y)) {
          s.fillCircle(px + CELL / 2, py + CELL / 2, 6, s.color24to16(Theme::ACCENT));
        } else {
          uint8_t n = _logic.neighborCount(x, y);
          if (n > 0) {
            char c[2] = { (char)('0' + n), 0 };
            s.setTextColor(NUM_COLORS[n], s.color24to16(Theme::DARK));
            s.drawString(c, px + 8, py + 4, 2);
          }
        }
      } else {
        s.fillRect(px + 1, py + 1, CELL - 2, CELL - 2, s.color24to16(Theme::SECONDARY));
        if (_logic.isFlagged(x, y)) {
          s.fillTriangle(px + 8, py + 6, px + 18, py + 11, px + 8, py + 16,
                         s.color24to16(Theme::ACCENT));
        }
      }
      s.drawRect(px, py, CELL, CELL, s.color24to16(Theme::DARK));
    }
  }

  // HUD bottom strip
  char buf[32];
  snprintf(buf, sizeof(buf), "REVEALED %d", _logic.revealedCount());
  s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
  s.drawString(buf, 4, 200, 2);
  snprintf(buf, sizeof(buf), "HI %lu", (unsigned long)_hiScore);
  s.drawString(buf, 4, 220, 2);
  s.setTextColor(s.color24to16(Theme::DIM), s.color24to16(Theme::BG));
  s.drawString("DOUBLE-TAP=FLAG", 160, 220, 2);

  if (_logic.isExploded()) {
    s.setTextColor(s.color24to16(Theme::ACCENT), s.color24to16(Theme::BG));
    s.fillRect(80, 100, 160, 40, s.color24to16(Theme::BG));
    s.drawString("BOOM!", 130, 100, 4);
    s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
    s.drawString("TAP TO EXIT", 100, 140, 2);
  } else if (_logic.isWon()) {
    s.setTextColor(s.color24to16(Theme::DIM), s.color24to16(Theme::BG));
    s.fillRect(80, 100, 160, 40, s.color24to16(Theme::BG));
    s.drawString("YOU WIN!", 110, 100, 4);
  }

  s.pushSprite(0, 0);
}

void Minesweeper::end() { _sprite.deleteSprite(); }
#endif
```

- [ ] **Step 4: Test, register, compile, flash, verify**

```bash
make test_minesweeper
```

Expected: tests pass.

In `paper-arcade.ino` uncomment Minesweeper include + `addGame`.

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

Verify: MINES card. Tap to reveal. Empty cells flood-fill. Numbers show neighbor counts. Double-tap = flag. Hit a mine → BOOM!

- [ ] **Step 5: Commit**

```bash
git add src/games/Minesweeper.h src/games/Minesweeper.cpp tests/test_minesweeper.cpp paper-arcade.ino
git commit -m "feat: Minesweeper — flood fill reveal, double-tap flag"
```

---

### Task 11: 2048

**Files:**
- Create: `src/games/Game2048.h`
- Create: `src/games/Game2048.cpp`
- Create: `tests/test_2048.cpp`
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Write tests/test_2048.cpp**

```cpp
#include "test_runner.h"
#include "games/Game2048.h"

int main() {
  Game2048Logic g;
  g.initWithSeed(42);

  // Manually set a known board to test merge logic
  g.setCell(0, 0, 2);
  g.setCell(1, 0, 2);
  g.setCell(2, 0, 0);
  g.setCell(3, 0, 0);

  // Slide left → [4,0,0,0]
  bool moved = g.slide(Game2048Logic::LEFT, /*spawn=*/false);
  ASSERT_TRUE(moved);
  ASSERT_EQ(g.cell(0, 0), 4);
  ASSERT_EQ(g.cell(1, 0), 0);
  ASSERT_EQ(g.score(), 4);

  // 2,2,2,2 left → 4,4,0,0
  Game2048Logic g2;
  g2.initEmpty();
  g2.setCell(0, 0, 2);
  g2.setCell(1, 0, 2);
  g2.setCell(2, 0, 2);
  g2.setCell(3, 0, 2);
  g2.slide(Game2048Logic::LEFT, false);
  ASSERT_EQ(g2.cell(0, 0), 4);
  ASSERT_EQ(g2.cell(1, 0), 4);
  ASSERT_EQ(g2.cell(2, 0), 0);

  // 4,4,2,2 right → 0,0,8,4
  Game2048Logic g3;
  g3.initEmpty();
  g3.setCell(0, 0, 4); g3.setCell(1, 0, 4);
  g3.setCell(2, 0, 2); g3.setCell(3, 0, 2);
  g3.slide(Game2048Logic::RIGHT, false);
  ASSERT_EQ(g3.cell(3, 0), 4);
  ASSERT_EQ(g3.cell(2, 0), 8);
  ASSERT_EQ(g3.cell(1, 0), 0);

  TEST_SUMMARY();
}
```

- [ ] **Step 2: Write src/games/Game2048.h**

```cpp
#pragma once
#include <cstdint>

class Game2048Logic {
public:
  enum Dir : uint8_t { LEFT, RIGHT, UP, DOWN };

  void     initWithSeed(uint32_t seed);
  void     initEmpty();
  bool     slide(Dir d, bool spawn = true);   // true = something moved
  bool     hasMoves() const;
  void     setCell(int x, int y, uint16_t v);
  uint16_t cell(int x, int y) const { return _grid[y * 4 + x]; }
  uint32_t score() const { return _score; }

private:
  uint16_t _grid[16] = {0};
  uint32_t _score = 0;
  uint32_t _rng = 1;

  uint32_t nextRand();
  void     spawnTile();
  bool     slideRow(uint16_t* row);    // collapse left, returns moved
  void     reverseRow(uint16_t* row);
  void     copyCol(int x, uint16_t* out);
  void     writeCol(int x, const uint16_t* in);
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class Game2048 : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "2048"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  Game2048Logic _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  TFT_eSprite   _sprite{nullptr};
  uint32_t      _hiScore = 0;
  bool          _done = false;
  bool          _gameOver = false;
};
#endif
```

- [ ] **Step 3: Write src/games/Game2048.cpp**

```cpp
#include "Game2048.h"
#include <cstring>

uint32_t Game2048Logic::nextRand() {
  _rng ^= _rng << 13;
  _rng ^= _rng >> 17;
  _rng ^= _rng << 5;
  return _rng;
}

void Game2048Logic::initEmpty() {
  std::memset(_grid, 0, sizeof(_grid));
  _score = 0;
  _rng = 1;
}

void Game2048Logic::initWithSeed(uint32_t seed) {
  initEmpty();
  _rng = seed ? seed : 1;
  spawnTile();
  spawnTile();
}

void Game2048Logic::setCell(int x, int y, uint16_t v) { _grid[y * 4 + x] = v; }

void Game2048Logic::spawnTile() {
  int empty[16], n = 0;
  for (int i = 0; i < 16; i++) if (_grid[i] == 0) empty[n++] = i;
  if (n == 0) return;
  int idx = empty[nextRand() % n];
  _grid[idx] = (nextRand() % 10 == 0) ? 4 : 2;
}

bool Game2048Logic::slideRow(uint16_t* row) {
  uint16_t out[4] = {0};
  int wi = 0;
  bool moved = false;

  // Collapse non-zero leftward, merging adjacent equals
  uint16_t pending = 0;
  for (int i = 0; i < 4; i++) {
    if (row[i] == 0) continue;
    if (pending == row[i]) {
      out[wi++] = pending * 2;
      _score += pending * 2;
      pending = 0;
    } else {
      if (pending) out[wi++] = pending;
      pending = row[i];
    }
  }
  if (pending) out[wi++] = pending;

  for (int i = 0; i < 4; i++) {
    if (out[i] != row[i]) moved = true;
    row[i] = out[i];
  }
  return moved;
}

void Game2048Logic::reverseRow(uint16_t* row) {
  uint16_t t;
  t = row[0]; row[0] = row[3]; row[3] = t;
  t = row[1]; row[1] = row[2]; row[2] = t;
}

void Game2048Logic::copyCol(int x, uint16_t* out) {
  for (int y = 0; y < 4; y++) out[y] = _grid[y * 4 + x];
}

void Game2048Logic::writeCol(int x, const uint16_t* in) {
  for (int y = 0; y < 4; y++) _grid[y * 4 + x] = in[y];
}

bool Game2048Logic::slide(Dir d, bool spawn) {
  bool moved = false;
  if (d == LEFT) {
    for (int y = 0; y < 4; y++) moved |= slideRow(&_grid[y * 4]);
  } else if (d == RIGHT) {
    for (int y = 0; y < 4; y++) {
      uint16_t* row = &_grid[y * 4];
      reverseRow(row); moved |= slideRow(row); reverseRow(row);
    }
  } else if (d == UP) {
    for (int x = 0; x < 4; x++) {
      uint16_t col[4]; copyCol(x, col);
      bool m = slideRow(col);
      writeCol(x, col); moved |= m;
    }
  } else { // DOWN
    for (int x = 0; x < 4; x++) {
      uint16_t col[4]; copyCol(x, col);
      reverseRow(col);
      bool m = slideRow(col);
      reverseRow(col);
      writeCol(x, col); moved |= m;
    }
  }
  if (moved && spawn) spawnTile();
  return moved;
}

bool Game2048Logic::hasMoves() const {
  for (int i = 0; i < 16; i++) if (_grid[i] == 0) return true;
  for (int y = 0; y < 4; y++)
    for (int x = 0; x < 3; x++)
      if (_grid[y * 4 + x] == _grid[y * 4 + x + 1]) return true;
  for (int y = 0; y < 3; y++)
    for (int x = 0; x < 4; x++)
      if (_grid[y * 4 + x] == _grid[(y + 1) * 4 + x]) return true;
  return false;
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"

static uint16_t tileColor(uint16_t v) {
  switch (v) {
    case 2:    return 0xCE59;
    case 4:    return 0xBE16;
    case 8:    return 0xFC00;
    case 16:   return 0xFB80;
    case 32:   return 0xF8C0;
    case 64:   return 0xF800;
    case 128:  return 0xEE85;
    case 256:  return 0xEE61;
    case 512:  return 0xE603;
    case 1024: return 0xE5E1;
    case 2048: return 0xE5C0;
    default:   return 0x4208;
  }
}

void Game2048::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("2048");
  _done = false; _gameOver = false;
  _sprite.setColorDepth(16);
  _sprite.createSprite(320, 240);
  _logic.initWithSeed(millis());
}

void Game2048::update(const InputEvent& input) {
  if (_done) return;
  if (_gameOver) {
    if (input.type == InputEvent::TAP) {
      _scores->setHighScore("2048", _logic.score());
      _hiScore = _scores->getHighScore("2048");
      _done = true;
    }
    return;
  }
  Game2048Logic::Dir d;
  bool valid = false;
  switch (input.type) {
    case InputEvent::SWIPE_LEFT:  d = Game2048Logic::LEFT;  valid = true; break;
    case InputEvent::SWIPE_RIGHT: d = Game2048Logic::RIGHT; valid = true; break;
    case InputEvent::SWIPE_UP:    d = Game2048Logic::UP;    valid = true; break;
    case InputEvent::SWIPE_DOWN:  d = Game2048Logic::DOWN;  valid = true; break;
    default: break;
  }
  if (valid) {
    _logic.slide(d, true);
    if (!_logic.hasMoves()) _gameOver = true;
  }
}

void Game2048::draw() {
  TFT_eSprite& s = _sprite;
  s.fillSprite(s.color24to16(Theme::BG));

  // Board: 4×4 grid centered, cell=52, gap=4 → 4*52+5*4 = 228; offset x=46
  const int CELL = 52, GAP = 4;
  const int OX = 46, OY = 4;
  s.fillRoundRect(OX - 4, OY - 4, 4 * CELL + 5 * GAP, 4 * CELL + 5 * GAP, 6,
                  s.color24to16(Theme::DARK));
  for (int y = 0; y < 4; y++) {
    for (int x = 0; x < 4; x++) {
      int px = OX + x * (CELL + GAP);
      int py = OY + y * (CELL + GAP);
      uint16_t v = _logic.cell(x, y);
      uint16_t col = (v == 0) ? s.color24to16(Theme::SECONDARY) : tileColor(v);
      s.fillRoundRect(px, py, CELL, CELL, 4, col);
      if (v > 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "%u", v);
        s.setTextColor(s.color24to16(Theme::TEXT), col);
        int font = (v < 100) ? 4 : (v < 1000 ? 4 : 2);
        int tw = s.textWidth(buf, font);
        s.drawString(buf, px + (CELL - tw) / 2, py + (CELL / 2 - 12), font);
      }
    }
  }

  char buf[32];
  s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
  snprintf(buf, sizeof(buf), "SCORE %lu", (unsigned long)_logic.score());
  s.drawString(buf, 4, 224, 2);
  snprintf(buf, sizeof(buf), "HI %lu", (unsigned long)_hiScore);
  s.drawString(buf, 240, 224, 2);

  if (_gameOver) {
    s.setTextColor(s.color24to16(Theme::ACCENT), s.color24to16(Theme::BG));
    s.fillRect(70, 90, 180, 60, s.color24to16(Theme::BG));
    s.drawString("GAME OVER", 90, 95, 4);
    s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
    s.drawString("TAP TO EXIT", 100, 130, 2);
  }

  s.pushSprite(0, 0);
}

void Game2048::end() { _sprite.deleteSprite(); }
#endif
```

- [ ] **Step 4: Test, register, compile, flash**

```bash
make test_2048
```

Expected: tests pass.

Register in `paper-arcade.ino`. Then:

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

Verify: 2048 card. Swipe in any direction merges tiles. Game over when grid fills.

- [ ] **Step 5: Commit**

```bash
git add src/games/Game2048.h src/games/Game2048.cpp tests/test_2048.cpp paper-arcade.ino
git commit -m "feat: 2048 — swipe merge with classic tile colors"
```

---

### Task 12: Breakout

**Files:**
- Create: `src/games/Breakout.h`
- Create: `src/games/Breakout.cpp`
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Write src/games/Breakout.h**

```cpp
#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class Breakout : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "BREAKOUT"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  static const int BRICK_COLS = 10, BRICK_ROWS = 5;
  static const int BRICK_W = 30, BRICK_H = 12;
  static const int PADDLE_W = 50, PADDLE_H = 6;

  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  TFT_eSprite   _sprite{nullptr};

  bool   _bricks[BRICK_ROWS * BRICK_COLS];
  int    _bricksRemaining = 0;
  float  _bx = 160, _by = 180;
  float  _vx = 1.4f, _vy = -1.6f;
  int    _paddleX = 135;
  uint32_t _lastTick = 0;
  uint32_t _score = 0;
  uint32_t _hiScore = 0;
  bool   _done = false;
  bool   _gameOver = false;
  uint8_t _lives = 3;

  void resetBall();
  void resetBricks();
  void brickHit(int idx);
};
#endif
```

- [ ] **Step 2: Write src/games/Breakout.cpp**

```cpp
#include "Breakout.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"

void Breakout::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("breakout");
  _done = false; _gameOver = false;
  _score = 0;  _lives = 3;
  _sprite.setColorDepth(16);
  _sprite.createSprite(320, 240);
  resetBricks();
  resetBall();
  _lastTick = millis();
}

void Breakout::resetBall() {
  _bx = 160; _by = 180;
  _vx = 1.4f; _vy = -1.6f;
}

void Breakout::resetBricks() {
  _bricksRemaining = 0;
  for (int i = 0; i < BRICK_ROWS * BRICK_COLS; i++) {
    _bricks[i] = true;
    _bricksRemaining++;
  }
}

void Breakout::brickHit(int idx) {
  if (!_bricks[idx]) return;
  _bricks[idx] = false;
  _bricksRemaining--;
  _score += 10;
}

void Breakout::update(const InputEvent& input) {
  if (_done) return;
  if (_gameOver) {
    if (input.type == InputEvent::TAP) {
      _scores->setHighScore("breakout", _score);
      _hiScore = _scores->getHighScore("breakout");
      _done = true;
    }
    return;
  }

  if (input.type == InputEvent::TAP || input.type == InputEvent::DRAG) {
    _paddleX = input.x - PADDLE_W / 2;
    if (_paddleX < 0) _paddleX = 0;
    if (_paddleX + PADDLE_W > 320) _paddleX = 320 - PADDLE_W;
  }

  uint32_t now = millis();
  uint32_t dt = now - _lastTick;
  if (dt > 50) dt = 50;
  _lastTick = now;
  if (dt < 16) return;
  int steps = dt / 8;

  for (int s = 0; s < steps; s++) {
    _bx += _vx;
    _by += _vy;

    // Walls
    if (_bx < 4)            { _bx = 4;       _vx = -_vx; }
    if (_bx > 316)          { _bx = 316;     _vx = -_vx; }
    if (_by < 4)            { _by = 4;       _vy = -_vy; }

    // Paddle (y=215, height=6, width=50, paddleX is left edge)
    if (_by >= 215 && _by <= 222 && _bx >= _paddleX && _bx <= _paddleX + PADDLE_W) {
      _vy = -fabsf(_vy);
      _vx = ((_bx - (_paddleX + PADDLE_W / 2)) / (PADDLE_W / 2.0f)) * 2.0f;
      _by = 215;
    }

    // Bricks: top area y=10..70
    if (_by >= 10 && _by < 10 + BRICK_ROWS * BRICK_H) {
      int row = (int)((_by - 10) / BRICK_H);
      int col = (int)(_bx / (BRICK_W + 1));
      if (col >= 0 && col < BRICK_COLS && row >= 0 && row < BRICK_ROWS) {
        int idx = row * BRICK_COLS + col;
        if (_bricks[idx]) {
          brickHit(idx);
          _vy = -_vy;
        }
      }
    }

    // Lost ball
    if (_by > 240) {
      _lives--;
      if (_lives == 0) _gameOver = true;
      else resetBall();
    }
  }

  if (_bricksRemaining == 0) _gameOver = true;
}

void Breakout::draw() {
  TFT_eSprite& s = _sprite;
  s.fillSprite(s.color24to16(Theme::BG));

  // Bricks
  for (int r = 0; r < BRICK_ROWS; r++) {
    uint16_t col = s.color24to16(0xff5500 + r * 0x000022);
    for (int c = 0; c < BRICK_COLS; c++) {
      if (_bricks[r * BRICK_COLS + c]) {
        s.fillRect(c * (BRICK_W + 1), 10 + r * BRICK_H, BRICK_W, BRICK_H - 1, col);
      }
    }
  }

  // Paddle
  s.fillRoundRect(_paddleX, 215, PADDLE_W, PADDLE_H, 2, s.color24to16(Theme::ACCENT));

  // Ball
  s.fillCircle((int)_bx, (int)_by, 4, s.color24to16(Theme::TEXT));

  // HUD
  char buf[32];
  s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
  snprintf(buf, sizeof(buf), "SCORE %lu", (unsigned long)_score);
  s.drawString(buf, 4, 224, 2);
  snprintf(buf, sizeof(buf), "LIVES %d", _lives);
  s.drawString(buf, 200, 224, 2);

  if (_gameOver) {
    s.setTextColor(s.color24to16(Theme::ACCENT), s.color24to16(Theme::BG));
    s.fillRect(70, 90, 180, 60, s.color24to16(Theme::BG));
    s.drawString(_bricksRemaining == 0 ? "YOU WIN!" : "GAME OVER", 80, 95, 4);
    s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
    s.drawString("TAP TO EXIT", 100, 130, 2);
  }
  s.pushSprite(0, 0);
}

void Breakout::end() { _sprite.deleteSprite(); }
#endif
```

- [ ] **Step 3: Register, compile, flash, verify**

In `paper-arcade.ino` uncomment Breakout include + addGame.

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

Verify: BREAKOUT card. Drag finger to move paddle. Ball bounces off bricks. Lose all 3 balls = GAME OVER.

- [ ] **Step 4: Commit**

```bash
git add src/games/Breakout.h src/games/Breakout.cpp paper-arcade.ino
git commit -m "feat: Breakout — drag paddle, brick grid, 3 lives"
```

---

### Task 13: Flappy Bird

**Files:**
- Create: `src/games/FlappyBird.h`
- Create: `src/games/FlappyBird.cpp`
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Write src/games/FlappyBird.h**

```cpp
#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class FlappyBird : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "FLAPPY"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  static const int BIRD_X = 60;
  static const int PIPE_W = 36;
  static const int GAP_H  = 80;
  static const int PIPE_COUNT = 3;

  struct Pipe { int x; int gapY; bool scored; };

  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  TFT_eSprite   _sprite{nullptr};
  float    _birdY = 120;
  float    _vy    = 0;
  Pipe     _pipes[PIPE_COUNT];
  uint32_t _score = 0;
  uint32_t _hiScore = 0;
  uint32_t _lastTick = 0;
  bool     _done = false;
  bool     _gameOver = false;

  void resetPipes();
  int  randGapY();
};
#endif
```

- [ ] **Step 2: Write src/games/FlappyBird.cpp**

```cpp
#include "FlappyBird.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include <cstdlib>

int FlappyBird::randGapY() {
  return 40 + (rand() % (180 - GAP_H));
}

void FlappyBird::resetPipes() {
  for (int i = 0; i < PIPE_COUNT; i++) {
    _pipes[i].x = 320 + i * 110;
    _pipes[i].gapY = randGapY();
    _pipes[i].scored = false;
  }
}

void FlappyBird::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("flappy");
  _done = false; _gameOver = false;
  _score = 0;
  _birdY = 120; _vy = 0;
  _sprite.setColorDepth(16);
  _sprite.createSprite(320, 240);
  srand(millis());
  resetPipes();
  _lastTick = millis();
}

void FlappyBird::update(const InputEvent& input) {
  if (_done) return;
  if (_gameOver) {
    if (input.type == InputEvent::TAP) {
      _scores->setHighScore("flappy", _score);
      _hiScore = _scores->getHighScore("flappy");
      _done = true;
    }
    return;
  }

  if (input.type == InputEvent::TAP) _vy = -3.5f;

  uint32_t now = millis();
  uint32_t dt = now - _lastTick;
  if (dt > 50) dt = 50;
  _lastTick = now;
  if (dt < 16) return;
  int steps = dt / 16;

  for (int s = 0; s < steps; s++) {
    _vy += 0.30f;
    _birdY += _vy;

    // Pipes scroll left
    for (int i = 0; i < PIPE_COUNT; i++) {
      _pipes[i].x -= 2;
      if (_pipes[i].x + PIPE_W < 0) {
        _pipes[i].x += PIPE_COUNT * 110;
        _pipes[i].gapY = randGapY();
        _pipes[i].scored = false;
      }
      // Score
      if (!_pipes[i].scored && _pipes[i].x + PIPE_W < BIRD_X) {
        _pipes[i].scored = true;
        _score++;
      }
      // Collision
      if (BIRD_X + 8 > _pipes[i].x && BIRD_X - 8 < _pipes[i].x + PIPE_W) {
        if (_birdY - 8 < _pipes[i].gapY || _birdY + 8 > _pipes[i].gapY + GAP_H) {
          _gameOver = true;
        }
      }
    }
    // Floor / ceiling
    if (_birdY > 232 || _birdY < 8) _gameOver = true;
  }
}

void FlappyBird::draw() {
  TFT_eSprite& s = _sprite;
  s.fillSprite(s.color24to16(Theme::BG));

  // Pipes
  uint16_t pipeCol = s.color24to16(Theme::SECONDARY);
  for (int i = 0; i < PIPE_COUNT; i++) {
    Pipe& p = _pipes[i];
    s.fillRect(p.x, 0, PIPE_W, p.gapY, pipeCol);
    s.fillRect(p.x, p.gapY + GAP_H, PIPE_W, 240 - (p.gapY + GAP_H), pipeCol);
    s.drawRect(p.x, 0, PIPE_W, p.gapY, s.color24to16(Theme::ACCENT));
    s.drawRect(p.x, p.gapY + GAP_H, PIPE_W, 240 - (p.gapY + GAP_H), s.color24to16(Theme::ACCENT));
  }

  // Bird (yellow circle with eye)
  s.fillCircle(BIRD_X, (int)_birdY, 8, 0xFFE0);
  s.fillCircle(BIRD_X + 3, (int)_birdY - 2, 2, 0x0000);

  // Score
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)_score);
  s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
  s.drawString(buf, 4, 4, 4);

  if (_gameOver) {
    s.setTextColor(s.color24to16(Theme::ACCENT), s.color24to16(Theme::BG));
    s.fillRect(70, 90, 180, 60, s.color24to16(Theme::BG));
    s.drawString("GAME OVER", 80, 95, 4);
    s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
    s.drawString("TAP TO EXIT", 100, 130, 2);
  }
  s.pushSprite(0, 0);
}

void FlappyBird::end() { _sprite.deleteSprite(); }
#endif
```

- [ ] **Step 3: Register, compile, flash, verify**

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

Verify: FLAPPY card. Tap to flap. Pipes scroll. Score increments per pipe.

- [ ] **Step 4: Commit**

```bash
git add src/games/FlappyBird.h src/games/FlappyBird.cpp paper-arcade.ino
git commit -m "feat: Flappy Bird — gravity, tap-to-flap, scrolling pipes"
```

---

### Task 14: Tetris

**Files:**
- Create: `src/games/Tetris.h`
- Create: `src/games/Tetris.cpp`
- Create: `tests/test_tetris.cpp`
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Write tests/test_tetris.cpp**

```cpp
#include "test_runner.h"
#include "games/Tetris.h"

int main() {
  TetrisLogic t;
  t.init(7);   // seed

  ASSERT_EQ(t.score(), 0);

  // Spawn a piece (init does this)
  ASSERT_TRUE(t.activePieceY() >= 0);

  // Move left/right within bounds
  int x0 = t.activePieceX();
  t.tryMove(-1);
  ASSERT_EQ(t.activePieceX(), x0 - 1);
  t.tryMove(1);
  ASSERT_EQ(t.activePieceX(), x0);

  // Hard drop should land piece and spawn next
  t.hardDrop();
  // After drop, a new piece spawns at top
  ASSERT_TRUE(t.activePieceY() <= 1);

  TEST_SUMMARY();
}
```

- [ ] **Step 2: Write src/games/Tetris.h**

```cpp
#pragma once
#include <cstdint>

class TetrisLogic {
public:
  static const int W = 10, H = 20;

  void init(uint32_t seed);
  bool tick();              // gravity step; returns false on game over
  bool tryMove(int dx);     // false if blocked
  bool tryRotate();
  void hardDrop();

  uint8_t cell(int x, int y) const { return _board[y][x]; }
  int  activePieceX() const { return _px; }
  int  activePieceY() const { return _py; }
  int  activePieceType() const { return _ptype; }
  int  activePieceRot() const { return _prot; }
  uint32_t score() const { return _score; }
  bool gameOver() const { return _over; }

  // Returns 4 (x,y) positions of the active piece blocks
  void pieceBlocks(int outX[4], int outY[4]) const;

private:
  uint8_t _board[H][W] = {};
  int     _ptype = 0, _prot = 0;
  int     _px = 0, _py = 0;
  uint32_t _score = 0;
  uint32_t _rng = 1;
  bool    _over = false;

  uint32_t nextRand();
  void     spawn();
  bool     collides(int type, int rot, int px, int py) const;
  void     lock();
  int      clearLines();
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class Tetris : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "TETRIS"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  static const int CELL = 11;
  static const int OFFX = 105, OFFY = 8;

  TetrisLogic   _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  TFT_eSprite   _sprite{nullptr};
  uint32_t      _lastFall = 0;
  uint32_t      _hiScore = 0;
  bool          _done = false;
};
#endif
```

- [ ] **Step 3: Write src/games/Tetris.cpp**

```cpp
#include "Tetris.h"
#include <cstring>

// Standard 7 tetromino shapes, 4 rotations each, 4×4 grid (bitmask)
// Bit i means cell (i%4, i/4) is filled
static const uint16_t SHAPES[7][4] = {
  // I
  { 0x0F00, 0x2222, 0x00F0, 0x4444 },
  // O
  { 0x0660, 0x0660, 0x0660, 0x0660 },
  // T
  { 0x0E40, 0x4C40, 0x4E00, 0x4640 },
  // S
  { 0x06C0, 0x4620, 0x06C0, 0x4620 },
  // Z
  { 0x0C60, 0x2640, 0x0C60, 0x2640 },
  // J
  { 0x44C0, 0x8E00, 0x6440, 0x0E20 },
  // L
  { 0x4460, 0x0E80, 0xC440, 0x2E00 }
};

uint32_t TetrisLogic::nextRand() {
  _rng ^= _rng << 13;
  _rng ^= _rng >> 17;
  _rng ^= _rng << 5;
  return _rng;
}

void TetrisLogic::init(uint32_t seed) {
  std::memset(_board, 0, sizeof(_board));
  _rng = seed ? seed : 1;
  _score = 0;
  _over = false;
  spawn();
}

void TetrisLogic::spawn() {
  _ptype = nextRand() % 7;
  _prot  = 0;
  _px = 3;
  _py = 0;
  if (collides(_ptype, _prot, _px, _py)) _over = true;
}

bool TetrisLogic::collides(int type, int rot, int px, int py) const {
  uint16_t shape = SHAPES[type][rot];
  for (int b = 0; b < 16; b++) {
    if (!(shape & (1 << b))) continue;
    int x = px + (b % 4);
    int y = py + (b / 4);
    if (x < 0 || x >= W || y >= H) return true;
    if (y < 0) continue;
    if (_board[y][x]) return true;
  }
  return false;
}

bool TetrisLogic::tryMove(int dx) {
  if (_over) return false;
  if (collides(_ptype, _prot, _px + dx, _py)) return false;
  _px += dx;
  return true;
}

bool TetrisLogic::tryRotate() {
  if (_over) return false;
  int next = (_prot + 1) % 4;
  if (collides(_ptype, next, _px, _py)) return false;
  _prot = next;
  return true;
}

void TetrisLogic::lock() {
  uint16_t shape = SHAPES[_ptype][_prot];
  for (int b = 0; b < 16; b++) {
    if (!(shape & (1 << b))) continue;
    int x = _px + (b % 4);
    int y = _py + (b / 4);
    if (y >= 0 && y < H) _board[y][x] = (uint8_t)(_ptype + 1);
  }
}

int TetrisLogic::clearLines() {
  int cleared = 0;
  for (int y = H - 1; y >= 0; ) {
    bool full = true;
    for (int x = 0; x < W; x++) if (!_board[y][x]) { full = false; break; }
    if (full) {
      for (int yy = y; yy > 0; yy--)
        std::memcpy(_board[yy], _board[yy - 1], W);
      std::memset(_board[0], 0, W);
      cleared++;
    } else {
      y--;
    }
  }
  static const uint32_t SCORE[5] = { 0, 100, 300, 500, 800 };
  if (cleared >= 0 && cleared <= 4) _score += SCORE[cleared];
  return cleared;
}

bool TetrisLogic::tick() {
  if (_over) return false;
  if (collides(_ptype, _prot, _px, _py + 1)) {
    lock();
    clearLines();
    spawn();
    return !_over;
  }
  _py++;
  return true;
}

void TetrisLogic::hardDrop() {
  if (_over) return;
  while (!collides(_ptype, _prot, _px, _py + 1)) _py++;
  lock();
  clearLines();
  spawn();
}

void TetrisLogic::pieceBlocks(int outX[4], int outY[4]) const {
  uint16_t shape = SHAPES[_ptype][_prot];
  int n = 0;
  for (int b = 0; b < 16; b++) {
    if (!(shape & (1 << b))) continue;
    if (n >= 4) break;
    outX[n] = _px + (b % 4);
    outY[n] = _py + (b / 4);
    n++;
  }
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"

static const uint16_t PIECE_COLORS[8] = {
  0x0000, 0x07FF, 0xFFE0, 0xF81F, 0x07E0, 0xF800, 0x001F, 0xFD20
};

void Tetris::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("tetris");
  _done = false;
  _sprite.setColorDepth(16);
  _sprite.createSprite(320, 240);
  _logic.init(millis());
  _lastFall = millis();
}

void Tetris::update(const InputEvent& input) {
  if (_done) return;
  if (_logic.gameOver()) {
    if (input.type == InputEvent::TAP) {
      _scores->setHighScore("tetris", _logic.score());
      _hiScore = _scores->getHighScore("tetris");
      _done = true;
    }
    return;
  }

  // Tap zones (320×240 screen, board occupies x=105..215)
  if (input.type == InputEvent::TAP) {
    if (input.x < 35) {
      // Top corner (y<60) = rotate; rest = move left
      if (input.y < 60) _logic.tryRotate();
      else              _logic.tryMove(-1);
    } else if (input.x > 285) {
      _logic.tryMove(1);
    } else if (input.y > 200) {
      _logic.hardDrop();
    }
  }

  uint32_t now = millis();
  uint32_t fallMs = 600 - (_logic.score() / 500) * 50;
  if (fallMs < 100) fallMs = 100;
  if (now - _lastFall >= fallMs) {
    _lastFall = now;
    _logic.tick();
  }
}

void Tetris::draw() {
  TFT_eSprite& s = _sprite;
  s.fillSprite(s.color24to16(Theme::BG));

  // Board border
  s.drawRect(OFFX - 1, OFFY - 1, TetrisLogic::W * CELL + 2, TetrisLogic::H * CELL + 2,
             s.color24to16(Theme::ACCENT));

  // Stack
  for (int y = 0; y < TetrisLogic::H; y++) {
    for (int x = 0; x < TetrisLogic::W; x++) {
      uint8_t v = _logic.cell(x, y);
      if (v)
        s.fillRect(OFFX + x * CELL, OFFY + y * CELL, CELL - 1, CELL - 1, PIECE_COLORS[v]);
    }
  }

  // Active piece
  int bx[4], by[4];
  _logic.pieceBlocks(bx, by);
  uint16_t pcol = PIECE_COLORS[_logic.activePieceType() + 1];
  for (int i = 0; i < 4; i++) {
    if (by[i] < 0) continue;
    s.fillRect(OFFX + bx[i] * CELL, OFFY + by[i] * CELL, CELL - 1, CELL - 1, pcol);
  }

  // Side HUD
  s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
  s.drawString("SCORE", 230, 12, 2);
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)_logic.score());
  s.drawString(buf, 230, 32, 4);
  s.setTextColor(s.color24to16(Theme::DIM), s.color24to16(Theme::BG));
  s.drawString("HI", 230, 80, 2);
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)_hiScore);
  s.drawString(buf, 230, 100, 2);

  // Hint icons (control zones)
  s.setTextColor(s.color24to16(Theme::SECONDARY), s.color24to16(Theme::BG));
  s.drawString("ROT", 5, 30, 2);
  s.drawString("LEFT", 5, 110, 2);
  s.drawString("RIGHT", 285, 110, 2);
  s.drawString("DROP", 130, 218, 2);

  if (_logic.gameOver()) {
    s.setTextColor(s.color24to16(Theme::ACCENT), s.color24to16(Theme::BG));
    s.fillRect(70, 90, 180, 60, s.color24to16(Theme::BG));
    s.drawString("GAME OVER", 90, 95, 4);
    s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
    s.drawString("TAP TO EXIT", 100, 130, 2);
  }

  s.pushSprite(0, 0);
}

void Tetris::end() { _sprite.deleteSprite(); }
#endif
```

- [ ] **Step 4: Test, register, compile, flash**

```bash
make test_tetris
```

Register in `paper-arcade.ino`. Then flash and verify: TETRIS card. Pieces fall. Left strip = move left, right strip = move right, top-left corner = rotate, bottom = hard drop.

- [ ] **Step 5: Commit**

```bash
git add src/games/Tetris.h src/games/Tetris.cpp tests/test_tetris.cpp paper-arcade.ino
git commit -m "feat: Tetris — 7 pieces, line clear, tap-zone controls"
```

---

### Task 15: Space Invaders

**Files:**
- Create: `src/games/SpaceInvaders.h`
- Create: `src/games/SpaceInvaders.cpp`
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Write src/games/SpaceInvaders.h**

```cpp
#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class SpaceInvaders : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "INVADERS"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  static const int COLS = 8, ROWS = 4;
  static const int INV_W = 18, INV_H = 12, INV_GAP_X = 6, INV_GAP_Y = 8;
  static const int PLAYER_Y = 215, PLAYER_W = 22, PLAYER_H = 10;
  static const int MAX_BULLETS = 4, MAX_INV_BULLETS = 3;

  struct Bullet { int x; int y; bool active; };

  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  TFT_eSprite   _sprite{nullptr};

  bool   _alive[ROWS * COLS];
  int    _aliveCount = 0;
  int    _swarmX = 0, _swarmY = 0;
  int    _swarmDir = 1;
  uint32_t _swarmStep = 0, _swarmStepMs = 700;
  int    _playerX = 150;
  Bullet _bullets[MAX_BULLETS];
  Bullet _invBullets[MAX_INV_BULLETS];
  uint32_t _lastInvShot = 0;
  uint32_t _score = 0;
  uint32_t _hiScore = 0;
  uint8_t  _lives = 3;
  bool   _done = false;
  bool   _gameOver = false;

  void shoot();
  void tickSwarm();
  void tickBullets();
  bool checkCollisions();
};
#endif
```

- [ ] **Step 2: Write src/games/SpaceInvaders.cpp**

```cpp
#include "SpaceInvaders.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include <cstdlib>

void SpaceInvaders::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("invaders");
  _done = false; _gameOver = false;
  _score = 0;  _lives = 3;
  _aliveCount = ROWS * COLS;
  for (int i = 0; i < _aliveCount; i++) _alive[i] = true;
  _swarmX = 10; _swarmY = 20;
  _swarmDir = 1; _swarmStepMs = 700;
  _playerX = 150;
  for (int i = 0; i < MAX_BULLETS; i++) _bullets[i].active = false;
  for (int i = 0; i < MAX_INV_BULLETS; i++) _invBullets[i].active = false;
  _sprite.setColorDepth(16);
  _sprite.createSprite(320, 240);
  srand(millis());
  _swarmStep = millis();
  _lastInvShot = millis();
}

void SpaceInvaders::shoot() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!_bullets[i].active) {
      _bullets[i] = { _playerX + PLAYER_W / 2, PLAYER_Y - 4, true };
      return;
    }
  }
}

void SpaceInvaders::tickSwarm() {
  if (millis() - _swarmStep < _swarmStepMs) return;
  _swarmStep = millis();

  // Find leftmost/rightmost alive column
  int minCol = COLS, maxCol = -1;
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      if (_alive[r * COLS + c]) { if (c < minCol) minCol = c; if (c > maxCol) maxCol = c; }

  int leftX  = _swarmX + minCol * (INV_W + INV_GAP_X);
  int rightX = _swarmX + maxCol * (INV_W + INV_GAP_X) + INV_W;

  if ((_swarmDir > 0 && rightX + 4 >= 320) || (_swarmDir < 0 && leftX - 4 <= 0)) {
    _swarmDir = -_swarmDir;
    _swarmY += 8;
  } else {
    _swarmX += _swarmDir * 4;
  }

  // Speed up as fewer remain
  _swarmStepMs = 200 + (uint32_t)_aliveCount * 12;

  // Random invader shoot
  if (millis() - _lastInvShot > 700 + (rand() % 1500)) {
    _lastInvShot = millis();
    for (int i = 0; i < MAX_INV_BULLETS; i++) {
      if (_invBullets[i].active) continue;
      // Pick a random alive column, find lowest alive in that column
      int c = rand() % COLS;
      for (int r = ROWS - 1; r >= 0; r--) {
        if (_alive[r * COLS + c]) {
          int sx = _swarmX + c * (INV_W + INV_GAP_X) + INV_W / 2;
          int sy = _swarmY + r * (INV_H + INV_GAP_Y) + INV_H;
          _invBullets[i] = { sx, sy, true };
          break;
        }
      }
      break;
    }
  }
}

void SpaceInvaders::tickBullets() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!_bullets[i].active) continue;
    _bullets[i].y -= 6;
    if (_bullets[i].y < 0) _bullets[i].active = false;
  }
  for (int i = 0; i < MAX_INV_BULLETS; i++) {
    if (!_invBullets[i].active) continue;
    _invBullets[i].y += 4;
    if (_invBullets[i].y > 240) _invBullets[i].active = false;
  }
}

bool SpaceInvaders::checkCollisions() {
  // Player bullets vs invaders
  for (int b = 0; b < MAX_BULLETS; b++) {
    if (!_bullets[b].active) continue;
    int bx = _bullets[b].x, by = _bullets[b].y;
    for (int r = 0; r < ROWS; r++) {
      for (int c = 0; c < COLS; c++) {
        if (!_alive[r * COLS + c]) continue;
        int ix = _swarmX + c * (INV_W + INV_GAP_X);
        int iy = _swarmY + r * (INV_H + INV_GAP_Y);
        if (bx >= ix && bx <= ix + INV_W && by >= iy && by <= iy + INV_H) {
          _alive[r * COLS + c] = false;
          _aliveCount--;
          _bullets[b].active = false;
          _score += (ROWS - r) * 10;
          break;
        }
      }
    }
  }
  // Invader bullets vs player
  for (int b = 0; b < MAX_INV_BULLETS; b++) {
    if (!_invBullets[b].active) continue;
    int bx = _invBullets[b].x, by = _invBullets[b].y;
    if (by >= PLAYER_Y && bx >= _playerX && bx <= _playerX + PLAYER_W) {
      _invBullets[b].active = false;
      _lives--;
      if (_lives == 0) return true;
    }
  }
  // Swarm reached player
  if (_swarmY + ROWS * (INV_H + INV_GAP_Y) >= PLAYER_Y) return true;
  return false;
}

void SpaceInvaders::update(const InputEvent& input) {
  if (_done) return;
  if (_gameOver) {
    if (input.type == InputEvent::TAP) {
      _scores->setHighScore("invaders", _score);
      _hiScore = _scores->getHighScore("invaders");
      _done = true;
    }
    return;
  }

  if (input.type == InputEvent::TAP) {
    if (input.x < 40)        _playerX -= 12;
    else if (input.x > 280)  _playerX += 12;
    else if (input.y > 180)  shoot();
    if (_playerX < 0) _playerX = 0;
    if (_playerX + PLAYER_W > 320) _playerX = 320 - PLAYER_W;
  }

  tickSwarm();
  tickBullets();
  if (checkCollisions() || _aliveCount == 0) _gameOver = true;
}

void SpaceInvaders::draw() {
  TFT_eSprite& s = _sprite;
  s.fillSprite(s.color24to16(Theme::BG));

  // Invaders
  for (int r = 0; r < ROWS; r++) {
    uint16_t col = (r == 0) ? 0xF800 : (r == 1 ? 0xFE40 : (r == 2 ? 0x07E0 : 0x07FF));
    for (int c = 0; c < COLS; c++) {
      if (!_alive[r * COLS + c]) continue;
      int ix = _swarmX + c * (INV_W + INV_GAP_X);
      int iy = _swarmY + r * (INV_H + INV_GAP_Y);
      s.fillRect(ix, iy, INV_W, INV_H, col);
      s.fillRect(ix + 4, iy + 3, 3, 3, 0x0000);
      s.fillRect(ix + 11, iy + 3, 3, 3, 0x0000);
    }
  }

  // Player
  s.fillRect(_playerX, PLAYER_Y, PLAYER_W, PLAYER_H, s.color24to16(Theme::DIM));
  s.fillRect(_playerX + 9, PLAYER_Y - 4, 4, 4, s.color24to16(Theme::DIM));

  // Bullets
  for (int i = 0; i < MAX_BULLETS; i++)
    if (_bullets[i].active)
      s.fillRect(_bullets[i].x, _bullets[i].y, 2, 6, s.color24to16(Theme::TEXT));
  for (int i = 0; i < MAX_INV_BULLETS; i++)
    if (_invBullets[i].active)
      s.fillRect(_invBullets[i].x, _invBullets[i].y, 2, 6, s.color24to16(Theme::ACCENT));

  // HUD
  char buf[32];
  s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)_score);
  s.drawString(buf, 4, 4, 2);
  snprintf(buf, sizeof(buf), "LIVES %d", _lives);
  s.drawString(buf, 240, 4, 2);

  if (_gameOver) {
    s.setTextColor(s.color24to16(Theme::ACCENT), s.color24to16(Theme::BG));
    s.fillRect(70, 90, 180, 60, s.color24to16(Theme::BG));
    s.drawString(_aliveCount == 0 ? "VICTORY!" : "GAME OVER", 80, 95, 4);
    s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
    s.drawString("TAP TO EXIT", 100, 130, 2);
  }
  s.pushSprite(0, 0);
}

void SpaceInvaders::end() { _sprite.deleteSprite(); }
#endif
```

- [ ] **Step 3: Register, compile, flash, verify**

In `paper-arcade.ino` uncomment SpaceInvaders include + addGame.

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

Verify: INVADERS card. Left strip = move left, right strip = move right, bottom = shoot. Swarm marches and shoots back.

- [ ] **Step 4: Commit**

```bash
git add src/games/SpaceInvaders.h src/games/SpaceInvaders.cpp paper-arcade.ino
git commit -m "feat: Space Invaders — marching swarm, player and invader bullets"
```

---

### Task 16: Pac-Man (simplified)

**Files:**
- Create: `src/games/PacMan.h`
- Create: `src/games/PacMan.cpp`
- Modify: `paper-arcade.ino`

- [ ] **Step 1: Write src/games/PacMan.h**

```cpp
#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class PacMan : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "PACMAN"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  static const int COLS = 16, ROWS = 11, TILE = 20;
  enum CellType { WALL=1, DOT=0, PELLET=2, EMPTY=3 };

  // 0=dot, 1=wall, 2=power pellet
  static const uint8_t MAZE[ROWS][COLS];
  uint8_t _grid[ROWS][COLS];   // mutable copy

  enum Dir : uint8_t { D_UP, D_DOWN, D_LEFT, D_RIGHT, D_NONE };

  struct Actor { int8_t x, y; Dir dir; uint8_t pixOff; };
  Actor _pac;
  Dir   _pacNext = D_NONE;
  Actor _ghosts[3];
  bool  _ghostFrightened = false;
  uint32_t _frightUntil = 0;

  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  TFT_eSprite   _sprite{nullptr};
  uint32_t      _lastTick = 0;
  uint32_t      _score = 0;
  uint32_t      _hiScore = 0;
  uint8_t       _lives = 3;
  int           _dotsRemaining = 0;
  bool          _done = false;
  bool          _gameOver = false;

  bool canMove(int x, int y, Dir d) const;
  void moveActor(Actor& a);
  Dir  ghostChooseDir(const Actor& g) const;
  void resetActors();
  void checkGhostCollisions();
  static int  dirDx(Dir d);
  static int  dirDy(Dir d);
  static Dir  reverseDir(Dir d);
};
#endif
```

- [ ] **Step 2: Write src/games/PacMan.cpp**

```cpp
#include "PacMan.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include <cstdlib>

const uint8_t PacMan::MAZE[ROWS][COLS] = {
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  {1,2,0,0,0,0,0,1,1,0,0,0,0,0,2,1},
  {1,0,1,1,0,1,0,1,1,0,1,0,1,1,0,1},
  {1,0,1,1,0,1,0,0,0,0,1,0,1,1,0,1},
  {1,0,0,0,0,0,0,1,1,0,0,0,0,0,0,1},
  {1,1,0,1,1,1,0,1,1,0,1,1,1,0,1,1},
  {1,1,0,1,0,0,0,0,0,0,0,0,1,0,1,1},
  {1,1,0,1,0,1,1,0,0,1,1,0,1,0,1,1},
  {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1},
  {1,0,1,1,0,0,0,1,1,0,0,0,1,1,0,1},
  {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

int PacMan::dirDx(Dir d) {
  switch (d) { case D_LEFT: return -1; case D_RIGHT: return 1; default: return 0; }
}
int PacMan::dirDy(Dir d) {
  switch (d) { case D_UP: return -1; case D_DOWN: return 1; default: return 0; }
}
PacMan::Dir PacMan::reverseDir(Dir d) {
  switch (d) {
    case D_UP: return D_DOWN; case D_DOWN: return D_UP;
    case D_LEFT: return D_RIGHT; case D_RIGHT: return D_LEFT;
    default: return D_NONE;
  }
}

bool PacMan::canMove(int x, int y, Dir d) const {
  int nx = x + dirDx(d);
  int ny = y + dirDy(d);
  if (nx < 0 || ny < 0 || nx >= COLS || ny >= ROWS) return false;
  return _grid[ny][nx] != WALL;
}

void PacMan::resetActors() {
  _pac.x = 1; _pac.y = 9; _pac.dir = D_RIGHT; _pac.pixOff = 0;
  _pacNext = D_NONE;
  _ghosts[0] = { 14, 1, D_LEFT,  0 };
  _ghosts[1] = { 14, 9, D_UP,    0 };
  _ghosts[2] = { 7,  4, D_RIGHT, 0 };
}

void PacMan::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("pacman");
  _done = false; _gameOver = false;
  _score = 0; _lives = 3;
  _dotsRemaining = 0;
  for (int y = 0; y < ROWS; y++)
    for (int x = 0; x < COLS; x++) {
      _grid[y][x] = MAZE[y][x];
      if (MAZE[y][x] == DOT || MAZE[y][x] == PELLET) _dotsRemaining++;
    }
  resetActors();
  _ghostFrightened = false;
  _sprite.setColorDepth(16);
  _sprite.createSprite(320, 240);
  srand(millis());
  _lastTick = millis();
}

void PacMan::moveActor(Actor& a) {
  // Sub-tile movement: 4 px steps per tick, TILE pixels per cell
  if (a.pixOff == 0) {
    if (!canMove(a.x, a.y, a.dir)) return;
  }
  a.pixOff += 5;
  if (a.pixOff >= TILE) {
    a.x += dirDx(a.dir);
    a.y += dirDy(a.dir);
    a.pixOff = 0;
  }
}

PacMan::Dir PacMan::ghostChooseDir(const Actor& g) const {
  // Simple AI: at intersection, prefer direction toward Pac-Man (or random if frightened)
  Dir best = g.dir;
  if (_ghostFrightened) {
    Dir options[4] = { D_UP, D_DOWN, D_LEFT, D_RIGHT };
    for (int i = 0; i < 4; i++) {
      int j = rand() % 4;
      Dir t = options[i]; options[i] = options[j]; options[j] = t;
    }
    for (int i = 0; i < 4; i++) {
      Dir d = options[i];
      if (d == reverseDir(g.dir)) continue;
      if (canMove(g.x, g.y, d)) return d;
    }
    return reverseDir(g.dir);
  }
  // Chase: pick direction that minimizes Manhattan distance to Pac-Man
  int bestD = 0x7fff;
  Dir options[4] = { D_UP, D_DOWN, D_LEFT, D_RIGHT };
  for (int i = 0; i < 4; i++) {
    Dir d = options[i];
    if (d == reverseDir(g.dir)) continue;
    if (!canMove(g.x, g.y, d)) continue;
    int nx = g.x + dirDx(d);
    int ny = g.y + dirDy(d);
    int dist = abs(nx - _pac.x) + abs(ny - _pac.y);
    if (dist < bestD) { bestD = dist; best = d; }
  }
  if (bestD == 0x7fff) return reverseDir(g.dir);
  return best;
}

void PacMan::checkGhostCollisions() {
  for (int i = 0; i < 3; i++) {
    if (_ghosts[i].x == _pac.x && _ghosts[i].y == _pac.y) {
      if (_ghostFrightened) {
        _score += 200;
        _ghosts[i].x = 7; _ghosts[i].y = 4; _ghosts[i].pixOff = 0;
      } else {
        _lives--;
        if (_lives == 0) { _gameOver = true; }
        else { resetActors(); }
        return;
      }
    }
  }
}

void PacMan::update(const InputEvent& input) {
  if (_done) return;
  if (_gameOver) {
    if (input.type == InputEvent::TAP) {
      _scores->setHighScore("pacman", _score);
      _hiScore = _scores->getHighScore("pacman");
      _done = true;
    }
    return;
  }

  // D-pad zones (3×3 grid): center=ignore, edges = direction
  if (input.type == InputEvent::TAP) {
    int col = input.x / 107;  // 0,1,2
    int row = input.y / 80;   // 0,1,2
    if (row == 0 && col == 1) _pacNext = D_UP;
    else if (row == 2 && col == 1) _pacNext = D_DOWN;
    else if (row == 1 && col == 0) _pacNext = D_LEFT;
    else if (row == 1 && col == 2) _pacNext = D_RIGHT;
  }

  uint32_t now = millis();
  if (now - _lastTick < 80) return;
  _lastTick = now;

  // Try queued direction at intersections
  if (_pac.pixOff == 0 && _pacNext != D_NONE && canMove(_pac.x, _pac.y, _pacNext)) {
    _pac.dir = _pacNext;
    _pacNext = D_NONE;
  }
  moveActor(_pac);

  // Eat dots
  if (_pac.pixOff == 0) {
    uint8_t cell = _grid[_pac.y][_pac.x];
    if (cell == DOT)    { _grid[_pac.y][_pac.x] = EMPTY; _score += 10; _dotsRemaining--; }
    if (cell == PELLET) { _grid[_pac.y][_pac.x] = EMPTY; _score += 50; _dotsRemaining--;
                          _ghostFrightened = true; _frightUntil = now + 6000; }
  }
  if (_ghostFrightened && now > _frightUntil) _ghostFrightened = false;

  // Ghosts
  for (int i = 0; i < 3; i++) {
    Actor& g = _ghosts[i];
    if (g.pixOff == 0) g.dir = ghostChooseDir(g);
    moveActor(g);
  }

  checkGhostCollisions();
  if (_dotsRemaining == 0) _gameOver = true;
}

void PacMan::draw() {
  TFT_eSprite& s = _sprite;
  s.fillSprite(s.color24to16(Theme::BG));

  // Maze
  uint16_t wallCol = s.color24to16(Theme::SECONDARY);
  uint16_t dotCol  = s.color24to16(Theme::DIM);
  uint16_t pelCol  = s.color24to16(Theme::ACCENT);

  for (int y = 0; y < ROWS; y++) {
    for (int x = 0; x < COLS; x++) {
      int px = x * TILE, py = y * TILE;
      uint8_t v = _grid[y][x];
      if (v == WALL)        s.fillRect(px, py, TILE, TILE, wallCol);
      else if (v == DOT)    s.fillCircle(px + TILE / 2, py + TILE / 2, 2, dotCol);
      else if (v == PELLET) s.fillCircle(px + TILE / 2, py + TILE / 2, 5, pelCol);
    }
  }

  // Pac-Man
  int px = _pac.x * TILE + dirDx(_pac.dir) * _pac.pixOff + TILE / 2;
  int py = _pac.y * TILE + dirDy(_pac.dir) * _pac.pixOff + TILE / 2;
  s.fillCircle(px, py, 8, 0xFFE0);

  // Ghosts
  static const uint16_t GH[3] = { 0xF800, 0xF81F, 0x07FF };
  for (int i = 0; i < 3; i++) {
    int gx = _ghosts[i].x * TILE + dirDx(_ghosts[i].dir) * _ghosts[i].pixOff + TILE / 2;
    int gy = _ghosts[i].y * TILE + dirDy(_ghosts[i].dir) * _ghosts[i].pixOff + TILE / 2;
    uint16_t col = _ghostFrightened ? 0x001F : GH[i];
    s.fillCircle(gx, gy, 8, col);
  }

  // HUD
  char buf[32];
  s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)_score);
  s.drawString(buf, 4, 222, 2);
  snprintf(buf, sizeof(buf), "LIVES %d", _lives);
  s.drawString(buf, 240, 222, 2);

  if (_gameOver) {
    s.setTextColor(s.color24to16(Theme::ACCENT), s.color24to16(Theme::BG));
    s.fillRect(70, 90, 180, 60, s.color24to16(Theme::BG));
    s.drawString(_dotsRemaining == 0 ? "YOU WIN!" : "GAME OVER", 80, 95, 4);
    s.setTextColor(s.color24to16(Theme::TEXT), s.color24to16(Theme::BG));
    s.drawString("TAP TO EXIT", 100, 130, 2);
  }
  s.pushSprite(0, 0);
}

void PacMan::end() { _sprite.deleteSprite(); }
#endif
```

- [ ] **Step 3: Register, compile, flash, verify**

In `paper-arcade.ino` uncomment PacMan include + addGame.

```bash
arduino-cli compile --fqbn esp32:esp32:esp32 .
arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/ttyUSB0 .
```

Verify: PACMAN card. Tap top/bottom/left/right of screen for D-pad direction. Pac-Man eats dots. Power pellets turn ghosts blue (frightened). Ghosts chase.

- [ ] **Step 4: Commit**

```bash
git add src/games/PacMan.h src/games/PacMan.cpp paper-arcade.ino
git commit -m "feat: Pac-Man — simplified maze, 3 ghosts with chase/frightened AI"
```

---

## Self-Review Notes

- All 10 games + core engine + OTA covered.
- Native logic tests for Snake, Pong, Simon Says, Minesweeper, 2048, Tetris (rendering-heavy games verified manually).
- File-per-game keeps each game's responsibility isolated; adding game #11 only requires creating one new file pair and registering it in `paper-arcade.ino`.
- All control schemes from the spec are implemented (full-screen swipe, single tap, split tap, tap zone strips, grid tap).
- High scores persist via NVS in every game.
- OTA boot-hold integrated in main loop.
- BMP asset loading scaffolded — games currently use programmatic graphics; SD assets can be added later by simply calling `assets.loadBitmap()` in `begin()`.

After all 16 tasks complete, run a full smoke test: power up, browse all 10 games via swipe, play each game once, return to launcher, power-cycle, confirm high scores remain.

---

## Execution Handoff

Plan complete and saved to `docs/superpowers/plans/2026-04-29-paper-arcade.md`. Two execution options:

**1. Subagent-Driven (recommended)** — I dispatch a fresh subagent per task, review between tasks, fast iteration with isolation.

**2. Inline Execution** — execute tasks in this session using executing-plans, batch execution with checkpoints for review.

Which approach?

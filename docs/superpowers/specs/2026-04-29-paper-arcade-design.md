# Paper Arcade — Design Spec
**Date:** 2026-04-29  
**Status:** Approved

---

## Overview

A handheld retro arcade device built on the ESP32 CYD (ESP32-2432S028). Ten classic games selectable from a swipe carousel launcher, rendered on the built-in 2.8" 320×240 TFT display via resistive touch input. Assets loaded from a 512MB microSD card. High scores persisted to NVS. OTA firmware updates over WiFi.

---

## Hardware

| Component | Spec |
|-----------|------|
| MCU | ESP32 (dual-core 240MHz, 520KB RAM, 4MB Flash) |
| Display | ILI9341 TFT, 2.8", 320×240, SPI |
| Touch | XPT2046 resistive touchscreen, SPI |
| Storage (assets) | microSD card, 512MB, SPI (built-in TF slot) |
| Storage (scores) | NVS (internal flash, via Preferences library) |
| Connectivity | WiFi (OTA only) |
| Power | USB-C |

---

## Development Stack

- **Framework:** Arduino (ESP32 Arduino core)
- **Display driver:** TFT_eSPI (with `User_Setup.h` configured for CYD pinout)
- **Rendering:** `TFT_eSprite` double-buffering for tear-free 30fps
- **Touch driver:** XPT2046_Touchscreen
- **SD:** Arduino SD library
- **Persistence:** Arduino Preferences (NVS key-value store)
- **OTA:** ArduinoOTA (triggered by holding screen on boot)
- **Language:** C++17

---

## Visual Theme

**Synthwave** — deep navy/purple backgrounds (`#0d0221`), hot pink (`#f72585`) and violet (`#7209b7`) accents, white text. Applied to the launcher, menus, game HUDs, and score screens. Each game's gameplay area follows its own classic color palette (e.g. green snake, colorful Tetris blocks).

Theme constants live in `src/ui/Theme.h`:
```cpp
namespace Theme {
  const uint32_t BG        = 0x0d0221;
  const uint32_t ACCENT    = 0xf72585;
  const uint32_t SECONDARY = 0x7209b7;
  const uint32_t TEXT      = 0xffffff;
  const uint32_t DIM       = 0x4cc9f0;
}
```

---

## Architecture

Four layers:

```
[ Hardware: CYD board, ILI9341, XPT2046, microSD, NVS ]
          ↕
[ Drivers: TFT_eSPI, XPT2046_Touchscreen, SD, Preferences, ArduinoOTA ]
          ↕
[ Core Engine: Game interface, Launcher, InputManager, AssetManager, ScoreManager ]
          ↕
[ Games: Snake, Pong, Breakout, Tetris, FlappyBird, SpaceInvaders,
         PacMan, Minesweeper, Game2048, SimonSays ]
```

---

## Project Structure

```
paper-arcade/
├── paper-arcade.ino          # Arduino entry — setup() and loop()
├── User_Setup.h              # TFT_eSPI CYD pin configuration
│
├── src/
│   ├── core/
│   │   ├── Game.h            # Abstract base class
│   │   ├── Launcher.h/cpp    # Swipe carousel + game router
│   │   ├── InputManager.h/cpp
│   │   ├── AssetManager.h/cpp
│   │   └── ScoreManager.h/cpp
│   │
│   ├── games/
│   │   ├── Snake.h/cpp
│   │   ├── Pong.h/cpp
│   │   ├── Breakout.h/cpp
│   │   ├── Tetris.h/cpp
│   │   ├── FlappyBird.h/cpp
│   │   ├── SpaceInvaders.h/cpp
│   │   ├── PacMan.h/cpp
│   │   ├── Minesweeper.h/cpp
│   │   ├── Game2048.h/cpp
│   │   └── SimonSays.h/cpp
│   │
│   └── ui/
│       └── Theme.h
│
└── data/                     # Copied to SD card root
    ├── sprites/              # Per-game BMP sprite sheets
    ├── fonts/                # Bitmap font files
    └── scores.json           # SD-side score backup
```

---

## Game Interface

Every game implements this contract:

```cpp
class Game {
public:
  virtual void     begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores) = 0;
  virtual void     update(InputEvent input) = 0;
  virtual void     draw() = 0;
  virtual void     end() = 0;
  virtual const char* name()      = 0;
  virtual uint32_t    highScore() = 0;
  virtual bool        isDone()    = 0;  // true = return to launcher
};
```

The `Launcher` holds `Game* games[10]` and calls `update()` + `draw()` each frame on the active game. When `isDone()` returns true, it calls `end()` and re-enters the carousel.

---

## Launcher (Swipe Carousel)

- Displays one game card at a time: large icon (from SD), game name, high score
- Swipe left/right to browse all 10 games
- Dot indicator shows position (10 dots, active one highlighted in pink)
- Tap the card to launch the game
- Synthwave theme: navy background, pink border on active card, violet inactive dots

---

## Main Loop

```
setup():
  init TFT_eSPI → fill black
  init XPT2046
  init SD card
  init Preferences (NVS)
  init WiFi + ArduinoOTA (if boot-hold detected)
  Launcher.begin()

loop():
  ArduinoOTA.handle()           // non-blocking, ~0.1ms
  input = InputManager.read()   // poll touch, emit InputEvent
  activeGame.update(input)      // game logic tick
  activeGame.draw()             // render to sprite, push to TFT
  if (activeGame.isDone())
    Launcher.returnToMenu()     // call end(), re-enter carousel
```

**Target frame rate:** 30fps (33ms per frame).

---

## Input System

`InputManager` translates raw XPT2046 coordinates into typed events:

```cpp
enum InputType { TAP, SWIPE_LEFT, SWIPE_RIGHT, SWIPE_UP, SWIPE_DOWN, DRAG };

struct InputEvent {
  InputType type;
  uint16_t  x, y;   // position in game coordinates
  int16_t   dx, dy; // delta for swipes
};
```

Games never access the touchscreen directly — they only receive `InputEvent` structs.

### Control schemes per game

| Scheme | Games | Description |
|--------|-------|-------------|
| Full-screen swipe | Snake, 2048 | Swipe direction = move direction |
| Single tap anywhere | Flappy Bird | Any tap = flap |
| Split left/right tap | Pong, Breakout | Left half / right half of screen |
| Tap zone strips | Tetris, Space Invaders | Side strips move, bottom bar drops/fires, corner rotates |
| Grid tap | Minesweeper, Simon Says, Pac-Man | InputManager maps XY → (col, row) grid cell |

---

## Asset Pipeline

```
data/sprites/<game>/<asset>.bmp   → AssetManager.loadBitmap("game/asset")
data/fonts/<name>.bin             → AssetManager.loadFont("name")
```

- `AssetManager` maintains a small LRU cache (4 sprite slots)
- Each game calls `assets.load*()` in `begin()` and `assets.freeAll()` in `end()`
- Only the active game's assets are in RAM at any time
- BMP format: 16-bit RGB565, no compression (fastest to decode on ESP32)

---

## Persistence

**High scores** stored per game in NVS via Arduino `Preferences`:
- Namespace: `"arcade"`
- Key: game name (e.g. `"snake"`, `"tetris"`)
- Written on game over if new high score
- Read by `Launcher` to display in carousel cards

**SD backup:** `data/scores.json` written on each score update as a human-readable fallback.

---

## OTA Updates

- On boot: if screen is held for 3 seconds, device enters OTA mode
- Display shows: device IP address + "OTA Ready" message
- Developer uploads via `arduino-cli upload --port <IP> --fqbn esp32:esp32:esp32 paper-arcade.ino`
- After flash: device reboots normally
- OTA mode also accepts uploads via Arduino IDE (mDNS discovery)

---

## Games — Flash & Control Summary

| Game | Est. Flash | Control Scheme |
|------|-----------|----------------|
| Snake | ~20KB | Full-screen swipe |
| Pong | ~25KB | Split left/right tap |
| Breakout | ~35KB | Split left/right tap (drag) |
| Tetris | ~50KB | Tap zone strips |
| Flappy Bird | ~30KB | Single tap anywhere |
| Space Invaders | ~60KB | Tap zone strips |
| Pac-Man | ~100KB | Grid tap (D-pad zones) |
| Minesweeper | ~30KB | Grid tap |
| 2048 | ~25KB | Full-screen swipe |
| Simon Says | ~20KB | Grid tap (4 color zones) |
| **Total** | **~395KB** | well within 4MB Flash |

---

## Out of Scope

- Multiplayer / Bluetooth
- Sound (no speaker on base CYD — can be added later via I2S)
- Cloud leaderboards
- Game saves / continue (all games start fresh each session)

# Paper Arcade — UI/UX Revamp Design Spec
**Date:** 2026-04-29
**Status:** Approved

---

## Overview

A full redesign of the Paper Arcade device experience — homepage, in-game chrome, pause/game-over screens, and color system. The target feeling is **premium handheld console** (Game Boy Advance / Analogue Pocket aesthetic): clean, intentional, polished. Transitions are instant (no animation). Text is the primary visual element.

---

## Design Principles

1. **Text as visual** — game names rendered large and bold in their own accent colour are the art. No icons, no illustrations.
2. **OLED black** — pure `#000000` backgrounds. Everything else floats on darkness.
3. **One shared chrome** — all 10 games use identical HUD strip, pause overlay, and game-over overlay. Only the accent colour and score format differ.
4. **Instant cuts** — no slide animations, no fades. Snappy and responsive on constrained hardware.
5. **Constraint as aesthetic** — the limited TFT_eSPI font set and direct-render approach are celebrated, not hidden.

---

## Color System

Replace the existing Synthwave palette in `src/ui/Theme.h` entirely.

> **Note:** RGB565 values are computed with `((r>>3)<<11) | ((g>>2)<<5) | (b>>3)`. The hex values below are canonical; implementer should recompute RGB565 in `Theme.h` using that formula rather than copying from this table.

| Role | Name | Hex | RGB565 (approx) | Usage |
|------|------|-----|--------|-------|
| Background | `BG` | `#000000` | `0x0000` | All screen backgrounds |
| Primary text | `TEXT` | `#FFFFFF` | `0xFFFF` | Game names on game-over, scores |
| Accent / CTA | `ACCENT` | `#0A84FF` | `0x0C3F` | RESUME button, header divider |
| Muted | `MUTED` | `#8E8E93` | `0x8C72` | Labels, hints, unplayed scores ("--") |
| Card surface | `CARD` | `#1C1C1E` | `0x18E3` | Pause modal background |
| Separator | `SEP` | `#2C2C2E` | `0x2965` | Row dividers, subtle borders |
| Danger | `DANGER` | `#FF453A` | `0xFA27` | QUIT button |

### Per-game accent colours

Each game has one bold colour used for: its name in the launcher, its HUD strip underline, and its game-over title.

| Game | Colour | Hex | RGB565 |
|------|--------|-----|--------|
| Snake | Green | `#30D158` | `0x3228` |
| Pong | Blue | `#0A84FF` | `0x0433` |
| Simon Says | Red | `#FF453A` | `0xFA27` |
| Minesweeper | Amber | `#FF9F0A` | `0xFCC1` |
| 2048 | Rose | `#FF375F` | `0xF9AB` |
| Breakout | Indigo | `#5E5CE6` | `0x5AEC` |
| Flappy Bird | Yellow | `#FFD60A` | `0xFEA1` |
| Tetris | Sky | `#64D2FF` | `0x669F` |
| Space Invaders | Salmon | `#FF6961` | `0xFB4C` |
| Pac-Man | Purple | `#BF5AF2` | `0xBAAE` |

---

## Homepage / Launcher

**File:** `src/core/Launcher.cpp`

### Layout (240×320 portrait, no scrolling)

```
y=0..23     Header strip (24px)
y=24        Blue divider line (1px)
y=25..304   10 game rows × 28px each = 280px
y=305..319  Footer hint (15px)
```

Total: 24 + 1 + 280 + 15 = **320px** — exact fit.

### Header (24px)

```
┌──────────────────────────────────┐
│ PAPER ARCADE          10 GAMES   │  ← white left, muted right, font 2
├──────────────────────────────────┤  ← 1px ACCENT blue line
```

- `PAPER ARCADE`: white, font size 2, left-aligned, x=8, y=6
- `10 GAMES`: muted, font size 1, right-aligned, x=232, y=8
- Blue divider: `fillRect(0, 22, 240, 1, ACCENT565)`

### Game rows (28px each, y = 25 + row × 28)

Each row is a full-width tap target (240px × 28px):

```
│ SNAKE ────────────────── 250 │
│ green, font 4, x=8, y+6  │grey, font 2, right-aligned, x=232 │
```

- Game name: font size 4, **game accent colour**, x=8, vertically centred in row
- Score: font size 2, `MUTED` if zero (displays `--`), `TEXT` white if > 0, right-aligned x=232
- Row separator: `drawFastHLine(0, rowY + 27, 240, SEP)`
- Tap: full row bounding box — `(0 ≤ x ≤ 239) && (rowY ≤ y ≤ rowY+27)`

### Footer (15px)

```
│         HOLD 1.5s — PAUSE / QUIT         │
```

- Text: muted, font size 1, centred, y=308

---

## In-Game Chrome

Applied to every game. Games themselves handle their own play-area rendering. The chrome wraps around them.

### HUD Strip (22px at top)

```
┌──────────────────────────────────┐
│ SNAKE                       250  │  ← name in game colour, score in white
├──────────────────────────────────┤  ← 1px game-accent coloured line
│                                  │
│         [game plays here]        │
│                                  │
└──────────────────────────────────┘
```

- Game name: font size 2, game accent colour, x=6, y=5
- Score: font size 2, white, right-aligned x=234, y=5
- Lives (where applicable — Breakout, Space Invaders): dots at bottom of game area
  - Filled dot = `game accent colour`, empty dot = `SEP`, dot radius = 4px, y=310
- Accent underline: `drawFastHLine(0, 20, 240, gameAccentColour565)`
- **Game play area:** y=22 to y=319 (298px height for gameplay)

**Implementation note — compositing order:**
1. `_active->draw()` fills the full 240×320 canvas (including y=0..21). In normal play this is game content; on game-over it is the full-screen overlay.
2. `Launcher::draw()` then draws the HUD strip (y=0..21) on top, overwriting whatever the game drew there.

This means games never need to reserve space for the HUD — they paint freely and the Launcher composites the chrome. Games should not place essential text in the top 22px during normal play, but game-over content below y=22 is always fully visible.

### Pause Overlay (long-press 1.5s → LONG_PRESS event)

Drawn on top of the current game frame:

```
┌─────────────────────────────────┐
│     [dimmed game frame]         │
│                                 │
│      ┌─────────────────┐       │
│      │    P A U S E D  │ ← muted grey, size 1, letter-spaced
│      │  ┌───────────┐  │       │
│      │  │  RESUME   │  │ ← ACCENT blue bg, white text, size 2
│      │  └───────────┘  │       │
│      │  ┌───────────┐  │       │
│      │  │   QUIT    │  │ ← transparent bg, DANGER red text
│      │  └───────────┘  │       │
│      └─────────────────┘       │
└─────────────────────────────────┘
```

- Semi-dim: `fillRect(0, 22, 240, 298, BLACK)` at 50% via alternating pixel pattern, or just solid `CARD` color
- Modal card: `fillRoundRect(30, 100, 180, 110, 8, CARD)` with `drawRoundRect` border in `ACCENT` at 30% opacity
- "PAUSED": muted, font 1, letter-spacing 3, centred, y=114
- RESUME button: `fillRoundRect(46, 130, 148, 32, 5, ACCENT)` — white "RESUME" font 2 centred
- QUIT button: no fill, `drawRoundRect(46, 170, 148, 32, 5, SEP)` — DANGER red "QUIT TO MENU" font 2 centred
- Tap RESUME area: y∈[130,162] → resume game
- Tap QUIT area: y∈[170,202] → `returnToMenu()`

### Game Over Overlay

Replaces the game's own play area when `isDone()` triggers. Drawn by each game's `draw()` when `_done == true`:

```
┌─────────────────────────────────┐
│ SNAKE                       250 │  ← HUD strip (unchanged)
├─────────────────────────────────┤
│                                 │
│          S N A K E              │ ← game name, game accent, size 6, centred
│                                 │
│         GAME OVER               │ ← white, size 4, centred
│                                 │
│        YOUR SCORE               │ ← muted, size 1, centred
│           250                   │ ← white, size 6, centred
│                                 │
│       TAP ANYWHERE              │ ← muted, size 1, centred, y=290
└─────────────────────────────────┘
```

- Background: pure black
- Game name: game accent colour, font 6, centred x=120, y=80
- "GAME OVER" / "YOU WIN!": white, font 4, centred, y=145
- "YOUR SCORE": muted, font 1, centred, y=195
- Score value: white, font 6, centred, y=210
- "TAP ANYWHERE": muted, font 1, centred, y=290

Games that have a "win" condition (Minesweeper win, Pac-Man all dots, Breakout all bricks) show `YOU WIN!` in green (`#30D158`) instead of white.

---

## Navigation Flow

```
Boot → Splash → Launcher list
Launcher → tap row → game begins (HUD strip visible immediately)
In-game → hold 1.5s → PAUSED overlay → tap RESUME or QUIT
In-game → game ends → GAME OVER overlay → tap anywhere → Launcher
```

No transitions. Every state change is an instant cut.

**Splash screen** (shown for 800ms on boot):

```
┌─────────────────────────────────┐
│                                 │
│                                 │
│        PAPER ARCADE             │ ← white, size 4, centred
│                                 │
│           ────                  │ ← short blue underline (30px), centred
│                                 │
└─────────────────────────────────┘
```

---

## Implementation Scope

### Files to modify

| File | Change |
|------|--------|
| `src/ui/Theme.h` | Replace all colour constants with new OLED palette |
| `src/core/Launcher.cpp` | Full rewrite: vertical list homepage, HUD strip overlay, updated pause modal |
| `src/core/Launcher.h` | Add `uint16_t gameAccentColor(int idx)` helper, remove `_current` (no carousel) |
| `paper-arcade.ino` | Update splash screen text positioning |
| `src/games/*.cpp` | Remove each game's own HUD/score/game-over rendering; games render play area only (y=22..319); add `_gameColour` constant |

### Per-game changes

Each game's `draw()` method is simplified to:
1. Render the play area filling y=22..319 (298px tall)
2. When `_done == true`: render the game-over overlay (full-screen, including the y=0..21 zone)

The HUD strip (game name + score + underline) is drawn by `Launcher::draw()` after `_active->draw()`, so it always appears on top.

### New `Launcher` public interface

```cpp
// Returns the game accent colour (RGB565) for a given game index
uint16_t Launcher::gameAccentColor(int idx) const;
```

---

## Out of Scope

- Sound / haptic feedback (no speaker on base CYD)
- Animated transitions (hardware too constrained)
- High score leaderboards or cloud sync
- Settings screen or brightness control

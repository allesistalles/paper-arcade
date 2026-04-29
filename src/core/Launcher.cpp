#include "Launcher.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"

// Portrait 240×320 grid layout:
//   Header:  y=0..44   — title
//   Grid:    y=46..320 — 2 cols × 4 rows, cell 120×68px
//   Pause overlay appears on LONG_PRESS while in-game

static const uint16_t GAME_COLORS[] = {
  0x07E0,  // Snake    — green
  0x4E5E,  // Pong     — cyan (DIM)
  0xFFFF,  // Simon    — white
  0xF800,  // Mines    — red
  0xFC60,  // 2048     — orange
  0xF94B,  // Breakout — pink (ACCENT)
  0xFFE0,  // Flappy   — yellow
  0x07FF,  // slot 8   — aqua
  0x780F,  // slot 9   — purple
  0xFE40,  // slot 10  — gold
};

void Launcher::begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores) {
  _tft = &tft; _assets = &assets; _scores = &scores;
  _needsRedraw = true;
  _paused = false;
}

void Launcher::addGame(Game* g) {
  if (_count < MAX_GAMES) _games[_count++] = g;
}

Game* Launcher::update(const InputEvent& evt) {
  // Must return _active on every in-game frame — launch-tap suppression
  // in paper-arcade.ino depends on this consistent non-null return.
  if (_inGame) {
    if (evt.type == InputEvent::LONG_PRESS) {
      // Show pause overlay
      _paused = true;
      _needsRedraw = true;
      return _active;
    }
    if (_paused) {
      if (evt.type == InputEvent::TAP) {
        if (evt.y > 160 && evt.y < 220) {
          // Bottom half of overlay = QUIT
          returnToMenu();
          return nullptr;
        } else {
          // Top half = RESUME
          _paused = false;
          _needsRedraw = false;
        }
      }
      return _active;
    }
    return _active;
  }
  if (_count == 0) return nullptr;

  if (evt.type == InputEvent::TAP) {
    // Map tap to grid cell
    int col = evt.x / 120;
    int row = (int)(evt.y - 46) / 68;
    if (col >= 0 && col < 2 && row >= 0 && row < 4) {
      int idx = row * 2 + col;
      if (idx < _count) {
        _active = _games[idx];
        _inGame = true;
        _paused = false;
        _active->begin(*_tft, *_assets, *_scores);
        return _active;
      }
    }
  }
  return nullptr;
}

void Launcher::draw() {
  if (_inGame) {
    if (_active) _active->draw();
    if (_paused) drawPauseOverlay();
    return;
  }
  if (!_needsRedraw) return;
  _needsRedraw = false;

  TFT_eSPI& t = *_tft;
  uint16_t bg  = t.color24to16(Theme::BG);
  uint16_t acc = t.color24to16(Theme::ACCENT);
  uint16_t sec = t.color24to16(Theme::SECONDARY);
  uint16_t drk = t.color24to16(Theme::DARK);
  uint16_t txt = t.color24to16(Theme::TEXT);
  uint16_t dim = t.color24to16(Theme::DIM);

  t.fillScreen(bg);

  // ── Header ─────────────────────────────────────────────────────
  t.fillRect(0, 0, 240, 44, drk);
  // "PAPER" left, "ARCADE" right — both in their accent colours
  t.setTextColor(acc, drk);
  t.drawString("PAPER", 8, 8, 4);
  t.setTextColor(sec, drk);
  int aw = t.textWidth("ARCADE", 4);
  t.drawString("ARCADE", 236 - aw, 8, 4);
  // Neon divider line
  t.drawFastHLine(0, 43, 240, acc);
  t.drawFastHLine(0, 44, 240, sec);

  // ── Game grid ──────────────────────────────────────────────────
  for (int i = 0; i < 8; i++) {
    int col = i % 2;
    int row = i / 2;
    int x   = col * 120;
    int y   = 46 + row * 68;

    if (i < _count) {
      Game* g = _games[i];
      uint16_t gc = GAME_COLORS[i % 10];

      // Card background
      t.fillRect(x + 2, y + 2, 116, 64, drk);
      // Coloured top accent bar
      t.fillRect(x + 2, y + 2, 116, 5, gc);
      // Card border
      t.drawRect(x + 1, y + 1, 118, 66, gc);

      // Game name
      t.setTextColor(gc, drk);
      int nw = t.textWidth(g->name(), 2);
      t.drawString(g->name(), x + 60 - nw / 2, y + 14, 2);

      // High score
      char buf[20];
      snprintf(buf, sizeof(buf), "HI:%lu", (unsigned long)g->highScore());
      t.setTextColor(dim, drk);
      int hw = t.textWidth(buf, 2);
      t.drawString(buf, x + 60 - hw / 2, y + 38, 2);

      // "TAP" hint if score is 0
      if (g->highScore() == 0) {
        t.setTextColor(t.color24to16(0x2d2050), drk);
        int tw = t.textWidth("TAP", 1);
        t.drawString("TAP", x + 60 - tw / 2, y + 56, 1);
      }
    } else {
      // Empty slot — subtle dashed border
      t.drawRect(x + 1, y + 1, 118, 66, t.color24to16(0x1a1030));
      t.setTextColor(t.color24to16(0x1a1030), bg);
      t.drawString("+", x + 54, y + 22, 4);
    }
  }

  // Bottom hint
  t.setTextColor(t.color24to16(0x2d2050), bg);
  int hintW = t.textWidth("hold = pause", 1);
  t.drawString("hold = pause", 120 - hintW / 2, 314, 1);
}

void Launcher::drawPauseOverlay() {
  TFT_eSPI& t = *_tft;
  uint16_t acc = t.color24to16(Theme::ACCENT);
  uint16_t bg  = t.color24to16(Theme::BG);
  uint16_t drk = t.color24to16(Theme::DARK);
  uint16_t txt = t.color24to16(Theme::TEXT);
  uint16_t dim = t.color24to16(Theme::DIM);

  // Semi-transparent overlay: just a dark rounded rect in the centre
  t.fillRoundRect(20, 100, 200, 140, 10, drk);
  t.drawRoundRect(20, 100, 200, 140, 10, acc);

  t.setTextColor(acc, drk);
  int pw = t.textWidth("PAUSED", 4);
  t.drawString("PAUSED", 120 - pw / 2, 112, 4);

  // Resume button (top half of box)
  t.fillRoundRect(35, 148, 170, 36, 6, t.color24to16(Theme::SECONDARY));
  t.setTextColor(txt, t.color24to16(Theme::SECONDARY));
  int rw = t.textWidth("RESUME", 2);
  t.drawString("RESUME", 120 - rw / 2, 160, 2);

  // Quit button (bottom half)
  t.fillRoundRect(35, 192, 170, 36, 6, t.color24to16(0x500010));
  t.setTextColor(acc, t.color24to16(0x500010));
  int qw = t.textWidth("QUIT TO MENU", 2);
  t.drawString("QUIT TO MENU", 120 - qw / 2, 204, 2);
}

void Launcher::drawCard()   {}   // unused — grid replaces per-card draw
void Launcher::drawArrows() {}
void Launcher::drawDots()   {}

void Launcher::returnToMenu() {
  if (_active) { _active->end(); _active = nullptr; }
  _inGame = false;
  _paused = false;
  _needsRedraw = true;
}

#endif

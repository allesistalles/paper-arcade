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
    // HUD strip tap (y < HUD_H) → toggle pause
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

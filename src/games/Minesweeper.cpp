#include "Minesweeper.h"
#include <cstring>

uint32_t MinesLogic::nextRand() {
  _rng ^= _rng << 13; _rng ^= _rng >> 17; _rng ^= _rng << 5;
  return _rng;
}

void MinesLogic::init(uint8_t w, uint8_t h, uint8_t mines, uint32_t seed) {
  _w = w; _h = h; _mines = mines;
  _rng = seed ? seed : 1;
  _exploded = false; _revealed_count = 0;
  std::memset(_mine, 0, sizeof(_mine));
  std::memset(_revealed, 0, sizeof(_revealed));
  std::memset(_flag, 0, sizeof(_flag));
  placeMines(mines);
}

void MinesLogic::placeMines(uint8_t count) {
  for (uint8_t placed = 0; placed < count; ) {
    int x = (int)(nextRand() % _w);
    int y = (int)(nextRand() % _h);
    if (!_mine[idx(x, y)]) { _mine[idx(x, y)] = true; placed++; }
  }
}

bool MinesLogic::isMine(uint8_t x, uint8_t y)     const { return _mine[idx(x, y)]; }
bool MinesLogic::isRevealed(uint8_t x, uint8_t y) const { return _revealed[idx(x, y)]; }
bool MinesLogic::isFlagged(uint8_t x, uint8_t y)  const { return _flag[idx(x, y)]; }

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
  _revealed[idx(x, y)] = true; _revealed_count++;
  if (neighborCount(x, y) == 0)
    for (int dy = -1; dy <= 1; dy++)
      for (int dx = -1; dx <= 1; dx++)
        if (dx || dy) floodFill(x + dx, y + dy);
}

bool MinesLogic::reveal(uint8_t x, uint8_t y) {
  if (_exploded || _flag[idx(x, y)] || _revealed[idx(x, y)]) return true;
  if (_mine[idx(x, y)]) { _exploded = true; _revealed[idx(x, y)] = true; return false; }
  floodFill(x, y);
  return true;
}

void MinesLogic::toggleFlag(uint8_t x, uint8_t y) {
  if (_revealed[idx(x, y)]) return;
  _flag[idx(x, y)] = !_flag[idx(x, y)];
}

bool MinesLogic::isWon() const {
  return !_exploded && (_revealed_count == _w * _h - _mines);
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include "../ui/GameOver.h"

// NUM_COLS used inside draw() — defined there as static local

void Minesweeper::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("mines");
  _done = false; _dirty = true;
  _logic.init(W, H, 10, millis());
  _lastTapX = 255;
}

void Minesweeper::update(const InputEvent& input) {
  if (_done) return;
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
  if (input.type != InputEvent::TAP) return;
  if (input.y < 22 || input.y >= 22 + H * CELL) return;
  uint8_t cx = (uint8_t)(input.x / CELL);
  uint8_t cy = (uint8_t)((input.y - 22) / CELL);
  if (cx >= W || cy >= H) return;

  uint32_t now = millis();
  bool dbl = (cx == _lastTapX && cy == _lastTapY && now - _lastTapMs < 400);
  if (dbl) { _logic.toggleFlag(cx, cy); _lastTapX = 255; }
  else { _lastTapMs = now; _lastTapX = cx; _lastTapY = cy; _logic.reveal(cx, cy); }
  _dirty = true;
}

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

  s.fillRect(0, 22, 240, 298, Theme::BG565);

  static const uint16_t NUM_COLS[9] = {
    0x0000, Theme::ACCENT565, 0x07E0, Theme::DANGER565,
    0x780F, 0x7980, Theme::MUTED565, 0x0000, 0x7BEF
  };

  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      int px = x * CELL;
      int py = y * CELL + 22;  // shifted +22 below HUD
      if (_logic.isRevealed(x, y)) {
        s.fillRect(px + 1, py + 1, CELL - 2, CELL - 2, Theme::SEP565);
        if (_logic.isMine(x, y)) {
          s.fillCircle(px + CELL / 2, py + CELL / 2, 7, Theme::DANGER565);
        } else {
          uint8_t n = _logic.neighborCount(x, y);
          if (n > 0) {
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

void Minesweeper::end() {}
#endif

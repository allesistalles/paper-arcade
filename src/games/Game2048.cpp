#include "Game2048.h"
#include <cstring>

uint32_t Game2048Logic::nextRand() {
  _rng ^= _rng << 13; _rng ^= _rng >> 17; _rng ^= _rng << 5;
  return _rng;
}

void Game2048Logic::initEmpty() {
  std::memset(_grid, 0, sizeof(_grid)); _score = 0; _rng = 1;
}

void Game2048Logic::initWithSeed(uint32_t seed) {
  initEmpty(); _rng = seed ? seed : 1;
  spawnTile(); spawnTile();
}

void Game2048Logic::setCell(int x, int y, uint16_t v) { _grid[y * 4 + x] = v; }

void Game2048Logic::spawnTile() {
  int empty[16], n = 0;
  for (int i = 0; i < 16; i++) if (_grid[i] == 0) empty[n++] = i;
  if (!n) return;
  _grid[empty[nextRand() % n]] = (nextRand() % 10 == 0) ? 4 : 2;
}

bool Game2048Logic::slideRow(uint16_t* row) {
  uint16_t out[4] = {0}; int wi = 0; bool moved = false;
  uint16_t pending = 0;
  for (int i = 0; i < 4; i++) {
    if (!row[i]) continue;
    if (pending == row[i]) { out[wi++] = pending * 2; _score += pending * 2; pending = 0; }
    else { if (pending) out[wi++] = pending; pending = row[i]; }
  }
  if (pending) out[wi++] = pending;
  for (int i = 0; i < 4; i++) { if (out[i] != row[i]) moved = true; row[i] = out[i]; }
  return moved;
}

void Game2048Logic::reverseRow(uint16_t* row) {
  uint16_t t;
  t = row[0]; row[0] = row[3]; row[3] = t;
  t = row[1]; row[1] = row[2]; row[2] = t;
}

void Game2048Logic::copyCol(int x, uint16_t* out)     { for (int y=0;y<4;y++) out[y]=_grid[y*4+x]; }
void Game2048Logic::writeCol(int x, const uint16_t* in){ for (int y=0;y<4;y++) _grid[y*4+x]=in[y]; }

bool Game2048Logic::slide(Dir d, bool spawn) {
  bool moved = false;
  if (d == LEFT) { for (int y=0;y<4;y++) moved |= slideRow(&_grid[y*4]); }
  else if (d == RIGHT) {
    for (int y=0;y<4;y++) { uint16_t* r=&_grid[y*4]; reverseRow(r); moved|=slideRow(r); reverseRow(r); }
  } else if (d == UP) {
    for (int x=0;x<4;x++) { uint16_t c[4]; copyCol(x,c); bool m=slideRow(c); writeCol(x,c); moved|=m; }
  } else {
    for (int x=0;x<4;x++) { uint16_t c[4]; copyCol(x,c); reverseRow(c); bool m=slideRow(c); reverseRow(c); writeCol(x,c); moved|=m; }
  }
  if (moved && spawn) spawnTile();
  return moved;
}

bool Game2048Logic::hasMoves() const {
  for (int i=0;i<16;i++) if (!_grid[i]) return true;
  for (int y=0;y<4;y++) for (int x=0;x<3;x++) if (_grid[y*4+x]==_grid[y*4+x+1]) return true;
  for (int y=0;y<3;y++) for (int x=0;x<4;x++) if (_grid[y*4+x]==_grid[(y+1)*4+x]) return true;
  return false;
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include "../ui/GameOver.h"

static uint16_t tileColor(uint16_t v) {
  switch (v) {
    case 2:    return 0xCE59; case 4:    return 0xBE16;
    case 8:    return 0xFC00; case 16:   return 0xFB80;
    case 32:   return 0xF8C0; case 64:   return 0xF800;
    case 128:  return 0xEE85; case 256:  return 0xEE61;
    case 512:  return 0xE603; case 1024: return 0xE5E1;
    case 2048: return 0xE5C0; default:   return 0x4208;
  }
}

void Game2048::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("2048");
  _done = false; _gameOver = false; _dirty = true;
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
  Game2048Logic::Dir d; bool valid = false;
  switch (input.type) {
    case InputEvent::SWIPE_LEFT:  d = Game2048Logic::LEFT;  valid = true; break;
    case InputEvent::SWIPE_RIGHT: d = Game2048Logic::RIGHT; valid = true; break;
    case InputEvent::SWIPE_UP:    d = Game2048Logic::UP;    valid = true; break;
    case InputEvent::SWIPE_DOWN:  d = Game2048Logic::DOWN;  valid = true; break;
    default: break;
  }
  if (valid) { _logic.slide(d, true); if (!_logic.hasMoves()) _gameOver = true; _dirty = true; }
}

void Game2048::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_gameOver) {
    drawGameOverOverlay(s, "2048", _logic.score(), Theme::G2048565);
    return;
  }

  s.fillScreen(Theme::BG565);

  // Grid: CELL=54, GAP=3, OX=4, OY=62 (shifted from 40 to 62 = +22)
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

void Game2048::end() {}
#endif

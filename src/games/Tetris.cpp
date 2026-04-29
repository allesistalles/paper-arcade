#include "Tetris.h"

// Standard 7 tetrominoes, 4 rotations, 4×4 bitmask (bit i = cell (i%4, i/4))
static const uint16_t SHAPES[7][4] = {
  { 0x0F00, 0x2222, 0x00F0, 0x4444 },  // I
  { 0x0660, 0x0660, 0x0660, 0x0660 },  // O
  { 0x0E40, 0x4C40, 0x4E00, 0x4640 },  // T
  { 0x06C0, 0x4620, 0x06C0, 0x4620 },  // S
  { 0x0C60, 0x2640, 0x0C60, 0x2640 },  // Z
  { 0x44C0, 0x8E00, 0x6440, 0x0E20 },  // J
  { 0x4460, 0x0E80, 0xC440, 0x2E00 },  // L
};

uint32_t TetrisLogic::nextRand() {
  _rng ^= _rng << 13; _rng ^= _rng >> 17; _rng ^= _rng << 5;
  return _rng;
}

void TetrisLogic::init(uint32_t seed) {
  std::memset(_board, 0, sizeof(_board));
  _rng = seed ? seed : 1;
  _score = 0; _over = false;
  spawn();
}

void TetrisLogic::spawn() {
  _ptype = nextRand() % 7; _prot = 0; _px = 3; _py = 0;
  if (collides(_ptype, _prot, _px, _py)) _over = true;
}

bool TetrisLogic::collides(int type, int rot, int px, int py) const {
  uint16_t shape = SHAPES[type][rot];
  for (int b = 0; b < 16; b++) {
    if (!(shape & (1 << b))) continue;
    int x = px + (b % 4), y = py + (b / 4);
    if (x < 0 || x >= W || y >= H) return true;
    if (y < 0) continue;
    if (_board[y][x]) return true;
  }
  return false;
}

bool TetrisLogic::tryMove(int dx) {
  if (_over || !collides(_ptype, _prot, _px + dx, _py)) { _px += dx; return true; }
  return false;
}

bool TetrisLogic::tryRotate() {
  if (_over) return false;
  int next = (_prot + 1) % 4;
  if (!collides(_ptype, next, _px, _py)) { _prot = next; return true; }
  // Wall-kick: try offset ±1
  if (!collides(_ptype, next, _px + 1, _py)) { _prot = next; _px++; return true; }
  if (!collides(_ptype, next, _px - 1, _py)) { _prot = next; _px--; return true; }
  return false;
}

void TetrisLogic::lock() {
  uint16_t shape = SHAPES[_ptype][_prot];
  for (int b = 0; b < 16; b++) {
    if (!(shape & (1 << b))) continue;
    int x = _px + (b % 4), y = _py + (b / 4);
    if (y >= 0 && y < H) _board[y][x] = (uint8_t)(_ptype + 1);
  }
}

int TetrisLogic::clearLines() {
  int cleared = 0;
  for (int y = H - 1; y >= 0; ) {
    bool full = true;
    for (int x = 0; x < W; x++) if (!_board[y][x]) { full = false; break; }
    if (full) {
      for (int yy = y; yy > 0; yy--) std::memcpy(_board[yy], _board[yy - 1], W);
      std::memset(_board[0], 0, W);
      cleared++;
    } else { y--; }
  }
  static const uint32_t POINTS[5] = { 0, 100, 300, 500, 800 };
  if (cleared <= 4) _score += POINTS[cleared];
  return cleared;
}

bool TetrisLogic::tick() {
  if (_over) return false;
  if (collides(_ptype, _prot, _px, _py + 1)) {
    lock(); clearLines(); spawn(); return !_over;
  }
  _py++;
  return true;
}

void TetrisLogic::hardDrop() {
  if (_over) return;
  while (!collides(_ptype, _prot, _px, _py + 1)) _py++;
  lock(); clearLines(); spawn();
}

void TetrisLogic::pieceBlocks(int outX[4], int outY[4]) const {
  uint16_t shape = SHAPES[_ptype][_prot];
  int n = 0;
  for (int b = 0; b < 16 && n < 4; b++) {
    if (!(shape & (1 << b))) continue;
    outX[n] = _px + (b % 4); outY[n] = _py + (b / 4); n++;
  }
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"

// Classic Tetris colours per piece type
static const uint16_t PIECE_COLORS[8] = {
  0x0000, 0x07FF, 0xFFE0, 0xF81F, 0x07E0, 0xF800, 0x001F, 0xFD20
};

void Tetris::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("tetris");
  _done = false; _dirty = true;
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

  if (input.type == InputEvent::TAP) {
    if (input.x < OFFX) {
      if (input.y < OFFY + 60) _logic.tryRotate();
      else                     _logic.tryMove(-1);
      _dirty = true;
    } else if (input.x > OFFX + TetrisLogic::W * CELL) {
      _logic.tryMove(1);
      _dirty = true;
    } else if (input.y > 255) {
      _logic.hardDrop();
      _dirty = true;
    } else {
      // Tap on board → rotate
      _logic.tryRotate();
      _dirty = true;
    }
  }

  uint32_t now = millis();
  uint32_t fallMs = 600 - (_logic.score() / 500) * 50;
  if (fallMs < 80) fallMs = 80;
  if (now - _lastFall >= fallMs) {
    _lastFall = now;
    _logic.tick();
    _dirty = true;
  }
}

void Tetris::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;
  uint16_t bg  = s.color24to16(Theme::BG);
  uint16_t acc = s.color24to16(Theme::ACCENT);
  uint16_t drk = s.color24to16(Theme::DARK);
  uint16_t dim = s.color24to16(Theme::DIM);
  uint16_t txt = s.color24to16(Theme::TEXT);
  uint16_t sec = s.color24to16(Theme::SECONDARY);

  s.fillScreen(bg);

  // Board border
  s.drawRect(OFFX - 1, OFFY - 1, TetrisLogic::W * CELL + 2, TetrisLogic::H * CELL + 2, acc);

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

  // Ghost piece (drop preview)
  int ghostY = _logic.activePieceY();
  // find lowest valid Y
  while (true) {
    int ny = ghostY + 1;
    bool ok = true;
    uint16_t shape = SHAPES[_logic.activePieceType()][_logic.activePieceRot()];
    for (int b = 0; b < 16 && ok; b++) {
      if (!(shape & (1 << b))) continue;
      int gx = _logic.activePieceX() + (b % 4);
      int gy = ny + (b / 4);
      if (gx < 0 || gx >= TetrisLogic::W || gy >= TetrisLogic::H) ok = false;
      if (gy >= 0 && _logic.cell(gx, gy)) ok = false;
    }
    if (!ok) break;
    ghostY = ny;
  }
  if (ghostY != _logic.activePieceY()) {
    uint16_t shape = SHAPES[_logic.activePieceType()][_logic.activePieceRot()];
    for (int b = 0; b < 16; b++) {
      if (!(shape & (1 << b))) continue;
      int gx = _logic.activePieceX() + (b % 4);
      int gy = ghostY + (b / 4);
      if (gy < 0) continue;
      s.drawRect(OFFX + gx * CELL, OFFY + gy * CELL, CELL - 1, CELL - 1, drk);
    }
  }

  // Sidebar (x=180)
  s.setTextColor(dim, bg); s.drawString("SCORE", 178, 32, 2);
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)_logic.score());
  s.setTextColor(txt, bg); s.drawString(buf, 178, 50, 2);

  s.setTextColor(dim, bg); s.drawString("BEST", 178, 80, 2);
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)_hiScore);
  s.setTextColor(txt, bg); s.drawString(buf, 178, 98, 2);

  // Control zone hints
  s.setTextColor(sec, bg);
  s.drawString("ROT", 4, 35, 2);
  s.drawString("<", 10, 120, 4);
  s.drawString(">", 180, 120, 4);
  s.drawString("DROP", 88, 262, 2);

  if (_logic.gameOver()) {
    s.fillRect(OFFX, OFFY + 80, TetrisLogic::W * CELL, 60, bg);
    s.setTextColor(acc, bg);
    s.drawString("GAME", OFFX + 12, OFFY + 90, 4);
    s.drawString("OVER", OFFX + 12, OFFY + 120, 4);
    s.setTextColor(txt, bg);
    s.drawString("TAP", OFFX + 22, OFFY + 150, 2);
  }
}

void Tetris::end() {}
#endif

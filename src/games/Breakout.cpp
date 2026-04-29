#include "Breakout.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"

void Breakout::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("breakout");
  _done = false; _gameOver = false; _dirty = true;
  _score = 0; _lives = 3;
  _remaining = ROWS * COLS;
  for (int i = 0; i < ROWS * COLS; i++) _bricks[i] = true;
  _bx = 120; _by = 220; _vx = 1.2f; _vy = -1.8f;
  _paddleX = 92;
  _lastTick = millis();
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
    _paddleX = (int)input.x - PADDLE_W / 2;
    if (_paddleX < 0) _paddleX = 0;
    if (_paddleX + PADDLE_W > 240) _paddleX = 240 - PADDLE_W;
  }

  uint32_t now = millis();
  uint32_t dt = now - _lastTick;
  if (dt > 50) dt = 50;
  _lastTick = now;
  if (dt < 12) return;

  int steps = dt / 8;
  for (int step = 0; step < steps; step++) {
    _bx += _vx; _by += _vy;

    // Side walls
    if (_bx < 4)   { _bx = 4;   _vx = -_vx; }
    if (_bx > 236) { _bx = 236; _vx = -_vx; }
    // Top wall
    if (_by < 4)   { _by = 4;   _vy = -_vy; }

    // Paddle (y=295)
    if (_by >= 295 && _by <= 303 && _bx >= _paddleX && _bx <= _paddleX + PADDLE_W) {
      _vy = -fabsf(_vy);
      _vx = ((_bx - (_paddleX + PADDLE_W / 2.0f)) / (PADDLE_W / 2.0f)) * 2.0f;
      _by = 294;
    }

    // Bricks
    if (_by >= BRICK_TOP && _by < BRICK_TOP + ROWS * BRICK_H) {
      int row = (int)((_by - BRICK_TOP) / BRICK_H);
      int col = (int)(_bx / (BRICK_W + 2));
      if (col >= 0 && col < COLS && row >= 0 && row < ROWS) {
        int idx = row * COLS + col;
        if (_bricks[idx]) {
          _bricks[idx] = false; _remaining--;
          _score += 10; _vy = -_vy;
        }
      }
    }

    // Lost ball
    if (_by > 325) {
      _lives--;
      if (_lives == 0) _gameOver = true;
      else { _bx = 120; _by = 220; _vx = 1.2f; _vy = -1.8f; }
    }
    if (_remaining == 0) _gameOver = true;
  }
  _dirty = true;
}

void Breakout::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;
  uint16_t bg  = s.color24to16(Theme::BG);
  uint16_t acc = s.color24to16(Theme::ACCENT);
  uint16_t txt = s.color24to16(Theme::TEXT);

  s.fillScreen(bg);

  // Bricks
  for (int r = 0; r < ROWS; r++) {
    // Row colours cycling through the palette
    static const uint32_t ROW_COLS[6] = {
      0xf72585, 0x7209b7, 0x4cc9f0, 0x07e0a0, 0xffd700, 0xff6b35
    };
    uint16_t col = s.color24to16(ROW_COLS[r]);
    for (int c = 0; c < COLS; c++) {
      if (!_bricks[r * COLS + c]) continue;
      s.fillRect(c * (BRICK_W + 2), BRICK_TOP + r * BRICK_H, BRICK_W, BRICK_H - 2, col);
    }
  }

  // Paddle
  s.fillRoundRect(_paddleX, 295, PADDLE_W, PADDLE_H, 3, acc);

  // Ball
  s.fillCircle((int)_bx, (int)_by, 5, txt);

  // HUD
  char buf[32];
  s.setTextColor(txt, bg);
  snprintf(buf, sizeof(buf), "SCORE %lu", (unsigned long)_score);
  s.drawString(buf, 4, 308, 2);
  snprintf(buf, sizeof(buf), "LIVES %d", _lives);
  int lw = s.textWidth(buf, 2);
  s.drawString(buf, 240 - lw - 4, 308, 2);

  if (_gameOver) {
    s.fillRect(20, 170, 200, 60, bg);
    s.setTextColor(acc, bg);
    const char* msg = (_remaining == 0) ? "YOU WIN!" : "GAME OVER";
    int gw = s.textWidth(msg, 4);
    s.drawString(msg, 120 - gw / 2, 175, 4);
    s.setTextColor(txt, bg);
    int tw = s.textWidth("TAP TO EXIT", 2);
    s.drawString("TAP TO EXIT", 120 - tw / 2, 212, 2);
  }
}

void Breakout::end() {}
#endif

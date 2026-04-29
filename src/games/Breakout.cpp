#include "Breakout.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include "../ui/GameOver.h"

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

  if (_gameOver) {
    bool won = (_remaining == 0);
    drawGameOverOverlay(s, "BREAKOUT", _score, Theme::BREAKOUT565, won);
    return;
  }

  s.fillScreen(Theme::BG565);

  // Bricks (BRICK_TOP=52 now)
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

void Breakout::end() {}
#endif

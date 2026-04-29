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
  // Portrait 240×320: 8 cols × 6 rows bricks at top, paddle at bottom
  static const int COLS = 8, ROWS = 6;
  static const int BRICK_W = 28, BRICK_H = 14;
  static const int PADDLE_W = 56, PADDLE_H = 8;
  static const int BRICK_TOP = 30;  // y offset for first brick row

  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;

  bool     _bricks[ROWS * COLS];
  int      _remaining = 0;
  float    _bx = 120, _by = 220;
  float    _vx = 1.2f, _vy = -1.8f;
  int      _paddleX = 92;          // left edge of paddle
  uint32_t _lastTick = 0;
  uint32_t _score = 0;
  uint32_t _hiScore = 0;
  uint8_t  _lives = 3;
  bool     _done = false;
  bool     _gameOver = false;
  bool     _dirty = true;
};
#endif

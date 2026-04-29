#pragma once
#include <cstdint>

// Portrait Pong: horizontal paddles, player at bottom, AI at top.
// Ball bounces off left/right walls and off paddle surfaces.
class PongLogic {
public:
  static const uint8_t  WIN_SCORE = 5;
  static const int16_t  PAD_W = 60, PAD_H = 8, BALL_R = 5;
  static const int16_t  PLAYER_Y = 290, AI_Y = 24;

  void     init(uint16_t w, uint16_t h);
  void     setPlayerX(int16_t x);      // left edge of player paddle
  bool     tick(uint32_t deltaMs);     // false = match over
  int16_t  ballX()      const { return _bx; }
  int16_t  ballY()      const { return _by; }
  int16_t  playerX()    const { return _px; }
  int16_t  aiX()        const { return _ax; }
  uint8_t  playerScore() const { return _ps; }
  uint8_t  aiScore()     const { return _as; }

private:
  uint16_t _w = 0, _h = 0;
  int16_t  _bx = 0, _by = 0;
  float    _vx = 0.0f, _vy = 0.0f;
  int16_t  _px = 0, _ax = 0;
  uint8_t  _ps = 0, _as = 0;

  void resetBall();
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class Pong : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "PONG"; }
  uint32_t    highScore() override { return _hiScore; }
  uint32_t    score()     override { return (uint32_t)_logic.playerScore() * 100; }
  bool        isDone()    override { return _done; }

private:
  PongLogic     _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  uint32_t      _lastTick = 0;
  uint32_t      _hiScore = 0;
  bool          _done = false;
  bool          _dirty = true;
  bool          _needsFullRedraw = true;  // full clear only on start/score change
  int16_t       _paddleTargetX = 90;
  int16_t       _prevBx = 120, _prevBy = 160;
  int16_t       _prevAx = 90,  _prevPx = 90;
  uint8_t       _prevAScore = 0, _prevPScore = 0;
};
#endif

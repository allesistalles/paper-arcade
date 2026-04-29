#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class SpaceInvaders : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "INVADERS"; }
  uint32_t    score()     override { return _score; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  // Portrait 240×320:
  //   Invaders: 6 cols × 4 rows, 20×12px sprites + gaps, top area
  //   Player: y=275, moves along X
  //   Controls: x<50 = left, x>190 = right, y>240 = shoot
  static const int COLS = 6, ROWS = 4;
  static const int INV_W = 22, INV_H = 14, INV_GAP_X = 12, INV_GAP_Y = 8;
  static const int PLAYER_Y = 275, PLAYER_W = 24;
  static const int MAX_BULLETS = 3, MAX_INV_BULLETS = 2;

  struct Bullet { int16_t x, y; bool active; };

  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;

  bool   _alive[ROWS * COLS];
  int    _aliveCount = 0;
  int    _swarmX = 0, _swarmY = 25;
  int    _swarmDir = 1;
  uint32_t _swarmStep = 0, _swarmStepMs = 700;
  int    _playerX = 108;
  Bullet _bullets[MAX_BULLETS];
  Bullet _invBullets[MAX_INV_BULLETS];
  uint32_t _lastInvShot = 0;
  uint32_t _score = 0;
  uint32_t _hiScore = 0;
  uint8_t  _lives = 3;
  bool   _done = false;
  bool   _gameOver = false;
  bool   _dirty = true;
  bool   _needsFullRedraw = true;

  void shoot();
  void tickSwarm();
  void tickBullets();
  bool checkCollisions();
};
#endif

#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class FlappyBird : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "FLAPPY"; }
  uint32_t    score()     override { return _score; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  // Portrait 240×320: bird moves up/down, pipes scroll from bottom to top
  // Or landscape-style: bird left, pipes come from right — but in portrait
  // it feels better with bird near top and pipes scrolling up? No — keep the
  // classic feel: bird at x=50 (fixed), pipes scroll left, gap is vertical.
  static const int BIRD_X = 55;
  static const int PIPE_W  = 40;
  static const int GAP_H   = 90;
  static const int PIPE_COUNT = 3;

  struct Pipe { int x; int gapY; bool scored; };

  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  float    _birdY = 160;
  float    _vy    = 0;
  Pipe     _pipes[PIPE_COUNT];
  uint32_t _score = 0;
  uint32_t _hiScore = 0;
  uint32_t _lastTick = 0;
  bool     _done = false;
  bool     _gameOver = false;
  bool     _dirty = true;

  int randGapY();
};
#endif

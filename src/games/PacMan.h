#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class PacMan : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "PACMAN"; }
  uint32_t    score()     override { return _score; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  // Portrait: 16×11 maze × 15px = 240×165, y=48..213
  //           Score/lives: y=0..46
  //           D-pad zones: y=220..320
  static const int COLS = 16, ROWS = 11, TILE = 15;
  static const int MAZE_Y = 48;  // top of maze on screen

  static const uint8_t MAZE[ROWS][COLS];
  uint8_t _grid[ROWS][COLS];   // mutable copy

  enum Dir : uint8_t { D_UP, D_DOWN, D_LEFT, D_RIGHT, D_NONE };
  struct Actor { int8_t x, y; Dir dir; uint8_t pix; };

  Actor    _pac;
  Dir      _pacNext = D_NONE;
  Actor    _ghosts[3];
  bool     _frightened = false;
  uint32_t _frightUntil = 0;

  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  uint32_t      _lastTick = 0;
  uint32_t      _score = 0;
  uint32_t      _hiScore = 0;
  uint8_t       _lives = 3;
  int           _dotsLeft = 0;
  bool          _done = false;
  bool          _gameOver = false;
  bool          _dirty = true;

  bool canMove(int x, int y, Dir d) const;
  void moveActor(Actor& a);
  Dir  ghostDir(const Actor& g) const;
  void resetActors();
  void checkGhosts();
  static int  dirDx(Dir d);
  static int  dirDy(Dir d);
  static Dir  reverseDir(Dir d);
};
#endif

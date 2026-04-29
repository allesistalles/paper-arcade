#pragma once
#include <cstdint>
#include <cstring>

class TetrisLogic {
public:
  static const int W = 10, H = 20;

  void init(uint32_t seed);
  bool tick();
  bool tryMove(int dx);
  bool tryRotate();
  void hardDrop();

  uint8_t  cell(int x, int y) const { return _board[y][x]; }
  int      activePieceX()    const { return _px; }
  int      activePieceY()    const { return _py; }
  int      activePieceType() const { return _ptype; }
  int      activePieceRot()  const { return _prot; }
  uint32_t score()           const { return _score; }
  bool     gameOver()        const { return _over; }

  void pieceBlocks(int outX[4], int outY[4]) const;

private:
  uint8_t  _board[H][W] = {};
  int      _ptype = 0, _prot = 0, _px = 0, _py = 0;
  uint32_t _score = 0;
  uint32_t _rng = 1;
  bool     _over = false;

  uint32_t nextRand();
  void     spawn();
  bool     collides(int type, int rot, int px, int py) const;
  void     lock();
  int      clearLines();
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class Tetris : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "TETRIS"; }
  uint32_t    score()     override { return _logic.score(); }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  // Portrait 240×320:
  //   Board: 10 cols × 20 rows × 11px = 110×220, x=65, y=30
  //   Sidebar: x=180..240 (score, next piece)
  //   Left strip x<60: top quarter = rotate, rest = move left
  //   Right strip x>175: move right
  //   Bottom bar y>255: hard drop
  static const int CELL = 11, OFFX = 65, OFFY = 52;

  TetrisLogic   _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  uint32_t      _lastFall = 0;
  uint32_t      _hiScore = 0;
  bool          _done = false;
  bool          _dirty = true;
};
#endif

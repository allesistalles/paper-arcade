#pragma once
#include <cstdint>

class Game2048Logic {
public:
  enum Dir : uint8_t { LEFT, RIGHT, UP, DOWN };

  void     initWithSeed(uint32_t seed);
  void     initEmpty();
  bool     slide(Dir d, bool spawn = true);
  bool     hasMoves() const;
  void     setCell(int x, int y, uint16_t v);
  uint16_t cell(int x, int y) const { return _grid[y * 4 + x]; }
  uint32_t score() const { return _score; }

private:
  uint16_t _grid[16] = {0};
  uint32_t _score = 0;
  uint32_t _rng = 1;

  uint32_t nextRand();
  void     spawnTile();
  bool     slideRow(uint16_t* row);
  void     reverseRow(uint16_t* row);
  void     copyCol(int x, uint16_t* out);
  void     writeCol(int x, const uint16_t* in);
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class Game2048 : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "2048"; }
  uint32_t    highScore() override { return _hiScore; }
  uint32_t    score()     override { return _logic.score(); }
  bool        isDone()    override { return _done; }

private:
  Game2048Logic _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  uint32_t      _hiScore = 0;
  bool          _done = false;
  bool          _gameOver = false;
  bool          _dirty = true;
};
#endif

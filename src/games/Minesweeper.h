#pragma once
#include <cstdint>

class MinesLogic {
public:
  static const int MAX_W = 12, MAX_H = 14;

  void    init(uint8_t w, uint8_t h, uint8_t mines, uint32_t seed);
  bool    reveal(uint8_t x, uint8_t y);
  void    toggleFlag(uint8_t x, uint8_t y);

  bool    isMine(uint8_t x, uint8_t y)     const;
  bool    isRevealed(uint8_t x, uint8_t y) const;
  bool    isFlagged(uint8_t x, uint8_t y)  const;
  uint8_t neighborCount(uint8_t x, uint8_t y) const;
  bool    isExploded() const { return _exploded; }
  bool    isWon()      const;
  int     width()  const { return _w; }
  int     height() const { return _h; }
  int     revealedCount() const { return _revealed_count; }

private:
  uint8_t  _w = 0, _h = 0, _mines = 0;
  bool     _mine[MAX_W * MAX_H]     = {};
  bool     _revealed[MAX_W * MAX_H] = {};
  bool     _flag[MAX_W * MAX_H]     = {};
  bool     _exploded = false;
  int      _revealed_count = 0;
  uint32_t _rng = 1;

  inline int idx(int x, int y) const { return y * _w + x; }
  uint32_t   nextRand();
  void       placeMines(uint8_t count);
  void       floodFill(int x, int y);
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class Minesweeper : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "MINES"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  // Portrait 240x320: 10×11 cells at 24px = 240×264, HUD at y=272..320
  static const int CELL = 24, W = 10, H = 11;
  MinesLogic    _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  uint32_t      _lastTapMs = 0;
  uint8_t       _lastTapX = 255, _lastTapY = 255;
  uint32_t      _hiScore = 0;
  bool          _done = false;
  bool          _dirty = true;
};
#endif

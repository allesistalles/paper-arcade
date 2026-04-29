#pragma once
#include <cstdint>
#include <deque>

struct Point { int16_t x, y; };

class SnakeLogic {
public:
  enum Dir : uint8_t { RIGHT, LEFT, UP, DOWN };

  void     init(uint8_t cols, uint8_t rows);
  void     setDirection(Dir d);
  bool     tick();                                      // false = died
  int      length() const  { return (int)_body.size(); }
  int16_t  headX()  const  { return _body.front().x; }
  int16_t  headY()  const  { return _body.front().y; }
  Point    foodPos() const { return _food; }
  uint32_t score()   const { return _score; }
  const std::deque<Point>& body() const { return _body; }

private:
  std::deque<Point> _body;
  Point    _food = {0,0};
  Dir      _dir = RIGHT, _next = RIGHT;
  uint8_t  _cols = 0, _rows = 0;
  uint32_t _score = 0;

  void placeFood();
  bool onBody(int16_t x, int16_t y) const;
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class Snake : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "SNAKE"; }
  uint32_t    highScore() override { return _hiScore; }
  bool        isDone()    override { return _done; }

private:
  static const uint8_t COLS = 20, ROWS = 15, CELL = 16;
  SnakeLogic    _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  uint32_t      _lastTick = 0, _tickMs = 150;
  uint32_t      _hiScore = 0;
  bool          _done = false;
  bool          _dirty = true;       // redraw the whole screen on next draw()
  bool          _doneScreenShown = false;
};
#endif

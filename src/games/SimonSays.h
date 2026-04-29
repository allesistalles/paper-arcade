#pragma once
#include <cstdint>

class SimonLogic {
public:
  static const int MAX_LEN = 64;

  void    init(uint32_t seed);
  uint8_t sequenceAt(int i) const { return _seq[i]; }
  int     sequenceLength()  const { return _len; }
  bool    checkInput(uint8_t color);
  bool    advanceIfRoundComplete();
  bool    gameOver() const { return _over; }
  int     score()    const { return _len - 1; }
  int     userIndex() const { return _userIdx; }

private:
  uint8_t  _seq[MAX_LEN];
  int      _len = 0, _userIdx = 0;
  bool     _over = false;
  uint32_t _rng = 1;

  uint8_t nextRand();
  void    appendNext();
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../core/Game.h"
#include "../core/AssetManager.h"
#include "../core/ScoreManager.h"

class SimonSays : public Game {
public:
  void        begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) override;
  void        update(const InputEvent& input) override;
  void        draw() override;
  void        end() override;
  const char* name()      override { return "SIMON"; }
  uint32_t    highScore() override { return _hiScore; }
  uint32_t    score()     override { return (uint32_t)_logic.score(); }
  bool        isDone()    override { return _done; }

private:
  // Portrait 240x320: four quadrants each 120x160
  enum Phase { SHOWING, WAITING, FAIL };

  SimonLogic    _logic;
  TFT_eSPI*     _tft = nullptr;
  ScoreManager* _scores = nullptr;
  Phase         _phase = SHOWING;
  int           _showIdx = 0;
  uint32_t      _phaseStart = 0;
  int8_t        _flashColor = -1;
  uint32_t      _hiScore = 0;
  bool          _done = false;
  bool          _dirty = true;

  int8_t hitTest(uint16_t x, uint16_t y);
};
#endif

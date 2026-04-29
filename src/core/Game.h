#pragma once
#include <cstdint>

struct InputEvent {
  enum Type : uint8_t {
    NONE, TAP, SWIPE_LEFT, SWIPE_RIGHT, SWIPE_UP, SWIPE_DOWN, DRAG
  };
  Type     type = NONE;
  uint16_t x = 0, y = 0;
  int16_t  dx = 0, dy = 0;
};

#ifndef NATIVE_TEST
#include <TFT_eSPI.h>

class AssetManager;
class ScoreManager;

class Game {
public:
  virtual ~Game() {}
  virtual void        begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores) = 0;
  virtual void        update(const InputEvent& input) = 0;
  virtual void        draw() = 0;
  virtual void        end() = 0;
  virtual const char* name()      = 0;
  virtual uint32_t    highScore() = 0;
  virtual bool        isDone()    = 0;
};
#endif

#pragma once
#include "Game.h"

#ifndef NATIVE_TEST
#include <XPT2046_Touchscreen.h>
#include <SPI.h>

class InputManager {
public:
  InputManager(uint8_t csPin);
  void       begin();
  InputEvent read();

private:
  XPT2046_Touchscreen _touch;
  bool      _wasTouched = false;
  uint16_t  _startX = 0, _startY = 0;
  uint16_t  _lastX  = 0, _lastY  = 0;
  uint32_t  _touchStart = 0;

  uint16_t mapX(uint16_t raw);
  uint16_t mapY(uint16_t raw);
  static InputEvent::Type classify(int16_t dx, int16_t dy, uint32_t ms);
};
#endif

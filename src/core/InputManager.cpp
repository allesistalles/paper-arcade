#include "InputManager.h"

#ifndef NATIVE_TEST

InputManager::InputManager(uint8_t csPin) : _touch(csPin) {}

void InputManager::begin() {
  _touch.begin();
  // Don't call _touch.setRotation() — mapX/mapY translate raw XPT2046 coords
  // (200..3900, 300..3800) into 320x240 landscape screen coords ourselves.
}

uint16_t InputManager::mapX(uint16_t raw) {
  if (raw < 200) raw = 200;
  if (raw > 3900) raw = 3900;
  return (uint32_t)(raw - 200) * 319 / 3700;
}

uint16_t InputManager::mapY(uint16_t raw) {
  if (raw < 300) raw = 300;
  if (raw > 3800) raw = 3800;
  return (uint32_t)(raw - 300) * 239 / 3500;
}

InputEvent::Type InputManager::classify(int16_t dx, int16_t dy, uint32_t ms) {
  const int SWIPE_THRESH = 30;
  const int MAX_SWIPE_MS = 500;
  if (ms > MAX_SWIPE_MS) return InputEvent::NONE;
  int ax = dx < 0 ? -dx : dx;
  int ay = dy < 0 ? -dy : dy;
  if (ax < SWIPE_THRESH && ay < SWIPE_THRESH) return InputEvent::TAP;
  if (ax > ay) return dx < 0 ? InputEvent::SWIPE_LEFT : InputEvent::SWIPE_RIGHT;
  return dy < 0 ? InputEvent::SWIPE_UP : InputEvent::SWIPE_DOWN;
}

InputEvent InputManager::read() {
  InputEvent evt;
  bool touched = _touch.touched();

  if (touched) {
    TS_Point p = _touch.getPoint();
    uint16_t sx = mapX(p.x);
    uint16_t sy = mapY(p.y);

    if (!_wasTouched) {
      _startX = sx; _startY = sy;
      _lastX  = sx; _lastY  = sy;
      _touchStart = millis();
      _wasTouched = true;
    } else {
      int16_t mdx = (int16_t)sx - (int16_t)_startX;
      int16_t mdy = (int16_t)sy - (int16_t)_startY;
      int amx = mdx < 0 ? -mdx : mdx;
      int amy = mdy < 0 ? -mdy : mdy;
      if (amx > 5 || amy > 5) {
        evt.type = InputEvent::DRAG;
        evt.x = sx; evt.y = sy;
        evt.dx = mdx; evt.dy = mdy;
      }
      _lastX = sx; _lastY = sy;
    }
  } else if (_wasTouched) {
    uint32_t held = millis() - _touchStart;
    int16_t dx = (int16_t)_lastX - (int16_t)_startX;
    int16_t dy = (int16_t)_lastY - (int16_t)_startY;
    evt.type = classify(dx, dy, held);
    evt.x  = _lastX;  evt.y  = _lastY;
    evt.dx = dx;      evt.dy = dy;
    _wasTouched = false;
  }

  return evt;
}

#endif

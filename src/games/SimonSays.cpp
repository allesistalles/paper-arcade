#include "SimonSays.h"

void SimonLogic::init(uint32_t seed) {
  _len = 0; _userIdx = 0; _over = false;
  _rng = seed ? seed : 1;
  appendNext();
}

uint8_t SimonLogic::nextRand() {
  _rng ^= _rng << 13; _rng ^= _rng >> 17; _rng ^= _rng << 5;
  return _rng & 3;
}

void SimonLogic::appendNext() {
  if (_len < MAX_LEN) _seq[_len++] = nextRand();
}

bool SimonLogic::checkInput(uint8_t color) {
  if (_over) return false;
  if (_seq[_userIdx] != color) { _over = true; return false; }
  _userIdx++;
  return true;
}

bool SimonLogic::advanceIfRoundComplete() {
  if (_userIdx == _len) { _userIdx = 0; appendNext(); return true; }
  return false;
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include "../ui/GameOver.h"

// Portrait 240x320 layout:
//   Q0 top-left    (x:0..119,  y:0..159)   — red
//   Q1 top-right   (x:120..239,y:0..159)   — green
//   Q2 bottom-left (x:0..119,  y:160..319) — blue
//   Q3 bottom-right(x:120..239,y:160..319) — yellow
static const uint16_t QUAD_BRIGHT[4] = { 0xF800, 0x07E0, 0x001F, 0xFFE0 };
static const uint16_t QUAD_DIM[4]    = { 0x6000, 0x0300, 0x000C, 0x6320 };

int8_t SimonSays::hitTest(uint16_t x, uint16_t y) {
  int col = (x < 120) ? 0 : 1;
  int row = (y < 160) ? 0 : 1;
  return (int8_t)(row * 2 + col);
}

void SimonSays::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("simon");
  _done = false; _dirty = true;
  _logic.init(millis());
  _phase = SHOWING; _showIdx = 0;
  _flashColor = -1;
  _phaseStart = millis();
}

void SimonSays::update(const InputEvent& input) {
  if (_done) return;
  uint32_t now = millis();

  if (_phase == SHOWING) {
    uint32_t interval = 550;
    uint32_t dt = now - _phaseStart;
    int step = (int)(dt / interval);
    int newFlash = -1;
    if (step < _logic.sequenceLength() * 2) {
      newFlash = (step % 2 == 0) ? _logic.sequenceAt(step / 2) : -1;
    } else {
      _phase = WAITING; _flashColor = -1; _phaseStart = now;
      _dirty = true; return;
    }
    if (newFlash != _flashColor) { _flashColor = newFlash; _dirty = true; }
    return;
  }

  if (_phase == WAITING && input.type == InputEvent::TAP) {
    int8_t color = hitTest(input.x, input.y);
    _flashColor = color; _phaseStart = now; _dirty = true;
    if (!_logic.checkInput((uint8_t)color)) {
      _phase = FAIL;
      _scores->setHighScore("simon", (uint32_t)_logic.score());
      _hiScore = _scores->getHighScore("simon");
    } else if (_logic.advanceIfRoundComplete()) {
      _phase = SHOWING; _showIdx = 0; _phaseStart = now;
    }
  } else if (_phase == WAITING && _flashColor >= 0 && now - _phaseStart > 200) {
    _flashColor = -1; _dirty = true;
  }

  if (_phase == FAIL && input.type == InputEvent::TAP && now - _phaseStart > 500) {
    _scores->setHighScore("simon", (uint32_t)_logic.score());
    _hiScore = _scores->getHighScore("simon");
    _done = true;
    _dirty = true;
  }
}

void SimonSays::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_done || _phase == FAIL) {
    drawGameOverOverlay(s, "SIMON", (uint32_t)_logic.score(), Theme::SIMON565);
    return;
  }

  // Draw 4 quadrants — full screen, HUD composites on top
  for (int8_t i = 0; i < 4; i++) {
    int qx = (i % 2) * 120;
    int qy = (i / 2) * 160;
    uint16_t col = (_flashColor == i) ? QUAD_BRIGHT[i] : QUAD_DIM[i];
    s.fillRect(qx + 2, qy + 2, 116, 156, col);
  }

  // Score in centre circle
  s.fillCircle(120, 160, 28, Theme::BG565);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", _logic.score());
  s.setTextColor(Theme::TEXT565, Theme::BG565);
  int sw = s.textWidth(buf, 4);
  s.drawString(buf, 120 - sw / 2, 150, 4);
}

void SimonSays::end() {}
#endif

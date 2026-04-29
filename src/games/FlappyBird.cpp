#include "FlappyBird.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include <cstdlib>

int FlappyBird::randGapY() {
  return 50 + (rand() % (320 - GAP_H - 80));
}

void FlappyBird::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("flappy");
  _done = false; _gameOver = false; _dirty = true;
  _score = 0; _birdY = 160; _vy = 0;
  srand(millis());
  for (int i = 0; i < PIPE_COUNT; i++) {
    _pipes[i].x = 240 + i * 100;
    _pipes[i].gapY = randGapY();
    _pipes[i].scored = false;
  }
  _lastTick = millis();
}

void FlappyBird::update(const InputEvent& input) {
  if (_done) return;
  if (_gameOver) {
    if (input.type == InputEvent::TAP) {
      _scores->setHighScore("flappy", _score);
      _hiScore = _scores->getHighScore("flappy");
      _done = true;
    }
    return;
  }

  if (input.type == InputEvent::TAP) _vy = -4.0f;

  uint32_t now = millis();
  uint32_t dt = now - _lastTick;
  if (dt > 50) dt = 50;
  _lastTick = now;
  if (dt < 16) return;
  int steps = (int)(dt / 16);

  for (int step = 0; step < steps; step++) {
    _vy += 0.35f;
    _birdY += _vy;

    for (int i = 0; i < PIPE_COUNT; i++) {
      _pipes[i].x -= 2;
      if (_pipes[i].x + PIPE_W < 0) {
        _pipes[i].x += PIPE_COUNT * 100;
        _pipes[i].gapY = randGapY();
        _pipes[i].scored = false;
      }
      if (!_pipes[i].scored && _pipes[i].x + PIPE_W < BIRD_X) {
        _pipes[i].scored = true; _score++;
      }
      // Collision: bird is a circle of radius 8 at (BIRD_X, _birdY)
      if (BIRD_X + 8 > _pipes[i].x && BIRD_X - 8 < _pipes[i].x + PIPE_W) {
        if (_birdY - 8 < _pipes[i].gapY || _birdY + 8 > _pipes[i].gapY + GAP_H)
          _gameOver = true;
      }
    }
    if (_birdY > 312 || _birdY < 8) _gameOver = true;
  }
  _dirty = true;
}

void FlappyBird::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;
  uint16_t bg  = s.color24to16(Theme::BG);
  uint16_t sec = s.color24to16(Theme::SECONDARY);
  uint16_t acc = s.color24to16(Theme::ACCENT);
  uint16_t txt = s.color24to16(Theme::TEXT);

  s.fillScreen(bg);

  // Pipes
  for (int i = 0; i < PIPE_COUNT; i++) {
    Pipe& p = _pipes[i];
    // Left pipe (before gap)
    s.fillRect(p.x, 0, PIPE_W, p.gapY, sec);
    // Right pipe (after gap)
    s.fillRect(p.x, p.gapY + GAP_H, PIPE_W, 320 - (p.gapY + GAP_H), sec);
    s.drawRect(p.x, 0, PIPE_W, p.gapY, acc);
    s.drawRect(p.x, p.gapY + GAP_H, PIPE_W, 320 - (p.gapY + GAP_H), acc);
  }

  // Bird (yellow circle)
  s.fillCircle(BIRD_X, (int)_birdY, 8, 0xFFE0);
  s.fillCircle(BIRD_X + 3, (int)_birdY - 2, 2, 0x0000);

  // Score
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)_score);
  s.setTextColor(txt, bg);
  s.drawString(buf, 4, 4, 4);

  if (_gameOver) {
    s.fillRect(20, 130, 200, 60, bg);
    s.setTextColor(acc, bg);
    int gw = s.textWidth("GAME OVER", 4);
    s.drawString("GAME OVER", 120 - gw / 2, 135, 4);
    s.setTextColor(txt, bg);
    int tw = s.textWidth("TAP TO EXIT", 2);
    s.drawString("TAP TO EXIT", 120 - tw / 2, 172, 2);
  }
}

void FlappyBird::end() {}
#endif

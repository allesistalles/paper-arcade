#include "FlappyBird.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include "../ui/GameOver.h"
#include <cstdlib>

int FlappyBird::randGapY() {
  return 50 + (rand() % (320 - GAP_H - 80));
}

void FlappyBird::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("flappy");
  _done = false; _gameOver = false; _dirty = true; _needsFullRedraw = true;
  _score = 0; _birdY = 160; _prevBirdY = 160; _vy = 0;
  srand(millis());
  for (int i = 0; i < PIPE_COUNT; i++) {
    _pipes[i].x = 240 + i * 100;
    _pipes[i].gapY = randGapY();
    _pipes[i].scored = false;
    _prevPipeX[i] = _pipes[i].x;
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

  if (_gameOver) {
    drawGameOverOverlay(s, "FLAPPY", _score, Theme::FLAPPY565);
    _needsFullRedraw = true;
    return;
  }

  if (_needsFullRedraw) {
    s.fillRect(0, 22, 240, 298, Theme::BG565);
    _needsFullRedraw = false;
  }

  // Each pipe moved 2px left since last frame.
  // Fill the 2px strip revealed on the left of the old position with BG.
  // Fill the 2px strip now occupied on the right of the new position with pipe color.
  for (int i = 0; i < PIPE_COUNT; i++) {
    Pipe& p = _pipes[i];
    int dx = _prevPipeX[i] - p.x;   // should be ~2
    if (dx > 0) {
      // Erase left edge of old pipe position
      s.fillRect(_prevPipeX[i], 0, dx, p.gapY, Theme::BG565);
      s.fillRect(_prevPipeX[i], p.gapY + GAP_H, dx, 320 - (p.gapY + GAP_H), Theme::BG565);
    }
    // Draw right edge of new pipe position
    s.fillRect(p.x + PIPE_W - dx, 0, dx, p.gapY, Theme::SNAKE565);
    s.fillRect(p.x + PIPE_W - dx, p.gapY + GAP_H, dx, 320 - (p.gapY + GAP_H), Theme::SNAKE565);
    // Redraw pipe outlines
    s.drawRect(p.x, 0, PIPE_W, p.gapY, Theme::FLAPPY565);
    s.drawRect(p.x, p.gapY + GAP_H, PIPE_W, 320 - (p.gapY + GAP_H), Theme::FLAPPY565);
    _prevPipeX[i] = p.x;
  }

  // Erase old bird, draw new bird
  s.fillCircle(BIRD_X, (int)_prevBirdY, 9, Theme::BG565);
  s.fillCircle(BIRD_X, (int)_birdY, 8, Theme::FLAPPY565);
  s.fillCircle(BIRD_X + 3, (int)_birdY - 2, 2, Theme::BG565);
  _prevBirdY = _birdY;
}

void FlappyBird::end() {}
#endif

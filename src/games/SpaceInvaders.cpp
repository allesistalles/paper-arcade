#include "SpaceInvaders.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include "../ui/GameOver.h"
#include <cstdlib>

void SpaceInvaders::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("invaders");
  _done = false; _gameOver = false; _dirty = true; _needsFullRedraw = true;
  _score = 0; _lives = 3;
  _aliveCount = ROWS * COLS;
  for (int i = 0; i < ROWS * COLS; i++) _alive[i] = true;
  _swarmX = 4; _swarmY = 35;
  _swarmDir = 1; _swarmStepMs = 700;
  _playerX = 108;
  for (int i = 0; i < MAX_BULLETS; i++) _bullets[i].active = false;
  for (int i = 0; i < MAX_INV_BULLETS; i++) _invBullets[i].active = false;
  _swarmStep = millis(); _lastInvShot = millis();
  srand(millis());
}

void SpaceInvaders::shoot() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!_bullets[i].active) {
      _bullets[i] = { (int16_t)(_playerX + PLAYER_W / 2), (int16_t)(PLAYER_Y - 4), true };
      return;
    }
  }
}

void SpaceInvaders::tickSwarm() {
  if (millis() - _swarmStep < _swarmStepMs) return;
  _swarmStep = millis();

  // Find leftmost/rightmost alive column
  int minCol = COLS, maxCol = -1;
  for (int r = 0; r < ROWS; r++)
    for (int c = 0; c < COLS; c++)
      if (_alive[r * COLS + c]) { if (c < minCol) minCol = c; if (c > maxCol) maxCol = c; }

  int leftX  = _swarmX + minCol * (INV_W + INV_GAP_X);
  int rightX = _swarmX + maxCol * (INV_W + INV_GAP_X) + INV_W;

  if ((_swarmDir > 0 && rightX + 5 >= 240) || (_swarmDir < 0 && leftX - 5 <= 0)) {
    _swarmDir = -_swarmDir;
    _swarmY += 10;
  } else {
    _swarmX += _swarmDir * 5;
  }

  _swarmStepMs = 150 + (uint32_t)_aliveCount * 18;

  // Random invader shoots
  if (millis() - _lastInvShot > 600 + (rand() % 1200)) {
    _lastInvShot = millis();
    for (int i = 0; i < MAX_INV_BULLETS; i++) {
      if (_invBullets[i].active) continue;
      int c = rand() % COLS;
      for (int r = ROWS - 1; r >= 0; r--) {
        if (_alive[r * COLS + c]) {
          int sx = _swarmX + c * (INV_W + INV_GAP_X) + INV_W / 2;
          int sy = _swarmY + r * (INV_H + INV_GAP_Y) + INV_H;
          _invBullets[i] = { (int16_t)sx, (int16_t)sy, true };
          break;
        }
      }
      break;
    }
  }
}

void SpaceInvaders::tickBullets() {
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (!_bullets[i].active) continue;
    _bullets[i].y -= 7;
    if (_bullets[i].y < 0) _bullets[i].active = false;
  }
  for (int i = 0; i < MAX_INV_BULLETS; i++) {
    if (!_invBullets[i].active) continue;
    _invBullets[i].y += 5;
    if (_invBullets[i].y > 320) _invBullets[i].active = false;
  }
}

bool SpaceInvaders::checkCollisions() {
  for (int b = 0; b < MAX_BULLETS; b++) {
    if (!_bullets[b].active) continue;
    for (int r = 0; r < ROWS; r++) {
      for (int c = 0; c < COLS; c++) {
        if (!_alive[r * COLS + c]) continue;
        int ix = _swarmX + c * (INV_W + INV_GAP_X);
        int iy = _swarmY + r * (INV_H + INV_GAP_Y);
        if (_bullets[b].x >= ix && _bullets[b].x <= ix + INV_W &&
            _bullets[b].y >= iy && _bullets[b].y <= iy + INV_H) {
          _alive[r * COLS + c] = false;
          _aliveCount--;
          _bullets[b].active = false;
          _score += (ROWS - r) * 10;
        }
      }
    }
  }
  for (int b = 0; b < MAX_INV_BULLETS; b++) {
    if (!_invBullets[b].active) continue;
    if (_invBullets[b].y >= PLAYER_Y && _invBullets[b].y <= PLAYER_Y + 10 &&
        _invBullets[b].x >= _playerX && _invBullets[b].x <= _playerX + PLAYER_W) {
      _invBullets[b].active = false;
      _lives--;
      if (_lives == 0) return true;
    }
  }
  if (_swarmY + ROWS * (INV_H + INV_GAP_Y) >= PLAYER_Y) return true;
  return false;
}

void SpaceInvaders::update(const InputEvent& input) {
  if (_done) return;
  if (_gameOver) {
    if (input.type == InputEvent::TAP) {
      _scores->setHighScore("invaders", _score);
      _hiScore = _scores->getHighScore("invaders");
      _done = true;
    }
    return;
  }

  if (input.type == InputEvent::TAP) {
    if      (input.x < 50)   _playerX -= 14;
    else if (input.x > 190)  _playerX += 14;
    else                      shoot();
    if (_playerX < 0) _playerX = 0;
    if (_playerX + PLAYER_W > 240) _playerX = 240 - PLAYER_W;
  }

  tickSwarm();
  tickBullets();
  if (checkCollisions() || _aliveCount == 0) _gameOver = true;
  _dirty = true;
}

// Previous bullet positions for erase-before-draw
static int16_t sPrevBulX[3]    = {-1,-1,-1}, sPrevBulY[3]    = {-1,-1,-1};
static int16_t sPrevInvBulX[2] = {-1,-1},    sPrevInvBulY[2] = {-1,-1};
static int      sPrevPlayerX   = -1;

void SpaceInvaders::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;

  if (_gameOver) {
    drawGameOverOverlay(s, "INVADERS", _score, Theme::INVADERS565, (_aliveCount == 0));
    _needsFullRedraw = true;
    return;
  }

  static const uint16_t ROW_COLS[4] = {
    Theme::DANGER565, Theme::MINES565, Theme::SNAKE565, Theme::TETRIS565
  };

  if (_needsFullRedraw) {
    s.fillRect(0, 22, 240, 298, Theme::BG565);
    // Static elements
    for (int i = 0; i < 3; i++)
      s.fillCircle(108 + i * 14, 310, 4, Theme::INVADERS565);
    s.setTextColor(Theme::SEP565, Theme::BG565);
    s.drawString("<", 14, 290, 4);
    s.drawString(">", 210, 290, 4);
    s.drawString("FIRE", 98, 292, 2);
    // Force redraws of tracked elements
    sPrevPlayerX = -1;
    for (int i = 0; i < MAX_BULLETS; i++) sPrevBulX[i] = -1;
    for (int i = 0; i < MAX_INV_BULLETS; i++) sPrevInvBulX[i] = -1;
    _needsFullRedraw = false;
  }

  // Invaders — only redraw on swarm march (tickSwarm sets dirty)
  // Wipe old swarm area and redraw to handle movement
  // Since swarm moves infrequently, a targeted redraw is fast enough
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      int ix = _swarmX + c * (INV_W + INV_GAP_X);
      int iy = _swarmY + r * (INV_H + INV_GAP_Y);
      if (_alive[r * COLS + c]) {
        s.fillRect(ix, iy, INV_W, INV_H, ROW_COLS[r]);
        s.fillRect(ix + 4, iy + 3, 3, 3, Theme::BG565);
        s.fillRect(ix + 15, iy + 3, 3, 3, Theme::BG565);
      }
    }
  }

  // Player — erase old, draw new
  if (sPrevPlayerX >= 0) {
    s.fillRect(sPrevPlayerX, PLAYER_Y, PLAYER_W, 10, Theme::BG565);
    s.fillRect(sPrevPlayerX + 9, PLAYER_Y - 5, 4, 5, Theme::BG565);
  }
  s.fillRect(_playerX, PLAYER_Y, PLAYER_W, 10, Theme::INVADERS565);
  s.fillRect(_playerX + 9, PLAYER_Y - 5, 4, 5, Theme::INVADERS565);
  sPrevPlayerX = _playerX;

  // Player bullets — erase old position, draw new
  for (int i = 0; i < MAX_BULLETS; i++) {
    if (sPrevBulX[i] >= 0) s.fillRect(sPrevBulX[i], sPrevBulY[i], 2, 9, Theme::BG565);
    if (_bullets[i].active) {
      s.fillRect(_bullets[i].x, _bullets[i].y, 2, 7, Theme::TEXT565);
      sPrevBulX[i] = _bullets[i].x; sPrevBulY[i] = _bullets[i].y;
    } else {
      sPrevBulX[i] = -1;
    }
  }
  for (int i = 0; i < MAX_INV_BULLETS; i++) {
    if (sPrevInvBulX[i] >= 0) s.fillRect(sPrevInvBulX[i], sPrevInvBulY[i], 2, 9, Theme::BG565);
    if (_invBullets[i].active) {
      s.fillRect(_invBullets[i].x, _invBullets[i].y, 2, 7, Theme::DANGER565);
      sPrevInvBulX[i] = _invBullets[i].x; sPrevInvBulY[i] = _invBullets[i].y;
    } else {
      sPrevInvBulX[i] = -1;
    }
  }
}

void SpaceInvaders::end() {}
#endif

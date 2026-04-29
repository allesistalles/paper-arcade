#include "SpaceInvaders.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"
#include <cstdlib>

void SpaceInvaders::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("invaders");
  _done = false; _gameOver = false; _dirty = true;
  _score = 0; _lives = 3;
  _aliveCount = ROWS * COLS;
  for (int i = 0; i < ROWS * COLS; i++) _alive[i] = true;
  _swarmX = 4; _swarmY = 25;
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

void SpaceInvaders::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;
  uint16_t bg  = s.color24to16(Theme::BG);
  uint16_t acc = s.color24to16(Theme::ACCENT);
  uint16_t dim = s.color24to16(Theme::DIM);
  uint16_t txt = s.color24to16(Theme::TEXT);

  s.fillScreen(bg);

  static const uint16_t ROW_COLS[4] = { 0xF800, 0xFD20, 0x07E0, 0x07FF };
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      if (!_alive[r * COLS + c]) continue;
      int ix = _swarmX + c * (INV_W + INV_GAP_X);
      int iy = _swarmY + r * (INV_H + INV_GAP_Y);
      uint16_t col = ROW_COLS[r];
      s.fillRect(ix, iy, INV_W, INV_H, col);
      // Simple eye-like detail
      s.fillRect(ix + 4, iy + 3, 3, 3, 0x0000);
      s.fillRect(ix + 15, iy + 3, 3, 3, 0x0000);
    }
  }

  // Player ship
  s.fillRect(_playerX, PLAYER_Y, PLAYER_W, 10, dim);
  s.fillRect(_playerX + 10, PLAYER_Y - 5, 4, 5, dim);

  // Player bullets
  for (int i = 0; i < MAX_BULLETS; i++)
    if (_bullets[i].active)
      s.fillRect(_bullets[i].x, _bullets[i].y, 2, 7, txt);
  for (int i = 0; i < MAX_INV_BULLETS; i++)
    if (_invBullets[i].active)
      s.fillRect(_invBullets[i].x, _invBullets[i].y, 2, 7, acc);

  // HUD
  char buf[32];
  s.setTextColor(txt, bg);
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)_score);
  s.drawString(buf, 4, 4, 2);
  snprintf(buf, sizeof(buf), "LIVES %d", _lives);
  int lw = s.textWidth(buf, 2);
  s.drawString(buf, 240 - lw - 4, 4, 2);

  // Control hints
  s.setTextColor(s.color24to16(0x1a1040), bg);
  s.drawString("<", 14, 290, 4);
  s.drawString(">", 210, 290, 4);
  s.drawString("FIRE", 98, 292, 2);

  if (_gameOver) {
    s.fillRect(20, 120, 200, 70, bg);
    s.setTextColor(acc, bg);
    const char* msg = (_aliveCount == 0) ? "VICTORY!" : "GAME OVER";
    int gw = s.textWidth(msg, 4);
    s.drawString(msg, 120 - gw / 2, 128, 4);
    s.setTextColor(txt, bg);
    int tw = s.textWidth("TAP TO EXIT", 2);
    s.drawString("TAP TO EXIT", 120 - tw / 2, 168, 2);
  }
}

void SpaceInvaders::end() {}
#endif

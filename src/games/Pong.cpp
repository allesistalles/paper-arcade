#include "Pong.h"
#include <cstdlib>
#include <cmath>

void PongLogic::init(uint16_t w, uint16_t h) {
  _w = w; _h = h;
  _px = _ax = (w - PAD_W) / 2;
  _ps = _as = 0;
  resetBall();
}

void PongLogic::resetBall() {
  _bx = _w / 2; _by = _h / 2;
  _vx = ((rand() % 100) - 50) * 0.003f;
  _vy = ((rand() % 2) ? 1 : -1) * 0.18f;
}

void PongLogic::setPlayerX(int16_t x) {
  if (x < 0) x = 0;
  if (x + PAD_W > (int16_t)_w) x = _w - PAD_W;
  _px = x;
}

bool PongLogic::tick(uint32_t dt) {
  if (dt > 50) dt = 50;

  // AI: follow ball X, limited speed
  int16_t aiMid = _ax + PAD_W / 2;
  int16_t aiSpd = 3;
  if (aiMid + 3 < _bx) _ax += aiSpd;
  else if (aiMid - 3 > _bx) _ax -= aiSpd;
  if (_ax < 0) _ax = 0;
  if (_ax + PAD_W > (int16_t)_w) _ax = _w - PAD_W;

  _bx += (int16_t)(_vx * dt);
  _by += (int16_t)(_vy * dt);

  // Left/right wall bounce
  if (_bx - BALL_R < 0)           { _bx = BALL_R;      _vx = -_vx; }
  if (_bx + BALL_R > (int16_t)_w) { _bx = _w - BALL_R; _vx = -_vx; }

  // Player paddle (bottom): y = PLAYER_Y
  if (_by + BALL_R >= PLAYER_Y && _by + BALL_R <= PLAYER_Y + PAD_H &&
      _bx >= _px && _bx <= _px + PAD_W) {
    _vy = -fabsf(_vy) * 1.05f;
    _vx += ((float)(_bx - (_px + PAD_W / 2))) * 0.005f;
    _by = PLAYER_Y - BALL_R;
  }
  // AI paddle (top): y = AI_Y
  if (_by - BALL_R <= AI_Y + PAD_H && _by - BALL_R >= AI_Y &&
      _bx >= _ax && _bx <= _ax + PAD_W) {
    _vy = fabsf(_vy) * 1.05f;
    _by = AI_Y + PAD_H + BALL_R;
  }

  // Score: ball exits top or bottom
  if (_by > (int16_t)_h) { _ps++; resetBall(); }  // player scores
  if (_by < 0)            { _as++; resetBall(); }  // AI scores

  return _ps < WIN_SCORE && _as < WIN_SCORE;
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"

void Pong::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft = &tft; _scores = &scores;
  _hiScore = scores.getHighScore("pong");
  _done = false;
  _dirty = true;
  srand(millis());
  _logic.init(240, 320);
  _paddleTargetX = 90;
  _lastTick = millis();
}

void Pong::update(const InputEvent& input) {
  if (_done) return;
  if (input.type == InputEvent::TAP || input.type == InputEvent::DRAG) {
    // Touch X maps directly to paddle center
    _paddleTargetX = (int16_t)input.x - PongLogic::PAD_W / 2;
  }
  _logic.setPlayerX(_paddleTargetX);

  uint32_t now = millis();
  if (!_logic.tick(now - _lastTick)) {
    uint32_t s = (uint32_t)_logic.playerScore() * 100;
    _scores->setHighScore("pong", s);
    _hiScore = _scores->getHighScore("pong");
    _done = true;
  }
  _lastTick = now;
  _dirty = true;
}

void Pong::draw() {
  if (!_dirty) return;
  _dirty = false;

  TFT_eSPI& s = *_tft;
  uint16_t bg  = s.color24to16(Theme::BG);
  uint16_t acc = s.color24to16(Theme::ACCENT);
  uint16_t dim = s.color24to16(Theme::DIM);
  uint16_t txt = s.color24to16(Theme::TEXT);
  uint16_t drk = s.color24to16(Theme::DARK);

  s.fillScreen(bg);

  // Center dashed line
  for (int x = 0; x < 240; x += 14)
    s.drawFastHLine(x, 160, 8, drk);

  // AI paddle (top, cyan)
  s.fillRect(_logic.aiX(), PongLogic::AI_Y, PongLogic::PAD_W, PongLogic::PAD_H, dim);
  // Player paddle (bottom, pink)
  s.fillRect(_logic.playerX(), PongLogic::PLAYER_Y, PongLogic::PAD_W, PongLogic::PAD_H, acc);
  // Ball
  s.fillCircle(_logic.ballX(), _logic.ballY(), PongLogic::BALL_R, txt);

  // Scores
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", _logic.aiScore());
  s.setTextColor(dim, bg);
  s.drawString(buf, 20, 120, 6);
  snprintf(buf, sizeof(buf), "%d", _logic.playerScore());
  s.setTextColor(acc, bg);
  s.drawString(buf, 140, 120, 6);

  if (_done) {
    bool won = _logic.playerScore() >= PongLogic::WIN_SCORE;
    s.setTextColor(acc, bg);
    int gw = s.textWidth(won ? "YOU WIN!" : "GAME OVER", 4);
    s.drawString(won ? "YOU WIN!" : "GAME OVER", 120 - gw / 2, 145, 4);
    s.setTextColor(txt, bg);
    int tw = s.textWidth("TAP TO EXIT", 2);
    s.drawString("TAP TO EXIT", 120 - tw / 2, 185, 2);
  }
}

void Pong::end() {
  // Nothing to free — direct render.
}
#endif

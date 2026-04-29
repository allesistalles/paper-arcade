#include "Snake.h"
#include <cstdlib>

#ifdef NATIVE_TEST
static uint32_t native_seed = 42;
static uint32_t now_ms()    { return native_seed; }
#else
static uint32_t now_ms()    { return millis(); }
#endif

void SnakeLogic::init(uint8_t cols, uint8_t rows) {
  _cols = cols; _rows = rows;
  _body.clear();
  int16_t cy = rows / 2;
  _body.push_front({5, cy});
  _body.push_front({6, cy});
  _body.push_front({7, cy});
  _dir = _next = RIGHT;
  _score = 0;
  srand(now_ms());
  placeFood();
}

void SnakeLogic::setDirection(Dir d) {
  if ((_dir == RIGHT && d == LEFT) || (_dir == LEFT && d == RIGHT)) return;
  if ((_dir == UP    && d == DOWN) || (_dir == DOWN && d == UP))    return;
  _next = d;
}

bool SnakeLogic::tick() {
  _dir = _next;
  Point head = _body.front();
  switch (_dir) {
    case RIGHT: head.x++; break;
    case LEFT:  head.x--; break;
    case UP:    head.y--; break;
    case DOWN:  head.y++; break;
  }
  if (head.x < 0 || head.x >= _cols || head.y < 0 || head.y >= _rows) return false;

  bool ate = (head.x == _food.x && head.y == _food.y);
  if (!ate) _body.pop_back();
  if (onBody(head.x, head.y)) return false;
  _body.push_front(head);
  if (ate) { _score += 10; placeFood(); }
  return true;
}

void SnakeLogic::placeFood() {
  do {
    _food.x = rand() % _cols;
    _food.y = rand() % _rows;
  } while (onBody(_food.x, _food.y));
}

bool SnakeLogic::onBody(int16_t x, int16_t y) const {
  for (auto& p : _body)
    if (p.x == x && p.y == y) return true;
  return false;
}

#ifndef NATIVE_TEST
#include "../ui/Theme.h"

void Snake::begin(TFT_eSPI& tft, AssetManager&, ScoreManager& scores) {
  _tft    = &tft;
  _scores = &scores;
  _hiScore = scores.getHighScore("snake");
  _done    = false;
  _doneScreenShown = false;
  _tickMs  = 150;
  _logic.init(COLS, ROWS);
  _lastTick = millis();
  _dirty = true;
}

void Snake::update(const InputEvent& input) {
  if (_done) return;
  if (input.type == InputEvent::SWIPE_LEFT)  _logic.setDirection(SnakeLogic::LEFT);
  if (input.type == InputEvent::SWIPE_RIGHT) _logic.setDirection(SnakeLogic::RIGHT);
  if (input.type == InputEvent::SWIPE_UP)    _logic.setDirection(SnakeLogic::UP);
  if (input.type == InputEvent::SWIPE_DOWN)  _logic.setDirection(SnakeLogic::DOWN);

  uint32_t step = 150 - (_logic.score() / 50) * 10;
  if (step < 60) step = 60;
  _tickMs = step;

  if (millis() - _lastTick >= _tickMs) {
    _lastTick = millis();
    if (!_logic.tick()) {
      _scores->setHighScore("snake", _logic.score());
      _hiScore = _scores->getHighScore("snake");
      _done = true;
    }
    _dirty = true;
  }
}

void Snake::draw() {
  if (!_dirty) return;            // only redraw on game tick / state change
  _dirty = false;

  TFT_eSPI& s = *_tft;
  uint16_t bg     = s.color24to16(Theme::BG);
  uint16_t accent = s.color24to16(Theme::ACCENT);
  uint16_t dim    = s.color24to16(Theme::DIM);
  uint16_t sec    = s.color24to16(Theme::SECONDARY);
  uint16_t text   = s.color24to16(Theme::TEXT);

  s.fillScreen(bg);

  // Food
  Point f = _logic.foodPos();
  s.fillRect(f.x * CELL + 2, f.y * CELL + 2, CELL - 4, CELL - 4, accent);

  // Snake body — head distinct from rest
  const auto& body = _logic.body();
  for (size_t i = 0; i < body.size(); i++) {
    uint16_t c = (i == 0) ? dim : sec;
    s.fillRect(body[i].x * CELL + 1, body[i].y * CELL + 1, CELL - 2, CELL - 2, c);
  }

  // HUD line
  char buf[32];
  snprintf(buf, sizeof(buf), "SCORE %lu", (unsigned long)_logic.score());
  s.setTextColor(text, bg);
  s.drawString(buf, 4, 224, 2);
  snprintf(buf, sizeof(buf), "HI %lu", (unsigned long)_hiScore);
  s.drawString(buf, 240, 224, 2);

  if (_done) {
    s.setTextColor(accent, bg);
    s.drawString("GAME OVER", 80, 90, 4);
    s.setTextColor(text, bg);
    s.drawString("TAP TO EXIT", 90, 130, 2);
    _doneScreenShown = true;
  }
}

void Snake::end() {
  // Nothing to free — we don't own any heap allocations.
}
#endif

#include "Launcher.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"

// Portrait layout: 240w × 320h
//   Title:        y=8..36   (size 4)
//   Card:         x=10..230 (220w), y=50..270 (220h)
//     Initial:    centered ~y=85   (size 7)
//     Name:       centered ~y=185  (size 4)
//     HI score:   centered ~y=230  (size 2)
//   Arrows:       y=140 (size 6) at x=2 (left) and x=222 (right)
//   Dots:         y=295

void Launcher::begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores) {
  _tft = &tft; _assets = &assets; _scores = &scores;
  _needsRedraw = true;
}

void Launcher::addGame(Game* g) {
  if (_count < MAX_GAMES) _games[_count++] = g;
}

Game* Launcher::update(const InputEvent& evt) {
  // Must return _active on every in-game frame (even on NONE) — paper-arcade.ino's
  // launch-tap suppression depends on launcher.update() consistently signaling
  // in-game state via a non-null return.
  if (_inGame) return _active;
  if (_count == 0) return nullptr;

  if (evt.type == InputEvent::SWIPE_LEFT && _current < _count - 1) {
    _current++;
    _needsRedraw = true;
  } else if (evt.type == InputEvent::SWIPE_RIGHT && _current > 0) {
    _current--;
    _needsRedraw = true;
  } else if (evt.type == InputEvent::TAP) {
    Serial.printf("[launcher] TAP at (%u,%u) count=%d current=%d\n",
                  evt.x, evt.y, _count, _current);
    // Card hit-box: x∈[10,230], y∈[50,270]
    if (evt.x >= 10 && evt.x <= 230 && evt.y >= 50 && evt.y <= 270) {
      Serial.printf("[launcher] LAUNCHING %s\n", _games[_current]->name());
      _active = _games[_current];
      _inGame = true;
      _active->begin(*_tft, *_assets, *_scores);
      Serial.println("[launcher] begin() returned, returning to main loop");
      return _active;
    } else {
      Serial.println("[launcher] tap outside card hit-box");
    }
  }
  return nullptr;
}

void Launcher::draw() {
  if (_inGame) { if (_active) _active->draw(); return; }
  if (!_needsRedraw) return;

  TFT_eSPI& t = *_tft;
  t.fillScreen(t.color24to16(Theme::BG));
  t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::BG));
  // "PAPER ARCADE" centered, size 4 → ~168px wide, x = (240-168)/2 = 36
  t.drawString("PAPER ARCADE", 36, 12, 4);

  if (_count > 0) drawCard();
  drawArrows();
  drawDots();

  _needsRedraw = false;
}

void Launcher::drawCard() {
  TFT_eSPI& t = *_tft;
  Game* g = _games[_current];

  int x = 10, y = 50, w = 220, h = 220;
  t.drawRoundRect(x, y, w, h, 8, t.color24to16(Theme::ACCENT));
  t.fillRoundRect(x + 1, y + 1, w - 2, h - 2, 7, t.color24to16(Theme::DARK));

  // Game icon: large initial letter, centered
  char initial[2] = { g->name()[0], 0 };
  int initW = t.textWidth(initial, 7);
  t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::DARK));
  t.drawString(initial, 120 - initW / 2, 90, 7);

  // Game name, centered
  t.setTextColor(t.color24to16(Theme::TEXT), t.color24to16(Theme::DARK));
  int nameW = t.textWidth(g->name(), 4);
  t.drawString(g->name(), 120 - nameW / 2, 195, 4);

  // High score, centered
  char buf[32];
  snprintf(buf, sizeof(buf), "HI: %lu", (unsigned long)g->highScore());
  t.setTextColor(t.color24to16(Theme::DIM), t.color24to16(Theme::DARK));
  int hiW = t.textWidth(buf, 2);
  t.drawString(buf, 120 - hiW / 2, 235, 2);
}

void Launcher::drawArrows() {
  TFT_eSPI& t = *_tft;
  if (_current > 0) {
    t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::BG));
    t.drawString("<", 0, 145, 6);
  }
  if (_current < _count - 1) {
    t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::BG));
    t.drawString(">", 222, 145, 6);
  }
}

void Launcher::drawDots() {
  TFT_eSPI& t = *_tft;
  if (_count == 0) return;
  int totalW = _count * 12;
  int startX = (240 - totalW) / 2 + 4;
  for (int i = 0; i < _count; i++) {
    uint16_t col = (i == _current) ? t.color24to16(Theme::ACCENT) : t.color24to16(Theme::DARK);
    int r = (i == _current) ? 4 : 3;
    t.fillCircle(startX + i * 12, 295, r, col);
  }
}

void Launcher::returnToMenu() {
  if (_active) { _active->end(); _active = nullptr; }
  _inGame = false;
  _needsRedraw = true;
}

#endif

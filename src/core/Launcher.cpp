#include "Launcher.h"
#ifndef NATIVE_TEST
#include "../ui/Theme.h"

void Launcher::begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores) {
  _tft = &tft; _assets = &assets; _scores = &scores;
  _needsRedraw = true;
}

void Launcher::addGame(Game* g) {
  if (_count < MAX_GAMES) _games[_count++] = g;
}

Game* Launcher::update(const InputEvent& evt) {
  if (_inGame) return _active;
  if (_count == 0) return nullptr;

  if (evt.type == InputEvent::SWIPE_LEFT && _current < _count - 1) {
    _current++;
    _needsRedraw = true;
  } else if (evt.type == InputEvent::SWIPE_RIGHT && _current > 0) {
    _current--;
    _needsRedraw = true;
  } else if (evt.type == InputEvent::TAP) {
    if (evt.x > 50 && evt.x < 270 && evt.y > 35 && evt.y < 205) {
      _active = _games[_current];
      _inGame = true;
      _active->begin(*_tft, *_assets, *_scores);
      return _active;
    }
  }
  return nullptr;
}

void Launcher::draw() {
  if (_inGame) { _active->draw(); return; }
  if (!_needsRedraw) return;

  TFT_eSPI& t = *_tft;
  t.fillScreen(t.color24to16(Theme::BG));
  t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::BG));
  t.drawString("PAPER ARCADE", 60, 8, 4);

  if (_count > 0) drawCard();
  drawArrows();
  drawDots();

  _needsRedraw = false;
}

void Launcher::drawCard() {
  TFT_eSPI& t = *_tft;
  Game* g = _games[_current];

  int x = 60, y = 38;
  t.drawRoundRect(x, y, 200, 165, 8, t.color24to16(Theme::ACCENT));
  t.fillRoundRect(x + 1, y + 1, 198, 163, 7, t.color24to16(Theme::DARK));

  // Game icon placeholder: large initial letter in pink
  char initial[2] = { g->name()[0], 0 };
  t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::DARK));
  t.drawString(initial, 145, 60, 7);

  t.setTextColor(t.color24to16(Theme::TEXT), t.color24to16(Theme::DARK));
  int nameW = t.textWidth(g->name(), 4);
  t.drawString(g->name(), 160 - nameW / 2, 135, 4);

  char buf[32];
  snprintf(buf, sizeof(buf), "HI: %lu", (unsigned long)g->highScore());
  t.setTextColor(t.color24to16(Theme::DIM), t.color24to16(Theme::DARK));
  int hiW = t.textWidth(buf, 2);
  t.drawString(buf, 160 - hiW / 2, 175, 2);
}

void Launcher::drawArrows() {
  TFT_eSPI& t = *_tft;
  if (_current > 0) {
    t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::BG));
    t.drawString("<", 20, 110, 6);
  }
  if (_current < _count - 1) {
    t.setTextColor(t.color24to16(Theme::ACCENT), t.color24to16(Theme::BG));
    t.drawString(">", 290, 110, 6);
  }
}

void Launcher::drawDots() {
  TFT_eSPI& t = *_tft;
  if (_count == 0) return;
  int totalW = _count * 12;
  int startX = (320 - totalW) / 2 + 4;
  for (int i = 0; i < _count; i++) {
    uint16_t col = (i == _current) ? t.color24to16(Theme::ACCENT) : t.color24to16(Theme::DARK);
    int r = (i == _current) ? 4 : 3;
    t.fillCircle(startX + i * 12, 220, r, col);
  }
}

void Launcher::returnToMenu() {
  if (_active) { _active->end(); _active = nullptr; }
  _inGame = false;
  _needsRedraw = true;
}

#endif

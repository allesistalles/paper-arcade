#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "Game.h"
#include "AssetManager.h"
#include "ScoreManager.h"

class Launcher {
public:
  static const int MAX_GAMES = 10;

  void  begin(TFT_eSPI& tft, AssetManager& assets, ScoreManager& scores);
  // Caller retains ownership of the Game*; Launcher does not delete.
  void  addGame(Game* game);
  Game* update(const InputEvent& evt);  // returns active game (nullptr = in menu)
  void  draw();
  void  returnToMenu();

  bool     inGame()           const { return _inGame; }
  uint16_t gameAccentColor(int idx) const;  // RGB565 per-game accent

private:
  TFT_eSPI*     _tft    = nullptr;
  AssetManager* _assets = nullptr;
  ScoreManager* _scores = nullptr;
  Game*         _games[MAX_GAMES] = {};
  int           _count  = 0;
  bool          _inGame = false;
  bool          _paused = false;
  Game*         _active = nullptr;
  int           _activeIdx = -1;         // index of active game (for accent colour)
  bool          _needsRedraw = true;

  void drawHomepage();
  void drawHUD();
  void drawPauseOverlay();
};
#endif

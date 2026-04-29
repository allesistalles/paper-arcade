#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include "../ui/Theme.h"

// Draws the unified game-over overlay (full 240x320 screen).
// Call from game->draw() when _done == true.
// gameColor: per-game RGB565 accent colour (e.g. Theme::SNAKE565)
// won: true shows "YOU WIN!" in green, false shows "GAME OVER" in white
inline void drawGameOverOverlay(TFT_eSPI& s, const char* gameName,
                                uint32_t finalScore, uint16_t gameColor,
                                bool won = false) {
  uint16_t bg  = Theme::BG565;
  uint16_t txt = Theme::TEXT565;
  uint16_t mut = Theme::MUTED565;

  s.fillScreen(bg);

  // Game name in accent colour, font 6
  s.setTextColor(gameColor, bg);
  int nw = s.textWidth(gameName, 6);
  s.drawString(gameName, 120 - nw / 2, 80, 6);

  // "GAME OVER" / "YOU WIN!" in white (or green for wins)
  const char* msg = won ? "YOU WIN!" : "GAME OVER";
  uint16_t msgCol = won ? Theme::SNAKE565 : txt;  // green for wins
  s.setTextColor(msgCol, bg);
  int mw = s.textWidth(msg, 4);
  s.drawString(msg, 120 - mw / 2, 148, 4);

  // "YOUR SCORE" label
  s.setTextColor(mut, bg);
  int yw = s.textWidth("YOUR SCORE", 1);
  s.drawString("YOUR SCORE", 120 - yw / 2, 200, 1);

  // Score value
  char buf[16];
  snprintf(buf, sizeof(buf), "%lu", (unsigned long)finalScore);
  s.setTextColor(txt, bg);
  int sw = s.textWidth(buf, 6);
  s.drawString(buf, 120 - sw / 2, 215, 6);

  // "TAP ANYWHERE"
  s.setTextColor(mut, bg);
  int tw = s.textWidth("TAP ANYWHERE", 1);
  s.drawString("TAP ANYWHERE", 120 - tw / 2, 295, 1);
}
#endif

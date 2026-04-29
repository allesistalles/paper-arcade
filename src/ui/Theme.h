#pragma once
#include <cstdint>

// Minimal Premium palette — OLED black base, electric blue accent.
// 24-bit values are canonical; RGB565 computed with ((r>>3)<<11)|((g>>2)<<5)|(b>>3).
namespace Theme {
  // System colours
  constexpr uint32_t BG     = 0x000000;  // pure OLED black
  constexpr uint32_t TEXT   = 0xFFFFFF;  // sharp white
  constexpr uint32_t ACCENT = 0x0A84FF;  // electric blue
  constexpr uint32_t MUTED  = 0x8E8E93;  // grey — labels, hints, unplayed scores
  constexpr uint32_t CARD   = 0x1C1C1E;  // elevated surface (pause modal)
  constexpr uint32_t SEP    = 0x2C2C2E;  // separator lines
  constexpr uint32_t DANGER = 0xFF453A;  // quit / error red

  // Pre-computed RGB565
  constexpr uint16_t BG565     = 0x0000;
  constexpr uint16_t TEXT565   = 0xFFFF;
  constexpr uint16_t ACCENT565 = 0x0C3F;  // #0A84FF
  constexpr uint16_t MUTED565  = 0x8C72;  // #8E8E93
  constexpr uint16_t CARD565   = 0x18E3;  // #1C1C1E
  constexpr uint16_t SEP565    = 0x2965;  // #2C2C2E
  constexpr uint16_t DANGER565 = 0xFA27;  // #FF453A

  // Per-game accent colours (RGB565)
  constexpr uint16_t SNAKE565    = 0x368B;  // #30D158 green
  constexpr uint16_t PONG565     = 0x0C3F;  // #0A84FF blue
  constexpr uint16_t SIMON565    = 0xFA27;  // #FF453A red
  constexpr uint16_t MINES565    = 0xFCE1;  // #FF9F0A amber
  constexpr uint16_t G2048565    = 0xF9AB;  // #FF375F rose
  constexpr uint16_t BREAKOUT565 = 0x5AFC;  // #5E5CE6 indigo
  constexpr uint16_t FLAPPY565   = 0xFEA1;  // #FFD60A yellow
  constexpr uint16_t TETRIS565   = 0x669F;  // #64D2FF sky
  constexpr uint16_t INVADERS565 = 0xFB4C;  // #FF6961 salmon
  constexpr uint16_t PACMAN565   = 0xBADE;  // #BF5AF2 purple
}

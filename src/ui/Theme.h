#pragma once
#include <cstdint>

namespace Theme {
  // 24-bit RGB (use tft.color24to16() to convert)
  constexpr uint32_t BG        = 0x0d0221;
  constexpr uint32_t ACCENT    = 0xf72585;
  constexpr uint32_t SECONDARY = 0x7209b7;
  constexpr uint32_t TEXT      = 0xffffff;
  constexpr uint32_t DIM       = 0x4cc9f0;
  constexpr uint32_t DARK      = 0x2d0a4e;

  // Pre-computed RGB565
  constexpr uint16_t BG565        = 0x0801;
  constexpr uint16_t ACCENT565    = 0xF94B;
  constexpr uint16_t SECONDARY565 = 0x7016;
  constexpr uint16_t TEXT565      = 0xFFFF;
  constexpr uint16_t DIM565       = 0x4E5E;
  constexpr uint16_t DARK565      = 0x2849;
}

#pragma once
#ifndef NATIVE_TEST
#include <TFT_eSPI.h>
#include <SD.h>
#include <cstring>

// LRU cache of 4 RGB565 BMP sprites loaded from SD.
// Expected BMP: 16-bit (RGB565), uncompressed.
class AssetManager {
public:
  static const int CACHE_SIZE = 4;

  void begin(uint8_t sdCsPin);

  // "snake/apple" → /sprites/snake/apple.bmp
  // Returns nullptr if file missing or invalid.
  const uint16_t* loadBitmap(const char* key, uint16_t& w, uint16_t& h);

  void freeAll();

private:
  struct Slot {
    char     key[32] = {0};
    uint16_t* pixels = nullptr;
    uint16_t  w = 0, h = 0;
    uint32_t  lastUsed = 0;
  };

  Slot     _slots[CACHE_SIZE];
  uint32_t _tick = 0;

  int  findSlot(const char* key);
  int  evictLRU();
  bool loadBmpFromSD(const char* path, Slot& slot);
};
#endif

#include "AssetManager.h"
#ifndef NATIVE_TEST

bool AssetManager::begin(uint8_t sdCsPin, SPIClass& spi) {
  return SD.begin(sdCsPin, spi);
}

int AssetManager::findSlot(const char* key) {
  for (int i = 0; i < CACHE_SIZE; i++)
    if (_slots[i].pixels && strncmp(_slots[i].key, key, 31) == 0) return i;
  return -1;
}

int AssetManager::evictLRU() {
  for (int i = 0; i < CACHE_SIZE; i++)
    if (!_slots[i].pixels) return i;
  int lru = 0;
  for (int i = 1; i < CACHE_SIZE; i++)
    if (_slots[i].lastUsed < _slots[lru].lastUsed) lru = i;
  delete[] _slots[lru].pixels;
  _slots[lru].pixels = nullptr;
  _slots[lru].key[0] = 0;
  return lru;
}

bool AssetManager::loadBmpFromSD(const char* path, Slot& slot) {
  File f = SD.open(path, FILE_READ);
  if (!f) return false;

  uint8_t header[54];
  if (f.read(header, 54) != 54) { f.close(); return false; }
  if (header[0] != 'B' || header[1] != 'M') { f.close(); return false; }

  uint32_t dataOffset, width, height;
  uint16_t bpp;
  memcpy(&dataOffset, header + 10, 4);
  memcpy(&width,      header + 18, 4);
  memcpy(&height,     header + 22, 4);
  memcpy(&bpp,        header + 28, 2);

  if (bpp != 16 || width == 0 || height == 0 || width > 320 || height > 240) { f.close(); return false; }

  uint32_t pixelCount = width * height;
  slot.pixels = new (std::nothrow) uint16_t[pixelCount];
  if (!slot.pixels) { f.close(); return false; }

  slot.w = (uint16_t)width;
  slot.h = (uint16_t)height;

  f.seek(dataOffset);
  // BMP rows are bottom-up
  for (int row = (int)height - 1; row >= 0; row--) {
    if (f.read((uint8_t*)&slot.pixels[row * width], width * 2) != (int)(width * 2)) {
      delete[] slot.pixels;
      slot.pixels = nullptr;
      f.close();
      return false;
    }
  }
  f.close();
  return true;
}

const uint16_t* AssetManager::loadBitmap(const char* key, uint16_t& w, uint16_t& h) {
  int idx = findSlot(key);
  if (idx >= 0) {
    _slots[idx].lastUsed = ++_tick;
    w = _slots[idx].w; h = _slots[idx].h;
    return _slots[idx].pixels;
  }

  idx = evictLRU();
  char path[64];
  snprintf(path, sizeof(path), "/sprites/%s.bmp", key);
  if (!loadBmpFromSD(path, _slots[idx])) {
    w = 0; h = 0;
    return nullptr;
  }
  strncpy(_slots[idx].key, key, 31);
  _slots[idx].key[31] = '\0';
  _slots[idx].lastUsed = ++_tick;
  w = _slots[idx].w; h = _slots[idx].h;
  return _slots[idx].pixels;
}

void AssetManager::freeAll() {
  for (int i = 0; i < CACHE_SIZE; i++) {
    if (_slots[i].pixels) {
      delete[] _slots[i].pixels;
      _slots[i].pixels = nullptr;
      _slots[i].key[0] = 0;
      _slots[i].w = _slots[i].h = 0;
    }
  }
}

#endif

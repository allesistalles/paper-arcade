#include "AssetManager.h"
#ifndef NATIVE_TEST

void AssetManager::begin(uint8_t sdCsPin) {
  SD.begin(sdCsPin);
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

  uint32_t dataOffset = *(uint32_t*)(header + 10);
  uint32_t width      = *(uint32_t*)(header + 18);
  uint32_t height     = *(uint32_t*)(header + 22);
  uint16_t bpp        = *(uint16_t*)(header + 28);

  if (bpp != 16 || width > 320 || height > 240) { f.close(); return false; }

  uint32_t pixelCount = width * height;
  slot.pixels = new (std::nothrow) uint16_t[pixelCount];
  if (!slot.pixels) { f.close(); return false; }

  slot.w = (uint16_t)width;
  slot.h = (uint16_t)height;

  f.seek(dataOffset);
  // BMP rows are bottom-up
  for (int row = (int)height - 1; row >= 0; row--) {
    f.read((uint8_t*)&slot.pixels[row * width], width * 2);
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

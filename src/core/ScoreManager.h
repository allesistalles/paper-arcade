#pragma once
#ifndef NATIVE_TEST
#include <Preferences.h>
#include <cstdint>

class ScoreManager {
public:
  void     begin();
  uint32_t getHighScore(const char* gameName);
  void     setHighScore(const char* gameName, uint32_t score);
  void     end();

private:
  Preferences _prefs;
};
#endif

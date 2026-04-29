#pragma once
#ifndef NATIVE_TEST
#include <Preferences.h>
#include <cstdint>

class ScoreManager {
public:
  void     begin();
  // gameName must be <= 15 characters — ESP32 NVS truncates Preferences keys
  // beyond that, causing collisions across games with similar long names.
  uint32_t getHighScore(const char* gameName);
  void     setHighScore(const char* gameName, uint32_t score);
  void     end();

private:
  Preferences _prefs;
};
#endif

#include "ScoreManager.h"
#ifndef NATIVE_TEST

void ScoreManager::begin() {
  _prefs.begin("arcade", false);
}

uint32_t ScoreManager::getHighScore(const char* gameName) {
  return _prefs.getUInt(gameName, 0);
}

void ScoreManager::setHighScore(const char* gameName, uint32_t score) {
  if (score > getHighScore(gameName))
    _prefs.putUInt(gameName, score);
}

void ScoreManager::end() {
  _prefs.end();
}

#endif

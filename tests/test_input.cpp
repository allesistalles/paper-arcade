#include "test_runner.h"
#include <cstdint>

// Mirror of InputManager::classify — pure logic, no hardware deps.
// 0=NONE, 1=TAP, 2=SWIPE_LEFT, 3=SWIPE_RIGHT, 4=SWIPE_UP, 5=SWIPE_DOWN
int classify(int16_t dx, int16_t dy, uint32_t ms) {
  const int SWIPE_THRESH = 30;
  const int MAX_SWIPE_MS = 500;
  if (ms > MAX_SWIPE_MS) return 0;
  int ax = dx < 0 ? -dx : dx;
  int ay = dy < 0 ? -dy : dy;
  if (ax < SWIPE_THRESH && ay < SWIPE_THRESH) return 1;
  if (ax > ay) return dx < 0 ? 2 : 3;
  return dy < 0 ? 4 : 5;
}

int main() {
  ASSERT_EQ(classify(5, 3, 100), 1);          // small = TAP
  ASSERT_EQ(classify(-5, -3, 200), 1);        // small = TAP
  ASSERT_EQ(classify(-60, 10, 300), 2);       // SWIPE_LEFT
  ASSERT_EQ(classify(60, -5, 200), 3);        // SWIPE_RIGHT
  ASSERT_EQ(classify(5, -50, 250), 4);        // SWIPE_UP
  ASSERT_EQ(classify(3, 55, 300), 5);         // SWIPE_DOWN
  ASSERT_EQ(classify(-80, 0, 600), 0);        // too long = NONE
  ASSERT_EQ(classify(40, 41, 200), 5);        // dy>dx tied → vertical
  TEST_SUMMARY();
}

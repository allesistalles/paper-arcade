#include "test_runner.h"
#include "games/Snake.h"

int main() {
  SnakeLogic s;
  s.init(20, 15);

  ASSERT_EQ(s.length(), 3);
  ASSERT_EQ(s.headX(), 7);
  ASSERT_EQ(s.headY(), 7);

  s.tick();
  ASSERT_EQ(s.headX(), 8);
  ASSERT_EQ(s.headY(), 7);

  s.setDirection(SnakeLogic::UP);
  s.tick();
  ASSERT_EQ(s.headY(), 6);

  // Cannot reverse: pressing DOWN while moving UP keeps moving UP
  s.setDirection(SnakeLogic::DOWN);
  s.tick();
  ASSERT_EQ(s.headY(), 5);

  // Wall collision: snake starts at (7,7) moving RIGHT.
  // setDirection(LEFT) is rejected (would reverse), so direction stays RIGHT.
  // Walk RIGHT until head sits at x=19 (right edge), next tick walks off.
  SnakeLogic s2;
  s2.init(20, 15);
  for (int i = 0; i < 12; i++) ASSERT_TRUE(s2.tick());   // x: 7 → 19
  ASSERT_FALSE(s2.tick());                                // x=20, hits wall

  TEST_SUMMARY();
}

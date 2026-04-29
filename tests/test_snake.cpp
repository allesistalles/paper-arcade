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

  // Wall collision
  SnakeLogic s2;
  s2.init(20, 15);
  s2.setDirection(SnakeLogic::LEFT);
  for (int i = 0; i < 7; i++) s2.tick();   // moves to x=0
  ASSERT_FALSE(s2.tick());                  // next step hits wall

  TEST_SUMMARY();
}

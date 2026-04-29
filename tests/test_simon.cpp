#include "test_runner.h"
#include "games/SimonSays.h"

int main() {
  SimonLogic g;
  g.init(7);

  ASSERT_EQ(g.sequenceLength(), 1);

  uint8_t first = g.sequenceAt(0);
  ASSERT_TRUE(g.checkInput(first));
  ASSERT_TRUE(g.advanceIfRoundComplete());
  ASSERT_EQ(g.sequenceLength(), 2);

  // Wrong input ends game
  SimonLogic g2;
  g2.init(7);
  uint8_t wrong = (g2.sequenceAt(0) + 1) % 4;
  ASSERT_FALSE(g2.checkInput(wrong));
  ASSERT_TRUE(g2.gameOver());

  TEST_SUMMARY();
}

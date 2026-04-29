#include "test_runner.h"
#include "games/Game2048.h"

int main() {
  Game2048Logic g;
  g.initEmpty();
  g.setCell(0,0,2); g.setCell(1,0,2); g.setCell(2,0,0); g.setCell(3,0,0);
  bool moved = g.slide(Game2048Logic::LEFT, false);
  ASSERT_TRUE(moved);
  ASSERT_EQ(g.cell(0,0), 4);
  ASSERT_EQ(g.cell(1,0), 0);
  ASSERT_EQ(g.score(), 4);

  // 2,2,2,2 left → 4,4,0,0
  Game2048Logic g2;
  g2.initEmpty();
  g2.setCell(0,0,2); g2.setCell(1,0,2); g2.setCell(2,0,2); g2.setCell(3,0,2);
  g2.slide(Game2048Logic::LEFT, false);
  ASSERT_EQ(g2.cell(0,0), 4);
  ASSERT_EQ(g2.cell(1,0), 4);
  ASSERT_EQ(g2.cell(2,0), 0);

  // 4,4,2,2 right → 0,0,8,4
  Game2048Logic g3;
  g3.initEmpty();
  g3.setCell(0,0,4); g3.setCell(1,0,4); g3.setCell(2,0,2); g3.setCell(3,0,2);
  g3.slide(Game2048Logic::RIGHT, false);
  ASSERT_EQ(g3.cell(3,0), 4);
  ASSERT_EQ(g3.cell(2,0), 8);
  ASSERT_EQ(g3.cell(1,0), 0);

  TEST_SUMMARY();
}

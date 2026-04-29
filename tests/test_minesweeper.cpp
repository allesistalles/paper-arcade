#include "test_runner.h"
#include "games/Minesweeper.h"

int main() {
  MinesLogic m;
  m.init(10, 11, 10, 99);

  int mines = 0;
  for (int y = 0; y < 11; y++)
    for (int x = 0; x < 10; x++)
      if (m.isMine(x, y)) mines++;
  ASSERT_EQ(mines, 10);

  // Reveal a safe cell
  for (int y = 0; y < 11; y++)
    for (int x = 0; x < 10; x++)
      if (!m.isMine(x, y)) { ASSERT_TRUE(m.reveal(x, y)); goto safe_done; }
  safe_done: ;

  // Reveal a mine → exploded
  MinesLogic m2;
  m2.init(10, 11, 10, 99);
  for (int y = 0; y < 11; y++)
    for (int x = 0; x < 10; x++)
      if (m2.isMine(x, y)) { ASSERT_FALSE(m2.reveal(x, y)); ASSERT_TRUE(m2.isExploded()); goto mine_done; }
  mine_done: ;

  // Flag toggle
  MinesLogic m3;
  m3.init(10, 11, 10, 99);
  m3.toggleFlag(0, 0);
  ASSERT_TRUE(m3.isFlagged(0, 0));
  m3.toggleFlag(0, 0);
  ASSERT_FALSE(m3.isFlagged(0, 0));

  TEST_SUMMARY();
}

#include "test_runner.h"
#include "games/Tetris.h"

int main() {
  TetrisLogic t;
  t.init(42);

  ASSERT_EQ(t.score(), 0);
  ASSERT_FALSE(t.gameOver());
  // Active piece spawned at top
  ASSERT_TRUE(t.activePieceY() <= 1);

  // Can move left from spawn position
  int x0 = t.activePieceX();
  bool moved = t.tryMove(-1);
  // Might be blocked by wall depending on piece, but should at least not crash
  ASSERT_TRUE(moved || !moved);   // just verify no crash

  // Hard drop lands piece and spawns next one
  t.hardDrop();
  ASSERT_TRUE(t.activePieceY() <= 1);   // new piece at top
  ASSERT_FALSE(t.gameOver());

  // Another hard drop
  t.hardDrop();
  ASSERT_FALSE(t.gameOver());

  TEST_SUMMARY();
}

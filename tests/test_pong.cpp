#include "test_runner.h"
#include "games/Pong.h"

int main() {
  PongLogic p;
  p.init(240, 320);

  ASSERT_EQ(p.playerScore(), 0);
  ASSERT_EQ(p.aiScore(), 0);
  // Player paddle starts centered (X position, left edge of 60px paddle)
  ASSERT_TRUE(p.playerX() >= 80 && p.playerX() <= 100);

  // Move player paddle left
  p.setPlayerX(10);
  ASSERT_EQ(p.playerX(), 10);

  // Clamp at left wall
  p.setPlayerX(-50);
  ASSERT_EQ(p.playerX(), 0);

  // Clamp at right wall
  p.setPlayerX(5000);
  ASSERT_EQ(p.playerX(), 240 - PongLogic::PAD_W);

  TEST_SUMMARY();
}

CXX      := g++
CXXFLAGS := -std=c++17 -I src -I tests -DNATIVE_TEST -Wall
BIN      := tests/bin

$(BIN):
	mkdir -p $(BIN)

test_input: tests/test_input.cpp | $(BIN)
	$(CXX) $(CXXFLAGS) tests/test_input.cpp -o $(BIN)/test_input
	$(BIN)/test_input

test_snake: tests/test_snake.cpp src/games/Snake.cpp | $(BIN)
	$(CXX) $(CXXFLAGS) tests/test_snake.cpp src/games/Snake.cpp -o $(BIN)/test_snake
	$(BIN)/test_snake

test_pong: tests/test_pong.cpp src/games/Pong.cpp | $(BIN)
	$(CXX) $(CXXFLAGS) tests/test_pong.cpp src/games/Pong.cpp -o $(BIN)/test_pong
	$(BIN)/test_pong

test_simon: tests/test_simon.cpp src/games/SimonSays.cpp | $(BIN)
	$(CXX) $(CXXFLAGS) tests/test_simon.cpp src/games/SimonSays.cpp -o $(BIN)/test_simon
	$(BIN)/test_simon

test_minesweeper: tests/test_minesweeper.cpp src/games/Minesweeper.cpp | $(BIN)
	$(CXX) $(CXXFLAGS) tests/test_minesweeper.cpp src/games/Minesweeper.cpp -o $(BIN)/test_minesweeper
	$(BIN)/test_minesweeper

test_2048: tests/test_2048.cpp src/games/Game2048.cpp | $(BIN)
	$(CXX) $(CXXFLAGS) tests/test_2048.cpp src/games/Game2048.cpp -o $(BIN)/test_2048
	$(BIN)/test_2048

test_tetris: tests/test_tetris.cpp src/games/Tetris.cpp | $(BIN)
	$(CXX) $(CXXFLAGS) tests/test_tetris.cpp src/games/Tetris.cpp -o $(BIN)/test_tetris
	$(BIN)/test_tetris

test: test_input test_snake test_pong test_simon test_minesweeper test_2048 test_tetris
	@echo "All native tests passed."

.PHONY: test test_input test_snake test_pong test_simon test_minesweeper test_2048 test_tetris

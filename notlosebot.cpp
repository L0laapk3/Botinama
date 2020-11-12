// notlosebot.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "Board.h"
#include "Card.h"
#include "CardBoard.h"
#include <string>
#include <bitset>



std::vector<Board> boards3;
std::vector<Board> boards4;
void recurse(GameCards& gameCards, Board& board, int depth, unsigned long long& count) {
	board.valid(gameCards);
	std::vector<Board> boards;
	bool first = true;
	board.forwardMoves(gameCards, [&](Board board) {
		boards.push_back(board);
		if (depth > 1 && !board.finished())
			recurse(gameCards, board, depth - 1, count);
		else
			count++;
	});
	//Board::print(gameCards, boards);
}

int main() {
	GameCards gameCards = CardBoard::fetchGameCards({ "ox", "boar", "horse", "elephant", "crab" });
	Board board = Board("1121100000000000000033433");
	//Board b = Board("0000001000001000000000000");
	board.print(gameCards);

	for (int depth = 1; depth < 9; depth++) {
		unsigned long long count = 0;
		recurse(gameCards, board, depth, count);
		std::cout << depth << " " << count << std::endl;
	}
	Board::print(gameCards, boards3);
	Board::print(gameCards, boards4);
}
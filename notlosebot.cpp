// notlosebot.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "Board.h"
#include "Card.h"
#include "CardBoard.h"
#include <string>



void recurse(GameCards& gameCards, Board& board, int depth, unsigned long long& count) {
	std::vector<Board> boards;
	board.valid(gameCards);
	board.forwardMoves(gameCards, [&](Board newBoard) {
		boards.push_back(newBoard);
		if (depth > 1 && !board.finished())
			recurse(gameCards, newBoard, depth - 1, count);
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
	
	for (int depth = 1; depth < 7; depth++) {
		unsigned long long count = 0;
		recurse(gameCards, board, depth, count);
		std::cout << depth << " " << count << std::endl;
	}
}
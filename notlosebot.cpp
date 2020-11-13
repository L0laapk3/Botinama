// notlosebot.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "Board.h"
#include "Card.h"
#include "CardBoard.h"
#include <string>
#include <bitset>
#include <chrono>
#include <algorithm>


unsigned long long count = 0;
void recursive(GameCards& gameCards, const Board& board, int depth) {
	if (board.finished()) {
		count++;
		return;
	}
	const Moves moves = board.forwardMoves(gameCards);
	if (depth > 1)
		for (auto i = moves.outputs.begin(); i < moves.end; i++) {
			recursive(gameCards, *i, depth - 1);
		}
	else
		count += moves.end - moves.outputs.begin();
}

int main() {
	GameCards gameCards = CardBoard::fetchGameCards({ "ox", "boar", "horse", "elephant", "crab" });
	Board board = Board::fromString("1121100000000000000033433");
	board.print(gameCards);

	for (int depth = 1; depth <= 10; depth++) {
		auto start = std::chrono::steady_clock::now();
		count = 0;
		recursive(gameCards, board, depth);
		auto end = std::chrono::steady_clock::now();
		float nps = std::roundf(count / (std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() * .001f));
		std::cout << depth << '\t' << nps << "M/s\t" << count << std::endl;
	}
}
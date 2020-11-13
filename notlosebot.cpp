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


unsigned long long perft(GameCards& gameCards, const Board& board, int depth) {
	if (!depth || board.finished())
		return 1;
	const Moves moves = board.forwardMoves(gameCards);
	auto total = 0ULL;
	for (auto i = moves.outputs.begin(); i < moves.end; i++)
		total += perft(gameCards, *i, depth - 1);
	return total;
}

int main() {
	GameCards gameCards = CardBoard::fetchGameCards({ "ox", "boar", "horse", "elephant", "crab" });
	Board board = Board::fromString("1121100000000000000033433");
	board.print(gameCards);

	for (int depth = 1; depth <= 10; depth++) {
		auto start = std::chrono::steady_clock::now();
		const auto count = perft(gameCards, board, depth);
		auto end = std::chrono::steady_clock::now();
		float nps = std::roundf(count / (std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() * .001f));
		std::cout << depth << '\t' << nps << "M/s\t" << count << std::endl;
	}
}
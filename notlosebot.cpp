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
//-Wbitwise-op-parentheses -march=native 


void recursiveF(GameCards& gameCards, const Board<false>& board, const bool finished, unsigned long long depth);
void recursiveT(GameCards& gameCards, const Board<true>& board, const bool finished, unsigned long long depth) {
	if (!depth || finished)
		count++;
	else
		board.forwardMoves<*(MoveFunc<false>)recursiveF>(gameCards, depth - 1);
}
void recursiveF(GameCards& gameCards, const Board<false>& board, const bool finished, unsigned long long depth) {
	if (!depth || finished)
		count++;
	else
		board.forwardMoves<*(MoveFunc<true>)recursiveT>(gameCards, depth - 1);
}

int main() {
	GameCards gameCards = CardBoard::fetchGameCards({ "ox", "boar", "horse", "elephant", "crab" });
	Board board = Board<false>::fromString("1121100000000000000033433");
	board.print(gameCards);

	for (int depth = 1; depth <= 10; depth++) {
		auto start = std::chrono::steady_clock::now();
		count = 0;
		recursiveF(gameCards, board, false, depth);
		auto end = std::chrono::steady_clock::now();
		float nps = std::roundf(count / (std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() * .001f));
		std::cout << depth << '\t' << nps << "M/s\t" << count << std::endl;
	}
}
// notlosebot.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>

#include "Board.h"
#include "Card.h"
#include "CardBoard.h"
#include <string>

int main() {
	std::array<const CardBoard, 5> cardStack{ CARDS[0], CARDS[1], CARDS[2], CARDS[15], CARDS[4] };
	Board b = Board("1121100000000000000033433");
	//Board b = Board("0000001000001000000000000");
	b.print(cardStack);
	std::vector<Board> boards;
	b.forwardMoves(cardStack, [&](Board board) {
		boards.push_back(board);
	});
	Board::print(cardStack, boards);

	CardBoard cb(CARDS[1]);
}


#include "Board.h"

#include <bitset>
#include <iostream>
#include <algorithm>




Board Board::fromString(std::string str) {
	Board board{ 0 };
	bool blueKingFound = false;
	bool redKingFound = false;
	for (int i = 0; i < 25; i++) {
		if (str[i] == '1' || str[i] == '2') {
			board.pieces |= 1ULL << i;
			if (str[i] == '2')
				blueKingFound = true;
			if (!blueKingFound)
				board.pieces += (1ULL << INDEX_KINGS[0]);
		}
		if (str[i] == '3' || str[i] == '4') {
			board.pieces |= 1ULL << (i + 32);
			if (str[i] == '4')
				redKingFound = true;
			if (!redKingFound)
				board.pieces += (1ULL << INDEX_KINGS[1]);
		}
	}
	if (false) // red starts
		board.pieces |= MASK_TURN;
	return board;
}



bool Board::finished() const {
	return pieces & MASK_FINISH;
}
bool Board::winner() const {
	return !(pieces & MASK_TURN);
}
void Board::valid(GameCards& gameCards) const {
	assert((pieces & (pieces >> 32) & MASK_PIECES) == 0); // overlapping pieces

	//assert((kings & MASK_PIECES & ~(pieces | (pieces >> 32))) == 0); //loose king

	//assert(_popcnt64(kings) == 2); // wrong amount of kings

	assert((((pieces >> 32) & pieces & MASK_CARDS) >> 27) == 0); // overlapping cards

	for (int i = 0; i < 2; i++) {
		uint32_t cards = (pieces >> (32 * i)) & MASK_CARDS;
		assert(_popcnt64(cards) == 2); // not exactly 2 cards
	}
}


std::string cardsShortName(std::array<const CardBoard, 5>& gameCards, int i, int length) {
	std::string res = "";
	const std::string& card = i < 5 ? gameCards[i].name : (std::to_string(i) + "??");
	for (uint32_t i = 0; i < length; i++)
		res += card.size() > i ? card[i] : ' ';
	res += ' ';
	return res;
}
void Board::print(GameCards& gameCards) const {
	Board::print(gameCards, { *this });
}
void Board::print(GameCards& gameCards, std::vector<Board> boards) {
	constexpr size_t MAXPERLINE = 10;
	for (size_t batch = 0; batch < boards.size(); batch += MAXPERLINE) {
		std::array<int, MAXPERLINE> blueKingPos;
		std::array<int, MAXPERLINE> redKingPos;
		std::array<CardsPos, MAXPERLINE> cards;
		for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
			const Board& board = boards[i];
			blueKingPos[i] = _popcnt32(board.pieces & MASK_PIECES) - 1 - ((board.pieces >> INDEX_KINGS[0]) & 7);
			redKingPos[i] = _popcnt32((board.pieces >> 32) & MASK_PIECES) - 1 - (board.pieces >> INDEX_KINGS[1]) & 7;
			std::cout << ((board.pieces >> INDEX_KINGS[0]) & 7) << std::endl;
			std::cout << ((board.pieces >> INDEX_KINGS[1]) & 7) << std::endl;
			cards[i] = CARDS_LUT[(board.pieces & MASK_CARDS) >> 27ULL];
			std::cout << cardsShortName(gameCards, cards[i].players[0] & 0xff, 4) << ' ' << cardsShortName(gameCards, (cards[i].players[0] >> 16) & 0xff, 4) << ' ';
		}
		std::cout << std::endl;
		for (int r = 5; r--> 0;) {
			for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
				const Board& board = boards[i];
				std::string end = board.finished() ? "END " : "    ";
				end += board.pieces & MASK_TURN ? '+' : 'o';
				const auto swapCardName = cardsShortName(gameCards, cards[i].side, 5);
				std::cout << end[4ULL - r] << '|';
				std::string str = "";
				for (int c = 5; c--> 0;) {
					const int mask = 1 << (5 * r + c);
					if (board.pieces & (board.pieces >> 32) & mask)
						str = '?' + str;
					else if (board.pieces & mask)
						str = (!blueKingPos[i]-- ? '0' : 'o') + str; // bloo
					else if ((board.pieces >> 32) & mask)
						str = (!redKingPos[i]-- ? 'X' : '+') + str; // +ed
					else
						str = ' ' + str;
				}
				std::cout << str;
				if (swapCardName.size() < 5)
					std::cout << "|   ";
				else
					std::cout << '|' << swapCardName[4ULL - r] << "  ";
			}
			std::cout << std::endl;
		}
		for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
			const Board& board = boards[i];
			std::cout << cardsShortName(gameCards, cards[i].players[1] & 0xff, 4) << ' ' << cardsShortName(gameCards, (cards[i].players[1] >> 16) & 0xff, 4) << ' ';
		}
		std::cout << std::endl;
		for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
			const Board& board = boards[i];
			board.valid(gameCards);
		}
	}
}

void printKings(uint64_t pieces) {
	std::cout << std::bitset<3>(pieces >> INDEX_KINGS[0]) << ' ' << std::bitset<3>(pieces >> INDEX_KINGS[1]) << std::endl;
}
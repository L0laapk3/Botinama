
#include "Board.h"
#include "Botama.h"

#include <bitset>
#include <iostream>
#include <algorithm>




Board Board::fromString(std::string str, bool player, bool flip) {
	Board board{ 0 };
	bool blueKingFound = false;
	bool redKingFound = false;
	for (int i = 0; i < 25; i++) {
		const auto c = str[flip ? 24ULL - i : i];
		if (c == '1' || c == '2') {
			board.pieces |= 1ULL << (i + 32 * flip);
			if (c == '2')
				blueKingFound = true;
			if (!blueKingFound)
				board.pieces += (1ULL << INDEX_KINGS[flip]);
		}
		if (c == '3' || c == '4') {
			board.pieces |= 1ULL << (i + 32 - 32 * flip);
			if (c == '4')
				redKingFound = true;
			if (!redKingFound)
				board.pieces += (1ULL << INDEX_KINGS[!flip]);
		}
	}
	if (player) // red starts
		board.pieces |= MASK_TURN;
	return board;
}



bool Board::winner() const {
	return !(pieces & MASK_TURN);
}

bool Board::currentPlayer() const {
	return pieces & MASK_TURN;
}

Board Board::invert() const {
	Board board{ 0 };
	U64 source = _rotr64(pieces, 32);
	for (int i = 0; i < 25; i++) {
		board.pieces <<= 1;
		board.pieces |= source & (1 | (1ULL << 32));
		source >>= 1;
	}
	board.pieces |= (_popcnt32(pieces & MASK_PIECES) - 1 - ((pieces >> INDEX_KINGS[0]) & 0x7)) << INDEX_KINGS[1];
	board.pieces |= (_popcnt32((pieces >> 32) & MASK_PIECES) - 1 - ((pieces >> INDEX_KINGS[1]) & 0x7)) << INDEX_KINGS[0];
	board.pieces |= ((U64)CARDS_INVERT[(pieces >> INDEX_CARDS) & 0x1f]) << INDEX_CARDS;
	board.pieces |= (pieces & MASK_TURN) ^ MASK_TURN;
	//std::cout << "invert" << std::endl << std::bitset<64>(pieces) << std::endl << std::bitset<64>(board.pieces) << std::endl;
	return board;
}

#undef NDEBUG
#include <assert.h>
void Board::valid() const {
	return;
	assert((pieces & (pieces >> 32) & MASK_PIECES) == 0); // overlapping pieces

	assert(((pieces >> INDEX_KINGS[0]) & 7) <= _popcnt32(pieces & MASK_PIECES)); //loose blue king
	assert(((pieces >> INDEX_KINGS[1]) & 7) <= _popcnt32((pieces >> 32) & MASK_PIECES)); //loose red king

	assert(((pieces & MASK_CARDS) >> INDEX_CARDS) < 30); //illegal card LUT index
}

std::string cardsShortName(const GameCards& gameCards, int i, int length) {
	std::string res = "";
	const std::string& card = i < 5 ? gameCards[i].name : (std::to_string(i) + "??");
	for (U32 i = 0; i < length; i++)
		res += card.size() > i ? card[i] : ' ';
	res += ' ';
	return res;
}
void Board::print(const GameCards& gameCards, bool finished, const bool verbose) const {
	Board::print(gameCards, { *this }, { finished }, verbose);
}
void Board::print(const GameCards& gameCards, std::vector<Board> boards, std::vector<bool> finished, const bool verbose) {
	constexpr size_t MAXPERLINE = 10;
	for (size_t batch = 0; batch < boards.size(); batch += MAXPERLINE) {
		std::array<U32, MAXPERLINE> blueKingPos{ 0 };
		std::array<U32, MAXPERLINE> redKingPos{ 0 };
		std::array<CardsPos, MAXPERLINE> cards{ 0 };
		if (verbose)
			for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
				const Board& board = boards[i];
				std::cout << std::bitset<7>(board.pieces >> 32 >> 25) << ' ' << std::bitset<25>(board.pieces >> 32) << ' ' << std::bitset<7>(board.pieces >> 25) << ' ' << std::bitset<25>(board.pieces) << std::endl;
			}
		for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
			const Board& board = boards[i];
			blueKingPos[i] = _popcnt32(board.pieces & MASK_PIECES) - 1 - ((board.pieces >> INDEX_KINGS[0]) & 7);
			redKingPos[i] = _popcnt32((board.pieces >> 32) & MASK_PIECES) - 1 - (board.pieces >> INDEX_KINGS[1]) & 7;
			cards[i] = CARDS_LUT[(board.pieces & MASK_CARDS) >> 27ULL];
			std::cout << cardsShortName(gameCards, cards[i].players[1] & 0xff, 4) << cardsShortName(gameCards, (cards[i].players[1] >> 16) & 0xff, 4) << ' ';
		}
		std::cout << std::endl;
		for (int r = 5; r-- > 0;) {
			for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
				const Board& board = boards[i];
				std::string end = finished[i] ? "END " : "    ";
				end += board.pieces & MASK_TURN ? '+' : 'o';
				const auto swapCardName = cardsShortName(gameCards, cards[i].side, 5);
				std::cout << end[4ULL - r] << '|';
				std::string str = "";
				for (int c = 5; c-- > 0;) {
					const int mask = 1 << (5 * r + c);
					if (board.pieces & (board.pieces >> 32) & mask)
						str = '?' + str;
					else if (board.pieces & mask)
						str = (!blueKingPos[i]-- ? '0' : 'o') + str; // bloo
					else if ((board.pieces >> 32) & mask)
						str = (!redKingPos[i]-- ? 'X' : '+') + str; // r+d
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
			//const Board& board = boards[i];
			std::cout << cardsShortName(gameCards, cards[i].players[0] & 0xff, 4) << cardsShortName(gameCards, (cards[i].players[0] >> 16) & 0xff, 4) << ' ';
		}
		std::cout << std::endl;
		for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
			const Board& board = boards[i];
			board.valid();
		}
	}
}
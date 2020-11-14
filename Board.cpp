
#include "Board.h"

#include <bitset>
#include <iostream>
#include <algorithm>




Board Board::fromString(std::string str) {
	Board board{
		(0b00011ULL << 27) | (0b01100ULL << (27 + 32)),
		0
	};
	for (int i = 0; i < 25; i++) {
		if (str[i] == '1' || str[i] == '2')
			board.pieces |= 1ULL << i;
		if (str[i] == '3' || str[i] == '4')
			board.pieces |= 1ULL << (i + 32);
		if (str[i] == '2' || str[i] == '4')
			board.kings |= 1ULL << i;
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

	assert((kings & MASK_PIECES & ~(pieces | (pieces >> 32))) == 0); //loose king

	assert(_popcnt64(kings) == 2); // wrong amount of kings

	assert((((pieces >> 32) & pieces & MASK_CARDS) >> 27) == 0); // overlapping cards

	for (int i = 0; i < 2; i++) {
		uint32_t cards = (pieces >> (32 * i)) & MASK_CARDS;
		assert(_popcnt64(cards) == 2); // not exactly 2 cards
	}
}


std::string cardsShortName(std::array<const CardBoard, 5>& gameCards, uint32_t bits, unsigned long long length) {
	bits &= 0xff;
	unsigned long i;
	std::string res = "";
	while (_BitScanForward(&i, bits)) {
		bits &= ~(1 << i);
		const std::string& card = i < 5 ? gameCards[i].name : (std::to_string(i) + "??");
		for (uint32_t i = 0; i < length; i++)
			res += card.size() > i ? card[i] : ' ';
		res += ' ';
	}
	return res;
}
void Board::print(GameCards& gameCards) const {
	Board::print(gameCards, { *this });
}
void Board::print(GameCards& gameCards, std::vector<Board> boards) {
	constexpr size_t MAXPERLINE = 10;
	for (size_t batch = 0; batch < boards.size(); batch += MAXPERLINE) {
		for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
			const Board& board = boards[i];
			std::cout << cardsShortName(gameCards, (board.pieces >> 27) & 0x1f, 4) << ' ';
		}
		std::cout << std::endl;
		for (int r = 5; r-- > 0;) {
			for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
				const Board& board = boards[i];
				std::string end = board.finished() ? "END " : "    ";
				end += board.pieces & MASK_TURN ? '+' : 'o';
				const auto swapCardName = cardsShortName(gameCards, ~(((board.pieces | (board.pieces >> 32)) >> 27) & 0x1f), 5);
				std::cout << end[4ULL - r] << '|';
				for (int c = 0; c < 5; c++) {
					const int mask = 1 << (5 * r + c);
					if (board.pieces & (board.pieces >> 32) & mask)
						std::cout << ((board.kings & mask) ? 'e' : '?');
					else if (board.pieces & mask)
						std::cout << ((board.kings & mask) ? '0' : 'o'); // bloo
					else if ((board.pieces >> 32) & mask)
						std::cout << ((board.kings & mask) ? 'X' : '+'); // +ed
					else
						std::cout << ((board.kings & mask) ? '!' : ' ');
				}
				if (swapCardName.size() < 5)
					std::cout << "|   ";
				else
					std::cout << '|' << swapCardName[4ULL - r] << "  ";
			}
			std::cout << std::endl;
		}
		for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
			const Board& board = boards[i];
			std::cout << cardsShortName(gameCards, board.pieces >> (27 + 32), 4) << ' ';
		}
		std::cout << std::endl;
		for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
			const Board& board = boards[i];
			board.valid(gameCards);
		}
	}
}

void printCards(std::vector<uint32_t> cardsVec) {
	for (auto& cards : cardsVec)
		std::cout << std::bitset<5>(cards >> 16) << ' ' << std::bitset<8>(cards >> 8) << ' ' << std::bitset<8>(cards) << "  ";
	std::cout << std::endl;
}

#include "Board.h"

#include <bitset>
#include <iostream>
#include <intrin.h>
#include <algorithm>

#pragma intrinsic(_BitScanForward)


constexpr uint32_t MASK_TURN = 1 << 25;
constexpr uint32_t MASK_FINISH = 1 << 26;
constexpr uint32_t MASK_CARDS = 0x1f << 27;

constexpr std::array<uint32_t, 2> END_POSITIONS = {
	0b00100 << 20,
	0b00100
};


Board Board::fromString(std::string str) {
	Board board;
	board.pieces[0] = 0b00011 << 27;
	board.pieces[1] = 0b01100 << 27;
	board.kings = 0;
	for (int i = 0; i < 25; i++) {
		if (str[i] == '1' || str[i] == '2')
			board.pieces[0] |= 1 << i;
		if (str[i] == '3' || str[i] == '4')
			board.pieces[1] |= 1 << i;
		if (str[i] == '2' || str[i] == '4')
			board.kings |= 1 << i;
	}
	if (false) // red starts
		board.pieces[0] |= MASK_TURN;
	return board;
}



bool Board::finished() const {
	return pieces[0] & MASK_FINISH;
}
bool Board::winner() const {
	return !(pieces[0] & MASK_TURN);
}
void Board::valid(GameCards& gameCards) const {
	auto doublePiece = pieces[0] & pieces[1] & 0x1fffffff;
	auto kingWithoutPiece = kings & 0x1fffffff & ~(pieces[0] | pieces[1]);
	if (doublePiece || kingWithoutPiece) {
		//print(gameCards);
		std::cout << "invalid board";
		throw "invalid board";
	}
	uint32_t cards0 = pieces[0] >> 27;
	uint32_t cards1 = pieces[1] >> 27;
	if (cards0 & cards1) {
		std::cout << "card overlap";
		throw "card overlap";
	}
	for (int i = 0; i < 2; i++) {
		uint32_t cards = pieces[i] >> 27;
		if (!cards) {
			std::cout << "card missing";
			throw "card missing";
		}
		cards &= ~(cards & -cards);
		if (!cards) {
			std::cout << "card missing";
			throw "card missing";
		}
		cards &= ~(cards & -cards);
		if (cards) {
			std::cout << "too many cards";
			throw "too many cards";
		}
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
			std::cout << cardsShortName(gameCards, board.pieces[0] >> 27, 4) << ' ';
		}
		std::cout << std::endl;
		for (int r = 5; r-- > 0;) {
			for (size_t i = batch; i < std::min(batch + MAXPERLINE, boards.size()); i++) {
				const Board& board = boards[i];
				std::string end = board.finished() ? "END " : "    ";
				end += board.pieces[0] & MASK_TURN ? '+' : 'o';
				const auto swapCardName = cardsShortName(gameCards, ~(board.pieces[0] | board.pieces[1]) >> 27, 5);
				std::cout << end[4ULL - r] << '|';
				for (int c = 0; c < 5; c++) {
					const int mask = 1 << (5 * r + c);
					if (board.pieces[0] & board.pieces[1] & mask)
						std::cout << ((board.kings & mask) ? 'e' : '?');
					else if (board.pieces[0] & mask)
						std::cout << ((board.kings & mask) ? '0' : 'o'); // bloo
					else if (board.pieces[1] & mask)
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
			std::cout << cardsShortName(gameCards, board.pieces[1] >> 27, 4) << ' ';
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

void Board::iterateMoves(Moves& moves, const CardBoard& card, uint32_t playerPieces, bool player, bool movingPlayer) const {
	//card.print();
	unsigned long from;
	uint32_t bitScan = pieces[movingPlayer] & 0x1ffffff;
	while (_BitScanForward(&from, bitScan)) {
		bitScan &= ~(1 << from);
		std::array<uint32_t, 2> boardsWithoutPiece{ pieces };
		boardsWithoutPiece[movingPlayer] = playerPieces & ~(1 << from);
		boardsWithoutPiece[0] = (boardsWithoutPiece[0] & ~MASK_TURN) | ((!player) << 25);
		uint32_t kingsWithoutPiece = kings & ~(1 << from);
		uint32_t isMovingKing = kingsWithoutPiece == kings ? 0 : ~0;
		uint32_t scan = card.moveBoard[player][from] & ~pieces[movingPlayer] & 0x1fffffff;
		uint32_t endMask = (END_POSITIONS[movingPlayer] & isMovingKing) | kings; // to be &'d with nextBit. kind lands on temple | piece takes king
		while (scan) {
			uint32_t nextBit = scan & -scan;
			scan &= ~nextBit;
			moves.end->pieces[movingPlayer] = boardsWithoutPiece[movingPlayer] | nextBit;
			moves.end->pieces[!movingPlayer] = boardsWithoutPiece[!movingPlayer] & ~nextBit;
			//const bool finished = nextBit & endMask;
			moves.end->kings = kingsWithoutPiece | (nextBit & isMovingKing);
			moves.end++;
		}
	}
}

Moves Board::forwardMoves(GameCards& gameCards) const {
	Moves out;
	bool player = pieces[0] & MASK_TURN;
	uint32_t cardScan = pieces[player] & MASK_CARDS;
	uint32_t playerPiecesWithNew = pieces[player] | (MASK_CARDS & ~pieces[!player]);
	for (int i = 0; i < 2; i++) {
		unsigned long cardI;
		_BitScanForward(&cardI, cardScan);
		cardScan &= ~(1ULL << cardI);
		uint32_t playerPieces = playerPiecesWithNew & ~(1ULL << cardI);
		const auto& card = gameCards[cardI - 27];
		iterateMoves(out, card, playerPieces, player, player);
	}
	return out;
}

Moves Board::reverseMoves(GameCards& gameCards) const {
	Moves out;
	/*
	bool player = pieces[0] & MASK_TURN;
	unsigned long swapCardI;
	_BitScanForward(&swapCardI, cards & CARDS_SWAPMASK);
	uint32_t cardScan = cards & CARDS_PLAYERMASK[!player];
	unsigned long playerCardI;
	_BitScanForward(&playerCardI, cardScan);
	uint32_t swapCardPlayerMask = (cards & CARDS_SWAPMASK) >> (!player ? 8 : 16);
	uint32_t firstCard = 1ULL << playerCardI;
	uint32_t secondCard = cards & CARDS_PLAYERMASK[!player] & ~firstCard;
	uint32_t newCards = (cards & 0xffffULL) | swapCardPlayerMask;
	iterateMoves(out, gameCards[swapCardI & 7], (newCards & ~firstCard) | (firstCard << (!player ? 8 : 16)), player, !player);
	iterateMoves(out, gameCards[swapCardI & 7], (newCards & ~secondCard) | (secondCard << (!player ? 8 : 16)), player, !player);*/
	return out;
}
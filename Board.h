#pragma once

#include <string>
#include <array>
#include <list>
#include "CardBoard.h"
#include <functional>
#include <cassert>
#include "Bitscan.h"

constexpr uint64_t MASK_TURN = 1ULL << 25;
constexpr uint64_t MASK_FINISH = 1ULL << 26;
constexpr uint64_t MASK_CARDS = 0x1fULL << 27;
constexpr uint64_t MASK_PIECES = 0x1ffffffULL;
constexpr std::array<uint64_t, 2> MASK_PLAYER = { 0xffff'ffffULL, 0xffff'ffff'0000'0000 };
constexpr std::array<uint32_t, 2> MASK_END_POSITIONS = { 0b00100ULL << 20, 0b00100ULL };
struct CardsPos {
	std::array<uint32_t, 2> players;
	uint32_t side;
	static CardsPos construct(uint32_t blue0, uint32_t blue1, uint32_t red0, uint32_t red1, uint32_t side, uint32_t swapBlue0, uint32_t swapBlue1, uint32_t swapRed0, uint32_t swapRed1) {
		return CardsPos{
			{
				(blue0 - 1) | (swapBlue0 << 8) | ((blue1 - 1) << 16) | (swapBlue1 << 24),
				(red0 - 1) | (swapRed0 << 8) | ((red1 - 1) << 16) | (swapRed1 << 24)
			},
			side
		};
	};
};
const std::array<CardsPos, 30> CARDS_LUT = {{
	CardsPos::construct(1, 2, 3, 4, 5,	26,	20,	12,	6),
	CardsPos::construct(1, 3, 2, 4, 5,	28,	14,	18,	7),
	CardsPos::construct(1, 4, 2, 3, 5,	29,	8,	19,	13),
	CardsPos::construct(2, 3, 1, 4, 5,	22,	16,	24,	9),
	CardsPos::construct(2, 4, 1, 3, 5,	23,	10,	25,	15),
	CardsPos::construct(3, 4, 1, 2, 5,	17,	11,	27,	21),
	CardsPos::construct(1, 2, 3, 5, 4,	25,	19,	12,	0),
	CardsPos::construct(1, 3, 2, 5, 4,	27,	13,	18,	1),
	CardsPos::construct(1, 5, 2, 3, 4,	29,	2,	20,	14),
	CardsPos::construct(2, 3, 1, 5, 4,	21,	15,	24,	3),
	CardsPos::construct(2, 5, 1, 3, 4,	23,	4,	26,	16),
	CardsPos::construct(3, 5, 1, 2, 4,	17,	5,	28,	22),
	CardsPos::construct(1, 2, 4, 5, 3,	24,	18,	6,	0),
	CardsPos::construct(1, 4, 2, 5, 3,	27,	7,	19,	2),
	CardsPos::construct(1, 5, 2, 4, 3,	28,	1,	20,	8),
	CardsPos::construct(2, 4, 1, 5, 3,	21,	9,	25,	4),
	CardsPos::construct(2, 5, 1, 4, 3,	22,	3,	26,	10),
	CardsPos::construct(4, 5, 1, 2, 3,	11,	5,	29,	23),
	CardsPos::construct(1, 3, 4, 5, 2,	24,	12,	7,	1),
	CardsPos::construct(1, 4, 3, 5, 2,	25,	6,	13,	2),
	CardsPos::construct(1, 5, 3, 4, 2,	26,	0,	14,	8),
	CardsPos::construct(3, 4, 1, 5, 2,	15,	9,	27,	5),
	CardsPos::construct(3, 5, 1, 4, 2,	16,	3,	28,	11),
	CardsPos::construct(4, 5, 1, 3, 2,	10,	4,	29,	17),
	CardsPos::construct(2, 3, 4, 5, 1,	18,	12,	9,	3),
	CardsPos::construct(2, 4, 3, 5, 1,	19,	6,	15,	4),
	CardsPos::construct(2, 5, 3, 4, 1,	20,	0,	16,	10),
	CardsPos::construct(3, 4, 2, 5, 1,	13,	7,	21,	5),
	CardsPos::construct(3, 5, 2, 4, 1,	14,	1,	22,	11),
	CardsPos::construct(4, 5, 2, 3, 1,	8,	2,	23,	17),
}};

class Board;
typedef void (*MoveFunc)(GameCards& gameCards, const Board& board, const bool finished, unsigned long long depth);

class Board {
public:
	uint64_t pieces;
	uint32_t kings;

	static Board fromString(std::string str);

	void print(GameCards& gameCards) const;
	static void print(GameCards& gameCards, std::vector<Board> board);

	void valid(GameCards& gameCards) const;
	bool finished() const;
	bool winner() const;

private:
	template<MoveFunc cb>
	void iterateMoves(GameCards& gameCards, const CardBoard& card, uint64_t piecesWithNewCards, bool player, bool movingPlayer, unsigned long long depth) const {
		//card.print();
		unsigned long fromI;
		uint64_t bitScan = piecesWithNewCards & (movingPlayer ? MASK_PIECES << 32 : MASK_PIECES);
		while (_BitScanForward64(&fromI, bitScan)) {
			bitScan &= ~(1ULL << fromI);
			uint64_t newPiecesWithoutLandPiece = piecesWithNewCards & ~(1ULL << fromI);
			uint32_t kingsWithoutPiece = kings & ~(1ULL << (movingPlayer ? fromI - 32ULL : fromI));
			bool isKingMove = kingsWithoutPiece != kings;
			uint32_t scan = card.moveBoard[player][movingPlayer ? fromI - 32ULL : fromI] & ~(movingPlayer ? pieces >> 32 : pieces);
			uint64_t endMask = (isKingMove ? MASK_END_POSITIONS[movingPlayer] : 0) | kings; // to be &'d with nextBit. kind lands on temple | piece takes king
			while (scan) {
				uint32_t landBit = scan & -scan;
				scan &= ~landBit;
				uint64_t landBitHigh = ((uint64_t)landBit) << 32;
				Board board;
				board.pieces = (newPiecesWithoutLandPiece | (movingPlayer ? landBitHigh : landBit)) & ~(movingPlayer ? landBit : landBitHigh); // add land piece and remove taken piece
				board.kings = kingsWithoutPiece | (isKingMove ? landBit : 0);
				const bool finished = landBit & endMask;
				cb(gameCards, board, finished, depth);
			}
		}
	}
public:
	template<MoveFunc cb>
	void forwardMoves(GameCards& gameCards, unsigned long long depth) const {
		bool player = pieces & MASK_TURN;
		const CardsPos& cardsPos = CARDS_LUT[(pieces &  MASK_CARDS) >> 27ULL];
		uint64_t playerPiecesWithoutCards = pieces & ~MASK_CARDS;
		playerPiecesWithoutCards ^= MASK_TURN; // invert player bit
		uint32_t cardStuff = cardsPos.players[player];
		for (int i = 0; i < 2; i++) {
			unsigned long cardI = cardStuff & 0xff;
			uint64_t piecesWithNewCards = playerPiecesWithoutCards | ((cardStuff & 0xff00) << (27ULL - 8ULL));
			cardStuff >>= 16;
			const auto& card = gameCards[cardI];
			iterateMoves<cb>(gameCards, card, piecesWithNewCards, player, player, depth);
		}
	}
	/*
	template<MoveFunc cb>
	void reverseMoves(GameCards& gameCards, unsigned long long depth) const {
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
		iterateMoves(gameCards[swapCardI & 7], (newCards & ~firstCard) | (firstCard << (!player ? 8 : 16)), player, !player, cb);
		iterateMoves(gameCards[swapCardI & 7], (newCards & ~secondCard) | (secondCard << (!player ? 8 : 16)), player, !player, cb);
	}*/

};
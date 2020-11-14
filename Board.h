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
		uint64_t cardScan = pieces & (player ? MASK_CARDS << 32 : MASK_CARDS);
		uint64_t playerPiecesWithReceivedCard = pieces | (player ? (MASK_CARDS & ~pieces) << 32 : ((MASK_CARDS << 32) & ~pieces) >> 32);
		playerPiecesWithReceivedCard ^= MASK_TURN; // invert player bit
		for (int i = 0; i < 2; i++) {
			unsigned long cardI;
			bool found = _BitScanForward64(&cardI, cardScan);
			assert(found);
			const uint64_t cardBit = ((uint64_t)1) << cardI;
			cardScan &= ~cardBit;
			uint64_t piecesWithNewCards = playerPiecesWithReceivedCard & ~cardBit;
			const auto& card = gameCards[player ? cardI - 27ULL - 32ULL : cardI - 27ULL];
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
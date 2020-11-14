#pragma once

#include <string>
#include <array>
#include <list>
#include "CardBoard.h"
#include <functional>
#include <bitset>
#include <cassert>
#include <iostream>
#include "Bitscan.h"

constexpr uint64_t MASK_TURN = 1ULL << 25;
constexpr uint64_t MASK_FINISH = 1ULL << 26;
constexpr uint64_t MASK_CARDS = 0x1fULL << 27;
constexpr uint64_t MASK_PIECES = 0x1ffffffULL;
constexpr std::array<uint64_t, 2> MASK_PLAYER = { 0xffff'ffffULL, 0xffff'ffff'0000'0000 };
constexpr std::array<int32_t, 2> INDEX_KINGS = { 32 + 25, 32 + 25 + 4 };
constexpr std::array<uint32_t, 2> MASK_END_POSITIONS = { 0b00100ULL << 20, 0b00100ULL };


void printKings(uint64_t pieces);

class Board;
typedef void (*MoveFunc)(GameCards& gameCards, const Board& board, const bool finished, char depth);

class Board {
public:
	uint64_t pieces;

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
		uint32_t bitScan = (movingPlayer ? piecesWithNewCards >> 32 : piecesWithNewCards) & MASK_PIECES;
		unsigned long lastFromI = 0;
		unsigned long fromI;
		unsigned long nextFromI;
		bool hasBits = _BitScanForward(&fromI, bitScan);
		uint32_t kingIndex = (piecesWithNewCards >> INDEX_KINGS[movingPlayer]) & 7;
		piecesWithNewCards &= ~(((uint64_t)7) << INDEX_KINGS[movingPlayer]);
		uint32_t beforeKingMask = _pdep_u32((1ULL << kingIndex), movingPlayer ? piecesWithNewCards >> 32 : piecesWithNewCards) - 1;
		const uint32_t opponentKingIndex = (1ULL << ((piecesWithNewCards >> INDEX_KINGS[!movingPlayer]) & 7));
		const uint32_t opponentKing = _pdep_u32(opponentKingIndex, movingPlayer ? piecesWithNewCards >> 32 : piecesWithNewCards);

		while (hasBits) {
			const uint32_t fromBit = (1ULL << fromI);
			bitScan &= ~fromBit;
			uint64_t newPiecesWithoutLandPiece = piecesWithNewCards & ~(movingPlayer ? ((uint64_t)fromBit) << 32 : fromBit);
			bool isKingMove = !kingIndex;
			nextFromI = 25;
			hasBits = _BitScanForward(&nextFromI, bitScan);
			
			const uint32_t endMask = opponentKing | (isKingMove ? MASK_END_POSITIONS[movingPlayer] : 0);
			uint32_t scan = card.moveBoard[player][fromI] & ~(movingPlayer ? piecesWithNewCards >> 32 : piecesWithNewCards);
			const uint32_t kingSamePositionRange = isKingMove ? (1ULL << nextFromI) - (1ULL < lastFromI) : ~((uint32_t)0);
			while (scan) {
				const uint32_t landBit = scan & -scan;
				scan &= ~landBit;
				const uint64_t landBitHigh = ((uint64_t)landBit) << 32;
				Board board{ newPiecesWithoutLandPiece };
				board.pieces |= movingPlayer ? landBitHigh : landBit;	 // add arrival piece
				board.pieces &= ~((movingPlayer ? landBit : landBitHigh) | (((uint64_t)7) << INDEX_KINGS[movingPlayer])); // possible take piece
				const uint32_t beforeKingPieces = (movingPlayer ? board.pieces >> 32 : board.pieces) & (isKingMove ? landBit - 1 : beforeKingMask);
				board.pieces |= ((uint64_t)_popcnt32(beforeKingPieces)) << INDEX_KINGS[movingPlayer];
				//std::cout << std::bitset<32>(endMask) << std::endl << std::bitset<32>(landBit) << 'h' << std::endl;
				const bool finished = landBit & endMask;
				cb(gameCards, board, finished, depth);
			}
			lastFromI = fromI;
			fromI = nextFromI;
			kingIndex--;
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
			uint64_t piecesWithNewCards = playerPiecesWithoutCards | (((uint64_t)cardStuff & 0xff00) << (27ULL - 8ULL));
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
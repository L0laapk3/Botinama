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

constexpr uint64_t INDEX_TURN = 25;
constexpr uint64_t MASK_TURN = 1ULL << INDEX_TURN;
constexpr uint64_t INDEX_CARDS = 27;
constexpr uint64_t MASK_CARDS = 0x1fULL << INDEX_CARDS;
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

	static Board fromString(std::string str, bool player = false);

	void print(GameCards& gameCards, bool finished = false) const;
	static void print(GameCards& gameCards, std::vector<Board> board, std::vector<bool> finished = { false });

	void valid() const;
	bool finished() const;
	bool winner() const;

private:
	template<MoveFunc cb>
	void iterateMoves(GameCards& gameCards, const std::array<uint32_t, 25>& moveBoard, uint64_t piecesWithNewCards, bool player, unsigned long long depth) const {
		//card.print();
		uint32_t bitScan = (player ? piecesWithNewCards >> 32 : piecesWithNewCards) & MASK_PIECES;
		unsigned long lastFromI = 0;
		unsigned long fromI;
		unsigned long nextFromI;
		bool hasBits = _BitScanForward(&fromI, bitScan);
		uint32_t kingIndex = (piecesWithNewCards >> INDEX_KINGS[player]) & 7;
		const uint32_t opponentKingIndex = (1ULL << ((piecesWithNewCards >> INDEX_KINGS[!player]) & 7));
		uint32_t beforeKingMask = _pdep_u32((1ULL << kingIndex), player ? piecesWithNewCards >> 32 : piecesWithNewCards) - 1;
		const uint32_t opponentKing = _pdep_u32(opponentKingIndex, player ? piecesWithNewCards : piecesWithNewCards >> 32);
		const uint32_t opponentBeforeKingPieces = (player ? piecesWithNewCards : piecesWithNewCards >> 32) & (opponentKing - 1);
		piecesWithNewCards &= ~(((uint64_t)0b1111111) << INDEX_KINGS[0]);
		//std::cout << std::bitset<25>(opponentKingIndex) << std::endl;

		while (hasBits) {
			const uint32_t fromBit = (1ULL << fromI);
			bitScan &= ~fromBit;
			uint64_t newPiecesWithoutLandPiece = piecesWithNewCards & ~(player ? ((uint64_t)fromBit) << 32 : fromBit);
			bool isKingMove = !kingIndex;
			nextFromI = 25;
			hasBits = _BitScanForward(&nextFromI, bitScan);
			
			const uint32_t endMask = opponentKing | (isKingMove ? MASK_END_POSITIONS[player] : 0);
			uint32_t scan = moveBoard[fromI] & ~(player ? piecesWithNewCards >> 32 : piecesWithNewCards);
			const uint32_t kingSamePositionRange = isKingMove ? (1ULL << nextFromI) - (1ULL < lastFromI) : ~((uint32_t)0);
			while (scan) {
				const uint32_t landBit = scan & -scan;
				scan &= ~landBit;
				const uint64_t landBitHigh = ((uint64_t)landBit) << 32;
				Board board{ newPiecesWithoutLandPiece };
				board.pieces |= player ? landBitHigh : landBit;	 // add arrival piece
				board.pieces &= ~(player ? landBit : landBitHigh); // possible take piece
				const uint32_t beforeKingPieces = (player ? board.pieces >> 32 : board.pieces) & (isKingMove ? landBit - 1 : beforeKingMask);
				board.pieces |= ((uint64_t)_popcnt32(beforeKingPieces)) << INDEX_KINGS[player];
				board.pieces |= ((uint64_t)_popcnt32(opponentBeforeKingPieces & ~landBit)) << INDEX_KINGS[!player];
				//printKings(board.pieces);
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
		const CardsPos& cardsPos = CARDS_LUT[(pieces &  MASK_CARDS) >> INDEX_CARDS];
		uint64_t playerPiecesWithoutCards = pieces & ~MASK_CARDS;
		playerPiecesWithoutCards ^= MASK_TURN; // invert player bit
		uint32_t cardStuff = cardsPos.players[player];
		for (int i = 0; i < 2; i++) {
			unsigned long cardI = cardStuff & 0xff;
			uint64_t piecesWithNewCards = playerPiecesWithoutCards | (((uint64_t)cardStuff & 0xff00) << (INDEX_CARDS - 8ULL));
			cardStuff >>= 16;
			const auto& card = gameCards[cardI];
			iterateMoves<cb>(gameCards, card.moveBoard[player], piecesWithNewCards, player, depth);
		}
	}
	
	template<MoveFunc cb>
	void reverseMoves(GameCards& gameCards, unsigned long long depth) const {
		bool player = pieces & MASK_TURN;
		const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
		uint64_t playerPiecesWithoutCards = pieces & ~MASK_CARDS;
		playerPiecesWithoutCards ^= MASK_TURN; // invert player bit
		uint32_t cardStuff = cardsPos.players[player];
		const auto& card = gameCards[cardsPos.side];
		for (int i = 0; i < 2; i++) {
			unsigned long cardI = cardStuff & 0xff;
			uint64_t piecesWithNewCards = playerPiecesWithoutCards | (((uint64_t)cardStuff & 0xff00) << (INDEX_CARDS - 8ULL));
			cardStuff >>= 16;
			iterateMoves<cb>(gameCards, card.moveBoard[!player], piecesWithNewCards, player, depth);
		}
	}

};
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
#include "Botama.h"


void printKings(U64 pieces);

class Board;
typedef void (*MoveFunc)(GameCards& gameCards, const Board& board, const bool finished, U32 depth);

class Board {
public:
	U64 pieces;

	static Board fromString(std::string str, bool player = false);

	void print(GameCards& gameCards, bool finished = false) const;
	static void print(GameCards& gameCards, std::vector<Board> board, std::vector<bool> finished = { false });

	void valid() const;
	bool finished() const;
	bool winner() const;

private:
	template<MoveFunc cb>
	void iterateMoves(GameCards& gameCards, const MoveBoard& moveBoards, U64 piecesWithNewCards, bool player, U64 depth) const {
		//card.print();
		U32 bitScan = (player ? piecesWithNewCards >> 32 : piecesWithNewCards) & MASK_PIECES;
		unsigned long lastFromI = 0;
		unsigned long fromI;
		unsigned long nextFromI;
		bool hasBits = _BitScanForward(&fromI, bitScan);
		U32 kingIndex = (piecesWithNewCards >> INDEX_KINGS[player]) & 7;
		const U32 opponentKingIndex = (1ULL << ((piecesWithNewCards >> INDEX_KINGS[!player]) & 7));
		U32 beforeKingMask = _pdep_u32((1ULL << kingIndex), player ? piecesWithNewCards >> 32 : piecesWithNewCards) - 1;
		const U32 opponentKing = _pdep_u32(opponentKingIndex, player ? piecesWithNewCards : piecesWithNewCards >> 32);
		const U32 opponentBeforeKingPieces = (player ? piecesWithNewCards : piecesWithNewCards >> 32) & (opponentKing - 1);
		piecesWithNewCards &= ~(((U64)0b1111111) << INDEX_KINGS[0]);
		//std::cout << std::bitset<25>(opponentKingIndex) << std::endl;

		while (hasBits) {
			const U32 fromBit = (1ULL << fromI);
			bitScan &= ~fromBit;
			U64 newPiecesWithoutLandPiece = piecesWithNewCards & ~(player ? ((U64)fromBit) << 32 : fromBit);
			bool isKingMove = !kingIndex;
			nextFromI = 25;
			hasBits = _BitScanForward(&nextFromI, bitScan);
			
			const U32 endMask = opponentKing | (isKingMove ? MASK_END_POSITIONS[player] : 0);
			U32 scan = moveBoards[fromI] & ~(player ? piecesWithNewCards >> 32 : piecesWithNewCards);
			const U32 kingSamePositionRange = isKingMove ? (1ULL << nextFromI) - (1ULL < lastFromI) : ~((U32)0);
			while (scan) {
				const U32 landBit = scan & -scan;
				scan &= ~landBit;
				const U64 landBitHigh = ((U64)landBit) << 32;
				Board board{ newPiecesWithoutLandPiece };
				board.pieces |= player ? landBitHigh : landBit;	 // add arrival piece
				board.pieces &= ~(player ? landBit : landBitHigh); // possible take piece
				const U32 beforeKingPieces = (player ? board.pieces >> 32 : board.pieces) & (isKingMove ? landBit - 1 : beforeKingMask);
				board.pieces |= ((U64)_popcnt32(beforeKingPieces)) << INDEX_KINGS[player];
				board.pieces |= ((U64)_popcnt32(opponentBeforeKingPieces & ~landBit)) << INDEX_KINGS[!player];
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
	void forwardMoves(GameCards& gameCards, U32 depth) const {
		bool player = pieces & MASK_TURN;
		const CardsPos& cardsPos = CARDS_LUT[(pieces &  MASK_CARDS) >> INDEX_CARDS];
		U64 playerPiecesWithoutCards = pieces & ~MASK_CARDS;
		playerPiecesWithoutCards ^= MASK_TURN; // invert player bit
		U32 cardStuff = cardsPos.players[player];
		for (int i = 0; i < 2; i++) {
			unsigned long cardI = cardStuff & 0xff;
			U64 piecesWithNewCards = playerPiecesWithoutCards | (((U64)cardStuff & 0xff00) << (INDEX_CARDS - 8ULL));
			cardStuff >>= 16;
			const auto& card = gameCards[cardI];
			iterateMoves<cb>(gameCards, card.moveBoards[player], piecesWithNewCards, player, depth);
		}
	}
	
	template<MoveFunc cb>
	void reverseMoves(GameCards& gameCards, U32 depth) const {
		bool player = !(pieces & MASK_TURN);
		const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
		U64 playerPiecesWithoutCards = pieces & ~MASK_CARDS;
		playerPiecesWithoutCards ^= MASK_TURN; // invert player bit
		const auto& card = gameCards[cardsPos.side];
		U32 cardStuff = cardsPos.players[player];
		for (int i = 0; i < 2; i++) {
			unsigned long cardI = cardStuff & 0xff;
			U64 piecesWithNewCards = playerPiecesWithoutCards | (((U64)cardStuff & 0xff00) << (INDEX_CARDS - 8ULL));
			cardStuff >>= 16;
			iterateMoves<cb>(gameCards, card.moveBoards[!player], piecesWithNewCards, player, depth);
		}
	}

};
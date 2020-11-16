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

	static Board fromString(std::string str, bool player);

	void print(GameCards& gameCards, bool finished = false) const;
	static void print(GameCards& gameCards, std::vector<Board> board, std::vector<bool> finished = { false });

	void valid() const;
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
			bitScan -= fromBit;
			U64 newPiecesWithoutLandPiece = piecesWithNewCards & ~(player ? ((U64)fromBit) << 32 : fromBit);
			bool isKingMove = !kingIndex;
			nextFromI = 25;
			hasBits = _BitScanForward(&nextFromI, bitScan);

			const U32 endMask = opponentKing | (isKingMove ? MASK_END_POSITIONS[player] : 0);
			U32 scan = moveBoards[fromI] & ~(player ? piecesWithNewCards >> 32 : piecesWithNewCards);
			const U32 kingSamePositionRange = isKingMove ? (1ULL << nextFromI) - (1ULL < lastFromI) : ~((U32)0);
			while (scan) {
				const U32 landBit = scan & -scan;
				scan -= landBit;
				const U64 landBitHigh = ((U64)landBit) << 32;
				U64 newPieces = newPiecesWithoutLandPiece;
				newPieces |= player ? landBitHigh : landBit;	 // add arrival piece
				newPieces &= ~(player ? landBit : landBitHigh); // possible take piece
				const U32 beforeKingPieces = (player ? newPieces >> 32 : newPieces) & (isKingMove ? landBit - 1 : beforeKingMask);
				newPieces |= ((U64)_popcnt32(beforeKingPieces)) << INDEX_KINGS[player];
				newPieces |= ((U64)_popcnt32(opponentBeforeKingPieces & ~landBit)) << INDEX_KINGS[!player];
				//printKings(board.pieces);
				//std::cout << std::bitset<32>(endMask) << std::endl << std::bitset<32>(landBit) << 'h' << std::endl;
				const bool finished = landBit & endMask;
				cb(gameCards, Board{ newPieces }, finished, depth);
			}
			lastFromI = fromI;
			fromI = nextFromI;
			kingIndex--;
		}
	}
	U32 countMoves(const MoveBoard& moveBoards, bool player) const {
		U32 bitScan = (player ? pieces >> 32 : pieces) & MASK_PIECES;
		U32 total = 0;
		for (int i = 0; i < 5; i++) {
			U32 fromBit = _pdep_u32(1 << i, bitScan);
			unsigned long fromI = 25;
			_BitScanForward(&fromI, fromBit);
			U32 scan = moveBoards[fromI] & ~bitScan;
			total += _popcnt32(scan);
		}
		return total;
	}
public:
	template<MoveFunc cb>
	void forwardMoves(GameCards& gameCards, U32 depth) const {
		bool player = pieces & MASK_TURN;
		const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
		U64 piecesWithoutCards = pieces & ~MASK_CARDS;
		piecesWithoutCards ^= MASK_TURN; // invert player bit
		U32 cardStuff = cardsPos.players[player];
		for (int i = 0; i < 2; i++) {
			unsigned long cardI = cardStuff & 0xff;
			U64 piecesWithNewCards = piecesWithoutCards | (((U64)cardStuff & 0xff00) << (INDEX_CARDS - 8ULL));
			cardStuff >>= 16;
			const auto& card = gameCards[cardI];
			iterateMoves<cb>(gameCards, card.moveBoards[player], piecesWithNewCards, player, depth);
		}
	}
	U32 countForwardMoves(GameCards& gameCards) const {
		bool player = pieces & MASK_TURN;
		const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
		U32 cardStuff = cardsPos.players[player];
		U32 total = 0;
		U64 playerPieces = (player ? pieces >> 32 : pieces) & MASK_PIECES;
		const auto& card0 = gameCards[cardStuff & 0xff].moveBoards[player];
		const auto& card1 = gameCards[(cardStuff >> 16) & 0xff].moveBoards[player];
		for (int i = 0; i < 5; i++) {
			U32 fromBit = _pdep_u32(1 << i, playerPieces);
			unsigned long fromI = 25;
			_BitScanForward(&fromI, fromBit);
			total += _popcnt64((card0[fromI] & ~playerPieces) | (((U64)card1[fromI] & ~playerPieces) << 32));
		}
		return total;
	}

	template<MoveFunc cb>
	void reverseMoves(GameCards& gameCards, U32 depth) const {
		bool player = !(pieces & MASK_TURN);
		const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
		U64 playerPiecesWithoutCards = pieces & ~MASK_CARDS;
		playerPiecesWithoutCards ^= MASK_TURN; // invert player bit
		const auto& card = gameCards[cardsPos.side];
		U32 cardStuff = cardsPos.players[!player];
		for (int i = 0; i < 2; i++) {
			unsigned long cardI = cardStuff & 0xff;
			U64 piecesWithNewCards = playerPiecesWithoutCards | (((U64)cardStuff & 0xff00) << (INDEX_CARDS - 8ULL));
			cardStuff >>= 16;
			iterateMoves<cb>(gameCards, card.moveBoards[!player], piecesWithNewCards, player, depth);
		}
	}

};
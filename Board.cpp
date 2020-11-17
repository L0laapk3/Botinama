﻿
#include "Board.h"
#include "Botama.h"

#include <intrin.h>
#include <bitset>
#include <iostream>
#include <algorithm>




U32 Board::countForwardMoves(const GameCards& gameCards) const {
	bool player = pieces & MASK_TURN;
	const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
	U32 cardStuff = cardsPos.players[player];
	U32 total = 0;
	U32 playerPieces = (pieces >> (player ? 32 : 0)) & MASK_PIECES;
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


Score Board::findImmediateWins(const GameCards& gameCards) const {
	bool player = pieces & MASK_TURN;
	// check win in 1 ply
	const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
	U32 player0cards = cardsPos.players[player];
	const auto& player0card0Reverse = gameCards[player0cards & 0xff].moveBoards[!player];
	const auto& player0card1Reverse = gameCards[(player0cards >> 16) & 0xff].moveBoards[!player];
	const U32 kingPieceNum = (pieces >> INDEX_KINGS[player]) & 7;
	const U32 opponentKingPieceNum = (pieces >> INDEX_KINGS[!player]) & 7;
	const U32 king = _pdep_u32(1ULL << kingPieceNum, pieces >> (player ? 32 : 0));
	const U32 opponentKing = _pdep_u32(1ULL << opponentKingPieceNum, pieces >> (player ? 0 : 32));
	unsigned long opponentKingI;
	_BitScanForward(&opponentKingI, opponentKing);
	const U32 positionsToAttackOpponentKing = player0card0Reverse[opponentKingI] | player0card1Reverse[opponentKingI];
	const U32 positionsToReachTemple = player0card0Reverse[INDEX_END_POSITIONS[player]] | player0card1Reverse[INDEX_END_POSITIONS[player]];
	if ((positionsToAttackOpponentKing & (pieces >> (player ? 32 : 0))) || (positionsToReachTemple & king))
		return SCORE_MAX; // win if take opponent king or if king reaches temple
	U32 player1cards = cardsPos.players[player];
	const auto& player1card0Reverse = gameCards[player1cards & 0xff].moveBoards[player];
	const auto& player1card1Reverse = gameCards[(player1cards >> 16) & 0xff].moveBoards[player];
	unsigned long kingI;
	_BitScanForward(&kingI, king);
	const U32 opponentPositionsToAttackKing = player1card0Reverse[kingI] | player1card1Reverse[kingI];
	const U32 opponentPositionsToReachTemple = player1card0Reverse[INDEX_END_POSITIONS[!player]] | player1card1Reverse[INDEX_END_POSITIONS[!player]];
	if (opponentPositionsToReachTemple & opponentKing)
		return SCORE_MIN; // lose if opponent king reaches temple
	// todo: lose if opponent can take your king and you cannot prevent it??
	return 0;
}
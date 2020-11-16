
#include "Board.h"
#include "Botama.h"

#include <bitset>
#include <iostream>
#include <algorithm>




U32 Board::countForwardMoves(GameCards& gameCards) const {
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

#include <bitset>
#include "TableBase.h"
#include "Board.h"


void placePieces(GameCards& gameCards, U64 board, U32 occupied, U32 beforeKing, U32 beforeOtherKing, U32 startAt, U32 spotsLeft, U32 minSpots0, U32 minSpotsAll) {
	if (spotsLeft == minSpotsAll) {
		board |= _popcnt64(beforeKing & board) << INDEX_KINGS[0];
		board |= _popcnt64(beforeOtherKing & board) << INDEX_KINGS[1];
		std::cout << std::bitset<25>(board >> 32) << ' ' << std::bitset<25>(board) << std::endl;
		for (int i = 0; i < 30; i++) {
			//Board{ board }.print(gameCards, true);
			board += 1ULL << INDEX_CARDS;
		}
		return;
	}
	bool player = spotsLeft <= minSpots0;
	U64 piece = 1ULL << startAt;
	while (piece & MASK_PIECES) {
		startAt++;
		if (piece & ~occupied)
			placePieces(gameCards, board | (((U64)piece) << (player ? 32 : 0)), occupied | piece, beforeKing, beforeOtherKing, spotsLeft == minSpots0 ? 0 : startAt, spotsLeft - 1, minSpots0, minSpotsAll);
		piece <<= 1;
	}
}

// bitboards where player 0 wins
void TableBase::generate(GameCards& gameCards, std::array<U32, 2> maxPawns) {
	U32 king = MASK_END_POSITIONS[0];
	U32 otherKing = 1ULL;
	for (U32 otherKingPos = 0; otherKingPos < 24; otherKingPos++) {
		otherKing <<= king == otherKing;
		for (U32 myMaxPawns = 0; myMaxPawns <= maxPawns[0]; myMaxPawns++)
			for (U32 otherMaxPawns = 0; otherMaxPawns <= maxPawns[1]; otherMaxPawns++)
				placePieces(gameCards, king | (((U64)otherKing) << 32), king | otherKing, king - 1, otherKing - (1ULL << 32), 0, 23, 23 - myMaxPawns, 23 - myMaxPawns - otherMaxPawns);
		otherKing <<= 1;
	}
}

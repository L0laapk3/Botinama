#pragma once


template<MoveFunc cb>
void Board::iterateMoves(GameCards& gameCards, const MoveBoard& moveBoards, U64 piecesWithNewCards, bool player, U32 maxDepth) const {
	//card.print();
	U32 bitScan = (piecesWithNewCards >> (player ? 32 : 0)) & MASK_PIECES;
	U32 kingPieceNum = (piecesWithNewCards >> INDEX_KINGS[player]) & 7;
	const U32 opponentKingPieceNum = (piecesWithNewCards >> INDEX_KINGS[!player]) & 7;
	const U32 beforeKingMask = _pdep_u32(1ULL << kingPieceNum, piecesWithNewCards >> (player ? 32 : 0)) - 1;
	const U32 opponentKing = _pdep_u32(1ULL << opponentKingPieceNum, piecesWithNewCards >> (player ? 0 : 32));
	const U32 opponentBeforeKingPieces = (piecesWithNewCards >> (player ? 0 : 32)) & (opponentKing - 1);
	piecesWithNewCards &= ~(((U64)0b1111111) << INDEX_KINGS[0]);

	unsigned long fromI;
	while (_BitScanForward(&fromI, bitScan)) {
		const U32 fromBit = (1ULL << fromI);
		bitScan -= fromBit;
		U64 newPiecesWithoutLandPiece = piecesWithNewCards & ~(((U64)fromBit) << (player ? 32 : 0));
		bool isKingMove = !kingPieceNum;

		const U32 endMask = opponentKing | (isKingMove ? MASK_END_POSITIONS[player] : 0);
		U32 scan = moveBoards[fromI] & ~(piecesWithNewCards >> (player ? 32 : 0));
		while (scan) {
			const U32 landBit = scan & -scan;
			scan -= landBit;
			U64 newPieces = newPiecesWithoutLandPiece;
			newPieces |= ((U64)landBit) << (player ? 32 : 0);	 // add arrival piece
			newPieces &= ~(((U64)landBit) << (player ? 0 : 32)); // possible take piece
			const U32 beforeKingPieces = (newPieces >> (player ? 32 : 0)) & (isKingMove ? landBit - 1 : beforeKingMask);
			newPieces |= ((U64)_popcnt32(beforeKingPieces)) << INDEX_KINGS[player];
			newPieces |= ((U64)_popcnt32(opponentBeforeKingPieces & ~landBit)) << INDEX_KINGS[!player];
			const bool finished = landBit & endMask;
			cb(gameCards, Board{ newPieces }, finished, maxDepth);
		}
		kingPieceNum--;
	}
}

template<MoveFunc cb>
void Board::forwardMoves(GameCards& gameCards, U32 maxDepth) const {
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
		iterateMoves<cb>(gameCards, card.moveBoards[player], piecesWithNewCards, player, maxDepth);
	}
}

template<MoveFunc cb>
void Board::reverseMoves(GameCards& gameCards, U32 maxDepth) const {
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
		iterateMoves<cb>(gameCards, card.moveBoards[!player], piecesWithNewCards, player, maxDepth);
	}
}
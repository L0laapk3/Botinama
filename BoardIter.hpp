#pragma once


// unfortunately due to my lack of cpp knowledge I had to duplicate this into boardsearch. see there for version with move sorting
template<MoveFunc cb, bool reverse>
void Board::iterateMoves(const GameCards& gameCards, const MoveBoard& moveBoards, U64 piecesWithNewCards, bool player, const bool createPiece) const {
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
		U32 opponentBeforeKingPiecesWithExtra = opponentBeforeKingPieces;
		const U32 fromBit = (1ULL << fromI);
		bitScan -= fromBit;
		bool isKingMove = !kingPieceNum;
		U64 newPiecesWithoutLandPiece = piecesWithNewCards & ~(((U64)fromBit) << (player ? 32 : 0));
		for (int i = 0; i <= createPiece && reverse; i++) { // spawn a piece, simulate taking for reverse moves
			newPiecesWithoutLandPiece |= i ? ((U64)fromBit) << (player ? 0 : 32) : 0;
			opponentBeforeKingPiecesWithExtra |= i ? fromBit & (opponentKing - 1) : 0;

			const U32 endMask = opponentKing | (isKingMove ? MASK_END_POSITIONS[player] : 0);
			U32 scan = moveBoards[fromI];
			if (reverse)
				scan &= ~(piecesWithNewCards | _rotr64(piecesWithNewCards, 32)); // blocked by any occupied square
			else
				scan &= ~(piecesWithNewCards >> (player ? 32 : 0));	// blocked by own pieces, opponent pieces are taken
			while (scan) {
				const U32 landBit = scan & -scan;
				scan -= landBit;
				U64 newPieces = newPiecesWithoutLandPiece;
				newPieces |= ((U64)landBit) << (player ? 32 : 0);	 // add arrival piece
				if (!reverse)
					newPieces &= ~(((U64)landBit) << (player ? 0 : 32)); // possible take piece
				const U32 beforeKingPieces = (newPieces >> (player ? 32 : 0)) & (isKingMove ? landBit - 1 : beforeKingMask);
				newPieces |= ((U64)_popcnt32(beforeKingPieces)) << INDEX_KINGS[player];
				newPieces |= ((U64)_popcnt32(opponentBeforeKingPiecesWithExtra & ~landBit)) << INDEX_KINGS[!player];
				const bool finished = landBit & endMask;
				cb(gameCards, Board{ newPieces }, finished);
			}
		}
		kingPieceNum--;
	}
}

template<MoveFunc cb>
void Board::forwardMoves(const GameCards& gameCards) const {
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
		iterateMoves<cb, false>(gameCards, card.moveBoards[player], piecesWithNewCards, player, false);
	}
}

template<MoveFunc cb>
void Board::reverseMoves(const GameCards& gameCards, const std::array<uint8_t, 2>& maxPieces) const {
	//print(gameCards, false, true);
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
		//std::cout << _popcnt32((pieces >> (player ? 0 : 32)) & MASK_PIECES) << std::endl;
		iterateMoves<cb, true>(gameCards, card.moveBoards[!player], piecesWithNewCards, player, maxPieces[!player] > _popcnt32((pieces >> (player ? 0 : 32)) & MASK_PIECES));
	}
}
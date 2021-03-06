

#include "Board.h"


template<>
Board Board::decompressIndex<false>(U32 boardComp) {
	U64 pieces = 0;
	pieces |= ((U64)boardComp % 30) << INDEX_CARDS;
	boardComp /= 30;
	
	if (TB_MEN <= 4) {
		U32 otherPiece1I = boardComp % 25;
		boardComp /= 25;
		pieces |= 1ULL << (otherPiece1I + 32);
		U32 piece1I = boardComp % 25;
		boardComp /= 25;
		pieces |= 1ULL << piece1I;
	} else {
		U32 otherPieceValue = boardComp % (25*13);
		U32 otherPiece1I = otherPieceValue / 25;
		U32 otherPiece2I = otherPieceValue % 25;
		if (otherPiece1I > otherPiece2I) {
			otherPieceValue = 25*26-1 - otherPieceValue;
			otherPiece1I = otherPieceValue / 25;
			otherPiece2I = otherPieceValue % 25;
		}
		pieces |= (1ULL << (otherPiece1I + 32)) | (1ULL << (otherPiece2I + 32));
		boardComp /= 25*13;
		
		U32 pieceValue = boardComp % (25*13);
		U32 piece1I = pieceValue / 25;
		U32 piece2I = pieceValue % 25;
		if (piece1I > piece2I) {
			pieceValue = 25*26-1 - pieceValue;
			piece1I = pieceValue / 25;
			piece2I = pieceValue % 25;
		}
		pieces |= (1ULL << piece1I) | (1ULL << piece2I);
		boardComp /= 25*13;
	}

	U32 otherKingI = boardComp % 25;
	boardComp /= 25;
	pieces |= (1ULL << (otherKingI + 32));
	// pieces |= ((U64)_popcnt64(((1ULL << (otherKingI + 32)) - (1ULL << 32)) & pieces)) << INDEX_KINGS[1];

	U32 kingI = boardComp % 25;
	boardComp /= 25;
	pieces |= (1ULL << kingI);
	// pieces |= ((U64)_popcnt32(((1ULL << kingI) - 1) & pieces)) << INDEX_KINGS[0];

	return Board{ pieces, (1ULL << kingI) | (1ULL << (otherKingI + 32)) };
}

template<>
Board Board::decompressIndex<true>(U32 boardComp) {
	U64 pieces = MASK_TURN;
	pieces |= ((U64)CARDS_INVERT[boardComp % 30]) << INDEX_CARDS;
    assert(CARDS_INVERT[boardComp % 30] < 30);
	boardComp /= 30;
	
	if (TB_MEN <= 4) {
		U32 piece1I = 24 - (boardComp % 25);
		boardComp /= 25;
		pieces |= 1ULL << piece1I;
		U32 otherPiece1I = 24 - (boardComp % 25);
		boardComp /= 25;
		pieces |= 1ULL << (otherPiece1I + 32);
	} else {	
		U32 pieceValue = boardComp % (25*13);
		U32 piece1I = pieceValue % 25;
		U32 piece2I = pieceValue / 25;
		if (piece2I > piece1I) {
			pieceValue = 25*26-1 - pieceValue;
			piece1I = pieceValue % 25;
			piece2I = pieceValue / 25;
		}
        piece1I = 24 - piece1I;
        piece2I = 24 - piece2I;
		pieces |= (1ULL << piece1I) | (1ULL << piece2I);
		boardComp /= 25*13;
        
		U32 otherPieceValue = boardComp % (25*13);
		U32 otherPiece1I = otherPieceValue % 25;
		U32 otherPiece2I = otherPieceValue / 25;
        // std::cout << otherPieceValue << ' ' << otherPiece1I << ' ' << otherPiece2I << " ref";
		if (otherPiece2I > otherPiece1I) {
			otherPieceValue = 25*26-1 - otherPieceValue;
			otherPiece1I = otherPieceValue % 25;
			otherPiece2I = otherPieceValue / 25;
            // std::cout << ' ' << otherPiece1I << ' ' << otherPiece2I;
		}
        // std::cout << std::endl;
        otherPiece1I = 24 - otherPiece1I;
        otherPiece2I = 24 - otherPiece2I;
		pieces |= (1ULL << (otherPiece1I + 32)) | (1ULL << (otherPiece2I + 32));
		boardComp /= 25*13;
	}

	U32 kingI = 24 - (boardComp % 25);
	boardComp /= 25;
	pieces |= 1ULL << kingI;

	U32 otherKingI = 24 - (boardComp % 25);
	boardComp /= 25;
	pieces |= 1ULL << (otherKingI + 32);

	return Board{ pieces, (1ULL << kingI) | (1ULL << (otherKingI + 32)) };
}


template<bool flip, bool withoutCards>
U32 Board::compressToIndex() const {
	U32 boardComp = 0;
	U64 bothPieces = (pieces - kings) & (MASK_PIECES | (((U64)MASK_PIECES) << 32));
	unsigned long kingI, otherKingI;
	assert(otherKing != 0);
	_BitScanForward(&kingI, kings);
	_BitScanForward(&otherKingI, kings >> 32);
	boardComp = boardComp * 25 + (flip ? 24 - otherKingI : kingI);
	boardComp = boardComp * 25 + (flip ? 24 - kingI : otherKingI);

	unsigned long piece1I, otherPiece1I;
	if (!_BitScanForward(&piece1I, bothPieces))
		piece1I = kingI;
	if (!_BitScanForward(&otherPiece1I, bothPieces >> 32))
		otherPiece1I = otherKingI;
	
	if (TB_MEN <= 4) {
		boardComp = boardComp * 25 + (flip ? 24 - otherPiece1I : piece1I);
		boardComp = boardComp * 25 + (flip ? 24 - piece1I : otherPiece1I);
		
		assert(boardComp < 25*25*25*25);
	} else {
		_BitScanReverse(&kingI, bothPieces);
		_BitScanReverse(&otherKingI, bothPieces >> 32);
		U32 pieceValue = flip ? 24*26 - otherPiece1I - 25*otherKingI : piece1I * 25 + kingI;
		U32 otherPieceValue = flip ? 24*26 - piece1I - 25*kingI : otherPiece1I * 25 + otherKingI;

		boardComp = boardComp * 25*13 + std::min(pieceValue, 25*26-1 - pieceValue);
		boardComp = boardComp * 25*13 + std::min(otherPieceValue, 25*26-1 - otherPieceValue);
		
		assert(boardComp < 25*25*25*13*25*13);
	}

	boardComp *= 30;

	if (!withoutCards) {
		U32 cardI = (pieces & MASK_CARDS) >> INDEX_CARDS;
		boardComp += flip ? CARDS_INVERT[cardI] : cardI;
	}


	// if (decompressIndex<false>(boardComp).pieces != pieces) {
	// 	std::cout << std::bitset<64>(decompressIndex<false>(boardComp).pieces) << std::endl << std::bitset<64>(pieces) << std::endl;
	// 	assert(decompressIndex<false>(boardComp).pieces == pieces);
	// }

	return boardComp;
}
template U32 Board::compressToIndex<false, false>() const;
template U32 Board::compressToIndex<false, true>() const;
template U32 Board::compressToIndex<true, false>() const;
template U32 Board::compressToIndex<true, true>() const;




#ifdef USE_TB
bool Board::testForwardTB(GameCards& cards, std::array<int8_t, TBSIZE>& table) const {
	// iterate moves
	const bool player = 1;
	const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
	U64 piecesWithoutCards = pieces & ~MASK_CARDS;
	piecesWithoutCards ^= MASK_TURN; // invert player bit
	U32 cardStuff = cardsPos.players[player];
	for (int i = 0; i < 2; i++) {
		unsigned long cardI = cardStuff & 0xff;
		U64 piecesWithNewCards = piecesWithoutCards | (((U64)cardStuff & 0xff00) << (INDEX_CARDS - 8ULL));
		cardStuff >>= 16;
		const auto& moveBoard = cards[cardI].moveBoards[player];
		U32 bitScan = (piecesWithNewCards >> (player ? 32 : 0)) & MASK_PIECES;

		const uint32_t king = kings >> (player ? 32 : 0);
		const uint32_t opponentKing = kings >> (player ? 0 : 32);

		unsigned long fromI;
		while (_BitScanForward(&fromI, bitScan)) {
			const U32 fromBit = (1ULL << fromI);
			bitScan -= fromBit;
			bool isKingMove = fromBit == king;
			U32 scan = moveBoard[fromI] & ~(pieces >> (player ? 32 : 0));
			U64 newPiecesWithoutLandPiece = piecesWithNewCards & ~(((U64)fromBit) << (player ? 32 : 0));

			const U32 endMask = opponentKing | (isKingMove ? MASK_END_POSITIONS[player] : 0);
			while (scan) {
				const U32 landBit = scan & -scan;
				scan -= landBit;
				Board newBoard = Board{ newPiecesWithoutLandPiece, isKingMove ? kings - (((U64)fromBit) << (player ? 32 : 0)) + (((U64)landBit) << (player ? 32 : 0)) : kings };
				newBoard.pieces |= ((U64)landBit) << (player ? 32 : 0);	 // add arrival piece
				newBoard.pieces &= ~(((U64)landBit) << (player ? 0 : 32)); // possible take piece
				if (landBit & endMask)
					return false;
				if (table[newBoard.compressToIndex<false>()] <= 0)
					return false;
			}
		}
	}
	return true;
}
#endif
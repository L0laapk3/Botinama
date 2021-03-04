

#include "Board.h"



U32 Board::compressToIndex() const {
	U32 boardComp = 0;
	U32 bluePieces = pieces & MASK_PIECES;
	U32 redPieces = (pieces >> 32) & MASK_PIECES;
	const U32 king = kings & MASK_PIECES;
	const U32 otherKing = (kings >> 32) & MASK_PIECES;
	unsigned long pieceI, otherpieceI;
	assert(king != 0);
	assert(otherKing != 0);
	_BitScanForward(&pieceI, king);
	_BitScanForward(&otherpieceI, otherKing);
	boardComp = boardComp * 25 + pieceI;
	boardComp = boardComp * 25 + otherpieceI;
	bluePieces -= king;
	redPieces -= otherKing;

	_BitScanForward(&pieceI, bluePieces);
	_BitScanForward(&otherpieceI, redPieces);
	bluePieces &= ~(1ULL << pieceI);
	redPieces &= ~(1ULL << otherpieceI);
	
	if (TB_MEN <= 4) {
		boardComp = boardComp * 25 + pieceI;
		boardComp = boardComp * 25 + otherpieceI;
		
		assert(boardComp < 25*25*25*25);
	} else {
		U32 pieceValue = pieceI * 25;
		U32 otherPieceValue = otherpieceI * 25;
		_BitScanForward(&pieceI, bluePieces);
		_BitScanForward(&otherpieceI, redPieces);
		pieceValue += pieceI;
		otherPieceValue += otherpieceI;

		boardComp = boardComp * 25*13 + std::min(pieceValue, 25*26-1 - pieceValue);
		boardComp = boardComp * 25*13 + std::min(otherPieceValue, 25*26-1 - otherPieceValue);
		
		assert(boardComp < 25*25*25*13*25*13);
	}

	boardComp = boardComp * 30 + ((pieces & MASK_CARDS) >> INDEX_CARDS);


	// if (decompress6Men(boardComp).pieces != board.pieces) {
	// 	std::cout << std::bitset<64>(decompress6Men(boardComp).pieces) << std::endl << std::bitset<64>(board.pieces) << std::endl;
	// 	assert(decompress6Men(boardComp).pieces == board.pieces);
	// }

	return boardComp;
}

U32 Board::invertCompressToIndex() const {
	U32 boardComp = 0;
	U32 bluePieces = pieces & MASK_PIECES;
	U32 redPieces = (pieces >> 32) & MASK_PIECES;
	const U32 king = kings & MASK_PIECES;
	const U32 otherKing = (kings >> 32) & MASK_PIECES;
	unsigned long pieceI, otherPieceI;
	_BitScanForward(&pieceI, king);
	_BitScanForward(&otherPieceI, otherKing);
	boardComp = boardComp * 25 + 24 - otherPieceI;
	boardComp = boardComp * 25 + 24 - pieceI;
	bluePieces -= king;
	redPieces -= otherKing;

	_BitScanReverse(&pieceI, bluePieces);
	_BitScanReverse(&otherPieceI, redPieces);
	bluePieces &= ~(1ULL << pieceI);
	redPieces &= ~(1ULL << otherPieceI);
	
	if (TB_MEN <= 4) {
		boardComp = boardComp * 25 + 24 - otherPieceI;
		boardComp = boardComp * 25 + 24 - pieceI;
	} else {
		U32 pieceValue = 25*24 + 24 - pieceI * 25;
		U32 otherPieceValue = 25*24 + 24 - otherPieceI * 25;
		_BitScanReverse(&pieceI, bluePieces);
		_BitScanReverse(&otherPieceI, redPieces);
		pieceValue -= pieceI;
		otherPieceValue -= otherPieceI;

		boardComp = boardComp * 25*13 + std::min(otherPieceValue, 25*26-1 - otherPieceValue);
		boardComp = boardComp * 25*13 + std::min(pieceValue, 25*26-1 - pieceValue);
		
		assert(boardComp < 25*25*25*13*25*13);
	}

	boardComp = boardComp * 30 + CARDS_INVERT[(pieces & MASK_CARDS) >> INDEX_CARDS];
	
	// if (decompress6Men(boardComp).pieces != board.invert().pieces) {
	// 	std::cout << std::bitset<64>(decompress6Men(boardComp).pieces) << std::endl << std::bitset<64>(board.invert().pieces) << 'i' << std::endl;
	// 	assert(decompress6Men(boardComp).pieces == board.invert().pieces);
	// }

	return boardComp;
}

Board Board::decompressIndex(U32 boardComp, bool player) {
	U64 pieces = 0;
	pieces |= ((U64)boardComp % 30) << INDEX_CARDS;
	boardComp /= 30;

	U32 piece1I, otherPiece1I;
	
	if (TB_MEN <= 4) {
		otherPiece1I = boardComp % 25;
		boardComp /= 25;
		pieces |= (1ULL << (otherPiece1I + 32)) & (MASK_PIECES << 32);
		piece1I = boardComp % 25;
		boardComp /= 25;
		pieces |= (1ULL << piece1I) & MASK_PIECES;
	} else {
		U32 otherPieceValue = boardComp % (25*13);
		otherPiece1I = otherPieceValue % 25;
		U32 otherPiece2I = otherPieceValue / 25;
		if (otherPiece1I <= otherPiece2I) {
			otherPieceValue = 25*26-1 - otherPieceValue;
			otherPiece1I = otherPieceValue % 25;
			otherPiece2I = otherPieceValue / 25;
		}
		pieces |= ((1ULL << (otherPiece1I + 32)) | (1ULL << (otherPiece2I + 32))) & (MASK_PIECES << 32);
		boardComp /= 25*13;
		
		U32 pieceValue = boardComp % (25*13);
		piece1I = pieceValue % 25;
		U32 piece2I = pieceValue / 25;
		if (piece1I <= piece2I) {
			pieceValue = 25*26-1 - pieceValue;
			piece1I = pieceValue % 25;
			piece2I = pieceValue / 25;
		}
		pieces |= ((1ULL << piece1I) | (1ULL << piece2I)) & MASK_PIECES;
		boardComp /= 25 * 13;
	}

	U32 otherKingI = boardComp % 25;
	boardComp /= 25;
	pieces |= (1ULL << (otherKingI + 32));
	// pieces |= ((U64)_popcnt64(((1ULL << (otherKingI + 32)) - (1ULL << 32)) & pieces)) << INDEX_KINGS[1];

	U32 kingI = boardComp % 25;
	boardComp /= 25;
	pieces |= (1ULL << kingI);
	// pieces |= ((U64)_popcnt32(((1ULL << kingI) - 1) & pieces)) << INDEX_KINGS[0];

	return Board{ pieces | (((U64)player) << INDEX_TURN), (1ULL << kingI) | (1ULL << (otherKingI + 32)) };
}







#ifdef USE_TB
bool Board::testForwardTB(GameCards& cards, std::array<int8_t, TBSIZE>& table) const {
	// iterate moves
	const bool player = 1;
	const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
	U64 piecesWithoutCards = pieces & ~MASK_CARDS;
	piecesWithoutCards ^= MASK_TURN; // invert player bit
	for (int taking = 1; taking >= 0; taking--) {
		U32 cardStuff = cardsPos.players[player];
		const U32 takeMask = ~(pieces >> (player ? 32 : 0)) & (pieces >> (player ? 0 : 32) ^ (taking ? 0 : ~(U32)0));
		const U32 kingMask = ~(pieces >> (player ? 32 : 0)) & ((pieces >> (player ? 0 : 32) | MASK_END_POSITIONS[player]) ^ (taking ? 0 : ~(U32)0));
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
				U32 scan = moveBoard[fromI] & (isKingMove ? kingMask : takeMask);
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
					if (table[newBoard.compressToIndex()] <= 0)
						return false;
				}
			}
		}
	}
	return true;
}
#endif

#include "Board.h"


Score Board::search(GameCards& gameCards, U32 depth, const bool finished, Score alpha, const Score beta) const {
	// negamax with alpha beta pruning
	bool player = pieces & MASK_TURN;
	if (finished || !depth)
		return (player ? -1 : 1) * eval(gameCards, finished);

	Score bestScore = SCORE_MIN;
	
	// Ive spent too many hours trying to get generator functions to compile since i want lazy move generation
	// fuck it, just copy pasting the movegen in here
	const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
	U64 piecesWithoutCards = pieces & ~MASK_CARDS;
	piecesWithoutCards ^= MASK_TURN; // invert player bit
	U32 cardStuff = cardsPos.players[player];
	for (int i = 0; i < 2; i++) {
		unsigned long cardI = cardStuff & 0xff;
		U64 piecesWithNewCards = piecesWithoutCards | (((U64)cardStuff & 0xff00) << (INDEX_CARDS - 8ULL));
		cardStuff >>= 16;
		const auto& moveBoard = gameCards[cardI].moveBoards[player];
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
			U32 scan = moveBoard[fromI] & ~(piecesWithNewCards >> (player ? 32 : 0));
			while (scan) {
				const U32 landBit = scan & -scan;
				scan -= landBit;
				Board board = Board{ newPiecesWithoutLandPiece };
				board.pieces |= ((U64)landBit) << (player ? 32 : 0);	 // add arrival piece
				board.pieces &= ~(((U64)landBit) << (player ? 0 : 32)); // possible take piece
				const U32 beforeKingPieces = (board.pieces >> (player ? 32 : 0)) & (isKingMove ? landBit - 1 : beforeKingMask);
				board.pieces |= ((U64)_popcnt32(beforeKingPieces)) << INDEX_KINGS[player];
				board.pieces |= ((U64)_popcnt32(opponentBeforeKingPieces & ~landBit)) << INDEX_KINGS[!player];
				const bool finished = landBit & endMask;
				// end of movegen
				// beginning of negamax
				Score childScore = -board.search(gameCards, depth - 1, finished, -beta, -alpha);
				if (childScore > bestScore) {
					bestScore = childScore;
					if (childScore > alpha) {
						alpha = bestScore;
						if (alpha >= beta)
							goto pruneLoop;
					}
				}
				// end of negamax
				// continue movegen
			}
			kingPieceNum--;
		}
	}
	pruneLoop:

	return bestScore;
}

#include "Board.h"
#include <algorithm>


SearchResult Board::search(const GameCards& gameCards, S32 depth, Score alpha, const Score beta, const bool quiescent) const {
	// negamax with alpha beta pruning
	bool player = pieces & MASK_TURN;

	depth--;

	U64 total = 0;

	Board bestBoard;
	Score bestScore = SCORE_MIN;

	bool foundAny = false;
	
	// Ive spent too many hours trying to get generator functions to compile since i want lazy move generation
	// fuck it, just copy pasting the movegen in here
	const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
	U64 piecesWithoutCards = pieces & ~MASK_CARDS;
	piecesWithoutCards ^= MASK_TURN; // invert player bit
	for (int taking = 1; taking >= (quiescent ? 1 : 0); taking--) {
		U32 cardStuff = cardsPos.players[player];
		const U32 moveMask = ~(pieces >> (player ? 32 : 0)) & ((pieces >> (player ? 0 : 32)) ^ (taking ? 0 : ~0));
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
				U32 scan = moveBoard[fromI] & moveMask;
				const U32 fromBit = (1ULL << fromI);
				bitScan -= fromBit;
				U64 newPiecesWithoutLandPiece = piecesWithNewCards & ~(((U64)fromBit) << (player ? 32 : 0));
				bool isKingMove = !kingPieceNum;

				const U32 endMask = opponentKing | (isKingMove ? MASK_END_POSITIONS[player] : 0);
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
					foundAny = true;
					// end of movegen
					// beginning of negamax

					Score childScore;
					if (finished) {

						childScore = SCORE_WIN + depth;
						total++;
					} else if (!depth || quiescent) {
						const auto& childSearch = board.search(gameCards, depth, -beta, -alpha, true);
						childScore = -childSearch.score;
						total += childSearch.total;
					} else {
						const auto& childSearch = board.search(gameCards, depth, -beta, -alpha, false);
						childScore = -childSearch.score;
						total += childSearch.total;
					}

					if (childScore > bestScore) {
						bestScore = childScore;
						bestBoard = board;
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
	}
	pruneLoop:

	if (!foundAny && quiescent)
		return { (player ? -1 : 1) * eval(gameCards), 0, 1 };

	return { bestScore, bestBoard, total };
}

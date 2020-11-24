
#include "Board.h"
#include <algorithm>
#include <chrono>

#include "TableBase.h"


SearchResult Board::search(const GameCards& gameCards, S32 maxDepth, const bool softTB, Score alpha, const Score beta, const bool quiescent) const {
	// negamax with alpha beta pruning
	bool player = pieces & MASK_TURN;

	maxDepth--;

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
		const U32 takeMask = ~(pieces >> (player ? 32 : 0)) & (pieces >> (player ? 0 : 32) ^ (taking ? 0 : ~0));
		const U32 kingMask = ~(pieces >> (player ? 32 : 0)) & ((pieces >> (player ? 0 : 32) | MASK_END_POSITIONS[player]) ^ (taking ? 0 : ~0));
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
				bool isKingMove = !kingPieceNum;
				U32 scan = moveBoard[fromI] & (isKingMove ? kingMask : takeMask);
				const U32 fromBit = (1ULL << fromI);
				bitScan -= fromBit;
				U64 newPiecesWithoutLandPiece = piecesWithNewCards & ~(((U64)fromBit) << (player ? 32 : 0));

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
						childScore = (maxDepth >= -1 ? SCORE_WIN : SCORE_WIN - SCORE_QUIESCENCE_WIN_OFFSET) + maxDepth;
						total++;

					} else {
						bool TBHit;
						if (_popcnt32(board.pieces & MASK_PIECES) <= 3 && _popcnt64(board.pieces & (MASK_PIECES << 32)) <= 3) {
							const int8_t result = TableBase::wonBoards[TableBase::compress6Men(board)*2+player];
							TBHit = result != 127;
							if (TBHit) {
								//uint16_t depth = ((uint16_t)(result > 0 ? result : -result) << 3) | 7;
								uint16_t depth = (((uint16_t)(result > 0 ? result : -result)) << 3);
								//board.print(gameCards);
								//std::cout << "win in " << depth << " for " << (result > 0 ? "blue" : "red");
								Score score = (quiescent ? SCORE_WIN - SCORE_QUIESCENCE_WIN_OFFSET : SCORE_WIN) + maxDepth - depth;
								if (result < 0 == player)
									childScore = score;
								else
									childScore = -score;
								//std::cout << ' ' << childScore << std::endl;
							}
						} else
							TBHit = false;
						if (softTB || !TBHit) {
							const auto& childSearch = board.search(gameCards, maxDepth, softTB, -beta, -alpha, !maxDepth || quiescent);
							childScore = -childSearch.score;
							total += childSearch.total;
						}
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



// only works at finding wins for blue, made for TB gen
bool Board::searchWinIn(const GameCards& gameCards, const U16 depth) const {
	bool player = pieces & MASK_TURN;
	if (depth == 1)
		return winInOne(gameCards);


	// same story as before, just copied from there..
	const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
	U64 piecesWithoutCards = pieces & ~MASK_CARDS;
	piecesWithoutCards ^= MASK_TURN; // invert player bit
	for (int taking = 1; taking >= 0; taking--) {
		U32 cardStuff = cardsPos.players[player];
		const U32 takeMask = ~(pieces >> (player ? 32 : 0)) & (pieces >> (player ? 0 : 32) ^ (taking ? 0 : ~0));
		const U32 kingMask = ~(pieces >> (player ? 32 : 0)) & ((pieces >> (player ? 0 : 32) | MASK_END_POSITIONS[player]) ^ (taking ? 0 : ~0));
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
				bool isKingMove = !kingPieceNum;
				U32 scan = moveBoard[fromI] & (isKingMove ? kingMask : takeMask);
				const U32 fromBit = (1ULL << fromI);
				bitScan -= fromBit;
				U64 newPiecesWithoutLandPiece = piecesWithNewCards & ~(((U64)fromBit) << (player ? 32 : 0));

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
					// end of movegen
					// beginning of recursive searchWinIn
					if (finished)
						return true;
					if (board.searchWinIn(gameCards, depth - 1) != player)
						return !player;

					// end of recursive searchWinIn
					// continue movegen
				}
				kingPieceNum--;
			}
		}
	}
	return player;
}





constexpr U32 startDepth = 0;
constexpr U32 minDepth = 4;
SearchResult Board::searchTime(const GameCards& cards, const U64 timeBudget, const int verboseLevel, const int expectedDepth) const {
	if (verboseLevel >= 2)
		print(cards);
	auto lastTime = 1ULL;
	auto predictedTime = 1ULL;
	S32 maxDepth = startDepth;
	S32 shortestEnd = std::numeric_limits<S32>::max();
	SearchResult result;
	while (true) {
		const auto beginTime = std::chrono::steady_clock::now();
		result = search(cards, ++maxDepth, shortestEnd != std::numeric_limits<S32>::max());
		const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
		predictedTime = time * time / lastTime;
		lastTime = time;
		bool foundWin = std::abs(result.score) >= SCORE_WIN - SCORE_QUIESCENCE_WIN_OFFSET / 2;
		bool foundProbableWin = std::abs(result.score) >= SCORE_WIN - SCORE_QUIESCENCE_WIN_OFFSET * 2;
		bool lastIteration = ((predictedTime > timeBudget * 1000) && (maxDepth >= minDepth)) || (maxDepth >= 64);

		if (lastIteration || foundWin || verboseLevel >= 2) {
			bool wrongWinDepth = false;
			S32 end = maxDepth - (std::abs(result.score) - (foundWin ? SCORE_WIN : SCORE_WIN - SCORE_QUIESCENCE_WIN_OFFSET));
			if (foundProbableWin) {
				if (foundWin) {
					shortestEnd = std::min(end, shortestEnd);
					if (expectedDepth >= 0 && end != expectedDepth) {
						print(cards);
						wrongWinDepth = true;
					}
				}
			}
			
			if ((verboseLevel >= 1 && (foundProbableWin ? (shortestEnd - 1) <= maxDepth : true)) || verboseLevel >= 2) {
				if (timeBudget >= 1000)
					printf("depth %2i in %.2fs (%2lluM/s, EBF=%5.2f): ", maxDepth, (float)time / 1E6, result.total / time, std::pow(result.total, 1. / maxDepth));
				else if (timeBudget >= 10)
					printf("depth %2i in %3.0fms (%2lluM/s, EBF=%5.2f): ", maxDepth, (float)time / 1E3, result.total / time, std::pow(result.total, 1. / maxDepth));
				else
					printf("depth %2i in %.1fms (%2lluM/s, EBF=%5.2f): ", maxDepth, (float)time / 1E3, result.total / time, std::pow(result.total, 1. / maxDepth));
			}
			if (foundProbableWin) {
				if ((verboseLevel >= 1 && (shortestEnd - 1) <= maxDepth) || wrongWinDepth || verboseLevel >= 2) {
					std::cout << (result.score > 0 ? "win" : "lose") << " in " << end << (foundWin ? "" : "?");
					if (wrongWinDepth) {
						std::cout << " (expected " << expectedDepth << ")" << std::endl;
						assert(0);
					}
					std::cout << std::endl;
				}
			} else if (verboseLevel >= 1)
				printf("%.2f\n", (float)result.score / SCORE_PIECE);

			if (lastIteration || (shortestEnd - 1) <= maxDepth)
				break;
		}

	}
	if (verboseLevel >= 2)
		std::cout << std::endl;
	return result;
}

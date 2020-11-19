
#include "Board.h"
#include <algorithm>
#include <chrono>


SearchResult Board::search(const GameCards& gameCards, S32 maxDepth, Score alpha, const Score beta, const bool quiescent) const {
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

						childScore = SCORE_WIN + maxDepth;
						total++;
					} else {
						const auto& childSearch = board.search(gameCards, maxDepth, -beta, -alpha, !maxDepth || quiescent);
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
		result = search(cards, ++maxDepth);
		const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
		predictedTime = time * time / lastTime;
		lastTime = time;
		bool foundWin = std::abs(result.score) >= SCORE_WIN - 1;
		bool foundProbableWin = std::abs(result.score) > SCORE_WIN - 64;
		bool lastIteration = ((predictedTime > timeBudget * 1000) && (maxDepth >= minDepth)) || (maxDepth >= 64);
		if (lastIteration || foundWin || verboseLevel >= 2) {
			if (verboseLevel >= 1) {
				if (timeBudget >= 1000)
					printf("depth %2i in %.2fs (%2lluM/s, EBF=%.2f): ", maxDepth, (float)time / 1E6, result.total / time, std::pow(result.total, 1. / maxDepth));
				else if (timeBudget >= 10)
					printf("depth %2i in %3.0fms (%2lluM/s, EBF=%.2f): ", maxDepth, (float)time / 1E3, result.total / time, std::pow(result.total, 1. / maxDepth));
				else
					printf("depth %2i in %.1fms (%2lluM/s, EBF=%.2f): ", maxDepth, (float)time / 1E3, result.total / time, std::pow(result.total, 1. / maxDepth));
			}
			if (foundProbableWin) {
				S32 end = maxDepth - (std::abs(result.score) - SCORE_WIN);
				bool quiescenceUnsure = (end - 1) > maxDepth;
				bool wrongWinDepth = false;
				if (!quiescenceUnsure) {
					shortestEnd = std::min(end, shortestEnd);
					if (expectedDepth >= 0 && end != expectedDepth) {
						print(cards);
						wrongWinDepth = true;
					}
				}
				if (verboseLevel >= 1 || wrongWinDepth) {
					std::cout << (result.score > 0 ? "win" : "lose") << " in " << end << (quiescenceUnsure ? "?" : "");
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

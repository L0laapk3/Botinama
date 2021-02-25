
#include "Game.h"
#include <algorithm>
#include <chrono>

#include "TableBase.h"
#include "MoveGenerator.h"


SearchResult Game::search(Board& board, U8 maxDepth, const bool quiescent, Score alpha, const Score beta) {
	return quiescent ? search<true>(board, maxDepth, alpha, beta) : search<false>(board, maxDepth, alpha, beta);
}

template<bool quiescent>
SearchResult Game::search(Board& board, U8 maxDepth, Score alpha, const Score beta) {
	maxDepth--;
	U64 total = 1;
	Board const* bestBoard = &board;
	auto type = TranspositionTable::EntryType::Alpha;
	bool first = true;

	const auto iterateBoard = [&](Board& newBoard) {
		Score childScore = SCORE_MIN;
		if (newBoard.isEnd) {
			childScore = SCORE_WIN + maxDepth;
			total++;
		} else {
			bool TBHit = false;
#ifdef USE_TB
			U32 myCount = _popcnt32(newBoard.pieces & MASK_PIECES), otherCount = _popcnt64(newBoard.pieces & (MASK_PIECES << 32));
			if (myCount <= (TB_MEN + 1) / 2 && otherCount <= (TB_MEN + 1) / 2) {
				if (TB_MEN % 1 == 0 || myCount + otherCount <= TB_MEN) {
					const int8_t result = board.turn ? (int8_t)(*tableBase.table)[TableBase::compress6Men(newBoard)] : -(int8_t)(*tableBase.table)[TableBase::invertCompress6Men(newBoard)];
					if (result != 0) {
						childScore = SCORE_WIN + maxDepth - std::abs(result) * 2 + (board.turn != result < 0);
						if (result < 0 != board.turn)
							childScore *= -1;
						TBHit = true;
					}
				}
			}
#endif
			if (!TBHit) {
				SearchResult childSearch;
				//if (first) // PVS
					childSearch = search(newBoard, maxDepth, !maxDepth || quiescent, -beta, -alpha);
				//else {
				//	childSearch = search(newBoard, maxDepth, !maxDepth || quiescent, -alpha - 1, -alpha);
				//	if (alpha < -childSearch.score && -childSearch.score < beta) // PVS failed
				//		childSearch = search(newBoard, maxDepth, !maxDepth || quiescent, -beta, childSearch.score);
				//}
				childScore = -childSearch.score;
				total += childSearch.total;
			}
		}
		first = false;
		
		// std::cout << "node " << childScore << std::endl;

		if (childScore > alpha)
			bestBoard = &newBoard;
		if (childScore >= beta) {
			type = TranspositionTable::EntryType::Beta;
			alpha = beta;
			return true;
		}
		if (childScore > alpha) {
			type = TranspositionTable::EntryType::Exact;
			alpha = childScore;
		}
		return false;
	};


	U64 historyBest = 0;
	TranspositionTable::Entry* history;
	if (quiescent) {
		Score standingPat = board.eval(cards);
		if (standingPat >= beta)
			return { beta, *bestBoard, 1 };
		if (alpha < standingPat)
			alpha = standingPat;
	} else {
#ifdef USE_TT
		history = transpositionTable.get(board);
		if (history != nullptr && history->depth >= maxDepth && history->bestMove) { // tt hit
			Score historyScore = history->score;
			if (std::abs(historyScore) > SCORE_WINNING_TRESHOLD)
				history->score += std::copysign(maxDepth / 2, history->score);
			if (history->type == TranspositionTable::EntryType::Exact)
				return { historyScore, history->bestMove, total };
			if (history->type == TranspositionTable::EntryType::Alpha && historyScore <= alpha)
				return { alpha, history->bestMove, total };
			if (history->type == TranspositionTable::EntryType::Beta && historyScore >= beta)
				return { beta, history->bestMove, total };
			historyBest = history->bestMove;
			bestBoard = Board{ historyBest };	
			if (iterateBoard(bestBoard))
				#pragma clang diagnostic ignored "-Wmicrosoft-goto"
				goto prune;
		}
#endif
	}


	MoveGenerator moveGen(*this, board, quiescent);

	auto it = moveGen.boards.begin();
	while (it != moveGen.end) {
		Board& newBoard = *it;
#ifdef USE_TT
		if (newBoard.pieces == historyBest)
			continue;
#endif
					
		if (iterateBoard(newBoard))
			goto prune;
	}
	prune:

#ifdef USE_TT
	if (!quiescent)
		transpositionTable.add(board, bestBoard, std::abs(alpha) > SCORE_WINNING_TRESHOLD ? alpha - std::copysign(maxDepth / 2, alpha) : alpha, maxDepth, type);
#endif
	return { alpha, *bestBoard, total };
}




SearchResult Game::search(U8 depth, Score alpha, Score beta) {
	return search(board, depth, false, alpha, beta);
}

void Game::bench(U8 depth) {
	const auto beginTime = std::chrono::steady_clock::now();
	auto result = search(depth);
	const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
	printf("depth %2i in %.2fs (%9llu, %2lluM/s, EBF=%5.2f): ", depth, (float)time / 1E6, result.total, result.total / time, std::pow(result.total, 1. / depth));
}


constexpr U32 minDepth = 4;
SearchResult Game::searchTime(Board& board, const U64 timeBudget, const float panicScale, const int verboseLevel, const U8 expectedDepth) {
	++turn;
	if (verboseLevel >= 3)
		board.print(cards);
	auto lastTime = 1ULL;
	auto predictedTime = 1ULL;
	const U8 startDepth = std::max(0, lastDepth - 4);
	U8 depth = startDepth;
	SearchResult result;
	U64 lastTotal = 1;
	while (true) {
		const auto beginTime = std::chrono::steady_clock::now();
		++depth;
		result = search(depth);
		const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
		predictedTime = time * time / lastTime;
		lastTime = time;
		bool foundWin = std::abs(result.score) >= SCORE_WINNING_TRESHOLD;
		bool isLoss = result.score < 0;

		U8 end = SCORE_WIN - std::abs(result.score) + depth;
		bool exhaustedSearch = lastTotal == result.total;
		lastTotal = result.total;

		bool lastIteration =
			((predictedTime > timeBudget * 1000 * (std::abs(result.score - lastScore) >= SCORE_PIECE / 4 && !foundWin ? 5 : 1)) && (depth > startDepth + 1))
			|| (depth >= 63)
			|| exhaustedSearch
			|| (foundWin && (end - 1 >= depth));

		if ((verboseLevel >= 1 && lastIteration) || verboseLevel >= 2) {
			if (result.score == SCORE_MIN) {
				printf("No moves found.\n");
				return result;
			}
			if (timeBudget >= 1000)
				printf("%2i. depth %2i in %.2fs (%9llu, %2lluM/s, EBF=%5.2f): ", turn, depth, (float)time / 1E6, result.total, result.total / time, std::pow(result.total, 1. / depth));
			else if (timeBudget >= 10)
				printf("%2i. depth %2i in %3.0fms (%9llu, %2lluM/s, EBF=%5.2f): ", turn, depth, (float)time / 1E3, result.total, result.total / time, std::pow(result.total, 1. / depth));
			else
				printf("%2i. depth %2i in %.1fms (%9llu, %2lluM/s, EBF=%5.2f): ", turn, depth, (float)time / 1E3, result.total, result.total / time, std::pow(result.total, 1. / depth));
			if (foundWin) {
				std::cout << (isLoss ? "lose" : "win") << " in " << (U64)end << std::endl;
			} else if (verboseLevel >= 1)
				printf("%+.2f\n", (float)result.score / SCORE_PIECE);
		}
		if (predictedTime <= timeBudget * 1000)
			lastDepth = exhaustedSearch ? 0 : depth;
		if (lastIteration) {
			lastScore = result.score;
			break;
		}
	}
#ifdef USE_TT
	transpositionTable.report();
#endif
	if (verboseLevel >= 2)
		std::cout << std::endl;
	return result;
}



U64 Game::perft(U8 depth) {
	const auto beginTime = std::chrono::steady_clock::now();
	U64 count = perft(board, depth);
	const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
	printf("depth %2i in %5.2fs (%3.0fM/s): %llu\n", depth, (float)time / 1E6, (float)count / time, count);
	return count;
}

U64 Game::perft(Board& board, U8 depth) const {


	// std::cout << (U32)depth << ' ' << board.turn << std::endl;
	// board.print(cards);

	if (depth == 1)
		return 1;

	MoveGenerator moveGen(*this, board, false);

	U64 total = 0;
	auto it = moveGen.boards.begin();
	while (it != moveGen.end) {
		Board& newBoard = *it++;
		if (newBoard.isEnd)
			total++;
		else
			total += perft(newBoard, depth - 1);
	}
	return total;
}




// // only works at finding wins for blue, made for TB gen
// bool Board::searchWinIn(const GameCards& gameCards, const U16 depth) const {
// 	bool player = pieces & MASK_TURN;
// 	if (depth == 1)
// 		return winInOne(gameCards);


// 	// same story as before, just copied from there..
// 	const CardsPos& cardsPos = CARDS_LUT[(pieces & MASK_CARDS) >> INDEX_CARDS];
// 	U64 piecesWithoutCards = pieces & ~MASK_CARDS;
// 	piecesWithoutCards ^= MASK_TURN; // invert player bit
// 	for (int taking = 1; taking >= 0; taking--) {
// 		U32 cardStuff = cardsPos.players[player];
// 		const U32 takeMask = ~(pieces >> (player ? 32 : 0)) & (pieces >> (player ? 0 : 32) ^ (taking ? 0 : ~0));
// 		const U32 kingMask = MASK_END_POSITIONS[player] ^ (taking ? 0 : ~0);
// 		for (int i = 0; i < 2; i++) {
// 			unsigned long cardI = cardStuff & 0xff;
// 			U64 piecesWithNewCards = piecesWithoutCards | (((U64)cardStuff & 0xff00) << (INDEX_CARDS - 8ULL));
// 			cardStuff >>= 16;
// 			const auto& moveBoard = gameCards[cardI].moveBoards[player];
// 			U32 bitScan = (piecesWithNewCards >> (player ? 32 : 0)) & MASK_PIECES;
// 			U32 kingPieceNum = (piecesWithNewCards >> INDEX_KINGS[player]) & 7;
// 			const U32 opponentKingPieceNum = (piecesWithNewCards >> INDEX_KINGS[!player]) & 7;
// 			const U32 beforeKingMask = _pdep_u32(1ULL << kingPieceNum, piecesWithNewCards >> (player ? 32 : 0)) - 1;
// 			const U32 opponentKing = _pdep_u32(1ULL << opponentKingPieceNum, piecesWithNewCards >> (player ? 0 : 32));
// 			const U32 opponentBeforeKingPieces = (piecesWithNewCards >> (player ? 0 : 32)) & (opponentKing - 1);
// 			piecesWithNewCards &= ~(((U64)0b1111111) << INDEX_KINGS[0]);

// 			unsigned long fromI;
// 			while (_BitScanForward(&fromI, bitScan)) {
// 				bool isKingMove = !kingPieceNum;
// 				U32 scan = moveBoard[fromI] & (isKingMove ? kingMask : takeMask);
// 				const U32 fromBit = (1ULL << fromI);
// 				bitScan -= fromBit;
// 				U64 newPiecesWithoutLandPiece = piecesWithNewCards & ~(((U64)fromBit) << (player ? 32 : 0));

// 				const U32 endMask = opponentKing | (isKingMove ? MASK_END_POSITIONS[player] : 0);
// 				while (scan) {
// 					const U32 landBit = scan & -scan;
// 					scan -= landBit;
// 					Board board = Board{ newPiecesWithoutLandPiece };
// 					board.pieces |= ((U64)landBit) << (player ? 32 : 0);	 // add arrival piece
// 					board.pieces &= ~(((U64)landBit) << (player ? 0 : 32)); // possible take piece
// 					const U32 beforeKingPieces = (board.pieces >> (player ? 32 : 0)) & (isKingMove ? landBit - 1 : beforeKingMask);
// 					board.pieces |= ((U64)_popcnt32(beforeKingPieces)) << INDEX_KINGS[player];
// 					board.pieces |= ((U64)_popcnt32(opponentBeforeKingPieces & ~landBit)) << INDEX_KINGS[!player];
// 					const bool finished = landBit & endMask;
// 					// end of movegen
// 					// beginning of recursive searchWinIn
// 					if (finished)
// 						return true;
// 					if (board.searchWinIn(gameCards, depth - 1) != player)
// 						return !player;

// 					// end of recursive searchWinIn
// 					// continue movegen
// 				}
// 				kingPieceNum--;
// 			}
// 		}
// 	}
// 	return player;
// }
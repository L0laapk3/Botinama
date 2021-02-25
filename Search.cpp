
#include "Game.h"
#include <algorithm>
#include <chrono>

#include "TableBase.h"
#include "BitScan.h"



IntermediateSearchResult::IntermediateSearchResult(const IntermediateSearchResult& other) : score(other.score), total(other.total) { }
IntermediateSearchResult::IntermediateSearchResult(Score score, uint32_t halfBoard, uint8_t card, U64 total) :
	score(score),
	total(total) { }

OuterSearchResult::OuterSearchResult(const OuterSearchResult& other) : IntermediateSearchResult(other), halfBoard(other.halfBoard), card(other.card) { }
OuterSearchResult::OuterSearchResult(Score score, uint32_t halfBoard, uint8_t card, U64 total) :
	IntermediateSearchResult(score, halfBoard, card, total),
	halfBoard(halfBoard),
	card(card) { }

SearchResult::SearchResult() : OuterSearchResult(0, 0, 0, 0), cardName(), fromIndex(25), toIndex(25) { }
SearchResult::SearchResult(const Game& game, const OuterSearchResult& result) : OuterSearchResult(result) {
	assert(board.pieces);
	cardName = game.cards[(CARDS_LUT[(game.board.pieces & MASK_CARDS) >> INDEX_CARDS].players[0] >> (card ? 16 : 0)) & 0xff].name;
	fromIndex = 25;
	toIndex = 25;
	_BitScanForward(&fromIndex, game.board.pieces & MASK_PIECES & ~result.halfBoard);
	_BitScanForward(&toIndex, result.halfBoard & ~(game.board.pieces & MASK_PIECES));
}

// SearchResult Game::search(const Board& board, U8 maxDepth, const bool quiescent, Score alpha, const Score beta) {
// 	return quiescent ? search<true>(board, maxDepth, alpha, beta) : search<false>(board, maxDepth, alpha, beta);
// }

// template<bool quiescent>
// SearchResult Game::search(const Board& board, U8 maxDepth, Score alpha, const Score beta) {
// 	bool player = board.pieces & MASK_TURN;
// 	maxDepth--;
// 	U64 total = 1;
// 	Board bestBoard{ 0 };
// 	auto type = TranspositionTable::EntryType::Alpha;
// 	bool first = true;

// 	const auto iterateBoard = [&](const Board& newBoard) {
// 		Score childScore = SCORE_MIN;
// 		if (newBoard.pieces & (1 << INDEX_FINISHED)) {
// 			childScore = SCORE_WIN + maxDepth;
// 			total++;
// 		} else {
// 			bool TBHit = false;
// #ifdef USE_TB
// 			U32 myCount = _popcnt32(newBoard.pieces & MASK_PIECES), otherCount = _popcnt64(newBoard.pieces & (MASK_PIECES << 32));
// 			if (myCount <= (TB_MEN + 1) / 2 && otherCount <= (TB_MEN + 1) / 2) {
// 				if (TB_MEN % 1 == 0 || myCount + otherCount <= TB_MEN) {
// 					const int8_t result = player ? (int8_t)(*tableBase.table)[TableBase::compress6Men(newBoard)] : -(int8_t)(*tableBase.table)[TableBase::invertCompress6Men(newBoard)];
// 					if (result != 0) {
// 						childScore = SCORE_WIN + maxDepth - std::abs(result) * 2 + (player != result < 0);
// 						if (result < 0 != player)
// 							childScore *= -1;
// 						TBHit = true;
// 					}
// 				}
// 			}
// #endif
// 			if (!TBHit) {
// 				SearchResult childSearch;
// 				if (first) // PVS
// 					childSearch = search(newBoard, maxDepth, !maxDepth || quiescent, -beta, -alpha);
// 				else {
// 					childSearch = search(newBoard, maxDepth, !maxDepth || quiescent, -alpha - 1, -alpha);
// 					if (alpha < -childSearch.score && -childSearch.score < beta) // PVS failed
// 						childSearch = search(newBoard, maxDepth, !maxDepth || quiescent, -beta, childSearch.score);
// 				}
// 				childScore = -childSearch.score;
// 				total += childSearch.total;
// 			}
// 		}
// 		first = false;

// 		if (childScore > alpha)
// 			bestBoard = newBoard;
// 		if (childScore >= beta) {
// 			type = TranspositionTable::EntryType::Beta;
// 			alpha = beta;
// 			return true;
// 		}
// 		if (childScore > alpha) {
// 			type = TranspositionTable::EntryType::Exact;
// 			alpha = childScore;
// 		}
// 		return false;
// 	};


// 	U64 historyBest = 0;
// 	TranspositionTable::Entry* history;
// 	if (quiescent) {
// 		Score standingPat = (player ? -1 : 1) * board.eval(cards);
// 		if (standingPat >= beta)
// 			return { beta, 0, total };
// 		if (alpha < standingPat)
// 			alpha = standingPat;
// 	} else {
// #ifdef USE_TT
// 		history = transpositionTable.get(board);
// 		if (history != nullptr && history->depth >= maxDepth && history->bestMove) { // tt hit
// 			Score historyScore = history->score;
// 			if (std::abs(historyScore) > SCORE_WINNING_TRESHOLD)
// 				history->score += std::copysign(maxDepth, history->score);
// 			if (history->type == TranspositionTable::EntryType::Exact)
// 				return { historyScore, history->bestMove, total };
// 			if (history->type == TranspositionTable::EntryType::Alpha && historyScore <= alpha)
// 				return { alpha, history->bestMove, total };
// 			if (history->type == TranspositionTable::EntryType::Beta && historyScore >= beta)
// 				return { beta, history->bestMove, total };
// 			historyBest = history->bestMove;
// 			bestBoard = Board{ historyBest };	
// 			if (iterateBoard(bestBoard))
// 				#pragma clang diagnostic ignored "-Wmicrosoft-goto"
// 				goto prune;
// 		}
// #endif
// 	}

	
// 	// iterate moves
// 	const CardsPos& cardsPos = CARDS_LUT[(board.pieces & MASK_CARDS) >> INDEX_CARDS];
// 	U64 piecesWithoutCards = board.pieces & ~MASK_CARDS;
// 	piecesWithoutCards ^= MASK_TURN; // invert player bit
// 	for (int taking = 1; taking >= (quiescent ? 1 : 0); taking--) {
// 		U32 cardStuff = cardsPos.players[player];
// 		const U32 takeMask = ~(board.pieces >> (player ? 32 : 0)) & (board.pieces >> (player ? 0 : 32) ^ (taking ? 0 : ~(U32)0));
// 		const U32 kingMask = ~(board.pieces >> (player ? 32 : 0)) & ((board.pieces >> (player ? 0 : 32) | MASK_END_POSITIONS[player]) ^ (taking ? 0 : ~(U32)0));
// 		for (int i = 0; i < 2; i++) {
// 			unsigned long cardI = cardStuff & 0xff;
// 			U64 piecesWithNewCards = piecesWithoutCards | (((U64)cardStuff & 0xff00) << (INDEX_CARDS - 8ULL));
// 			cardStuff >>= 16;
// 			const auto& moveBoard = cards[cardI].moveBoards[player];
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
// 					Board newBoard = Board{ newPiecesWithoutLandPiece };
// 					newBoard.pieces |= ((U64)landBit) << (player ? 32 : 0);	 // add arrival piece
// 					newBoard.pieces &= ~(((U64)landBit) << (player ? 0 : 32)); // possible take piece
// 					const U32 beforeKingPieces = (newBoard.pieces >> (player ? 32 : 0)) & (isKingMove ? landBit - 1 : beforeKingMask);
// 					newBoard.pieces |= ((U64)_popcnt32(beforeKingPieces)) << INDEX_KINGS[player];
// 					newBoard.pieces |= ((U64)_popcnt32(opponentBeforeKingPieces & ~landBit)) << INDEX_KINGS[!player];
// 					newBoard.pieces |= landBit & endMask ? 1 << INDEX_FINISHED : 0;
// #ifdef USE_TT
// 					if (newBoard.pieces == historyBest)
// 						continue;
// #endif
					
// 					if (iterateBoard(newBoard))
// 						goto prune;
// 				}
// 				kingPieceNum--;
// 			}
// 		}
// 	}
// 	prune:

// #ifdef USE_TT
// 	if (!quiescent)
// 		transpositionTable.add(board, bestBoard, std::abs(alpha) > SCORE_WINNING_TRESHOLD ? alpha - std::copysign(maxDepth, alpha) : alpha, maxDepth, type);
// #endif
// 	return { alpha, bestBoard, total };
// }



template<bool firstDepth>
typename std::conditional<firstDepth, OuterSearchResult, IntermediateSearchResult>::type Game::search(const MoveTable::Move& currHalfBoard, const uint32_t currHalfAddress, const MoveTable::Move& otherHalfBoard, const U8 cardsI, U8 maxDepth, const bool quiescent, Score alpha, const Score beta) {
	return quiescent ? search<firstDepth, true>(currHalfBoard, currHalfAddress, otherHalfBoard, cardsI, maxDepth, alpha, beta) : search<firstDepth, false>(currHalfBoard, currHalfAddress, otherHalfBoard, cardsI, maxDepth, alpha, beta);
}


template<bool firstDepth, bool quiescent>
typename std::conditional<firstDepth, OuterSearchResult, IntermediateSearchResult>::type Game::search(const MoveTable::Move& currHalfBoard, const uint32_t currHalfAddress, const MoveTable::Move& otherHalfBoard, const U8 cardsI, U8 maxDepth, Score alpha, const Score beta) {
	maxDepth--;
	U64 total = 1;
	auto type = TranspositionTable::EntryType::Alpha;
	bool first = true;
	uint32_t bestHalfBoard = 0;
	uint8_t bestCard;

	const std::array<U8, 2> newCardsBothI = {{ (U8)((CARDS_LUT_FLIP[cardsI].players[0] >> 8) & 0xff), (U8)((CARDS_LUT_FLIP[cardsI].players[0] >> 24) & 0xff) }};
	const auto iterateBoard = [&](const MoveTable::Move& newHalfBoard, const U8 card, const uint32_t otherHalfAddress) {
		const auto& newCardsI = newCardsBothI[card];
		Score childScore = SCORE_MIN;
		if (newHalfBoard.halfScore >= SCORE_WINNING_TRESHOLD) {
			childScore = SCORE_WIN + maxDepth;
			total++;
		} else {
			bool TBHit = false;
#ifdef USE_TB
			U32 myCount = _popcnt32(newBoard.pieces & MASK_PIECES), otherCount = _popcnt64(newBoard.pieces & (MASK_PIECES << 32));
			if (myCount <= (TB_MEN + 1) / 2 && otherCount <= (TB_MEN + 1) / 2) {
				if (TB_MEN % 1 == 0 || myCount + otherCount <= TB_MEN) {
					const int8_t result = player ? (int8_t)(*tableBase.table)[TableBase::compress6Men(newBoard)] : -(int8_t)(*tableBase.table)[TableBase::invertCompress6Men(newBoard)];
					if (result != 0) {
						childScore = SCORE_WIN + maxDepth - std::abs(result) * 2 + (player != result < 0);
						if (result < 0 != player)
							childScore *= -1;
						TBHit = true;
					}
				}
			}
#endif
			if (!TBHit) {
#ifdef USE_PVS
				auto childSearch = search<false>(otherHalfBoard, otherHalfAddress, newHalfBoard, newCardsI, maxDepth, !maxDepth || quiescent, first ? -beta : -alpha - 1, -alpha);
				if (!first && alpha < -childSearch.score && -childSearch.score < beta) { // PVS failed
					auto childSearch2 = search<false>(otherHalfBoard, otherHalfAddress, newHalfBoard, newCardsI, maxDepth, !maxDepth || quiescent, -beta, childSearch.score);
					childScore = -childSearch2.score;
					total += childSearch2.total;
				} else {
					childScore = -childSearch.score;
					total += childSearch.total;
				}
#else
				auto childSearch = search<false>(otherHalfBoard, otherHalfAddress, newHalfBoard, newCardsI, maxDepth, !maxDepth || quiescent, -beta, -alpha);
				childScore = -childSearch.score;
				total += childSearch.total;
#endif
			}
		}
		first = false;

		if (childScore > alpha)
			bestHalfBoard = newHalfBoard.bitboard;
			bestCard = card;
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
		Score standingPat = currHalfBoard.halfScore - otherHalfBoard.halfScore;
		if (standingPat >= beta)
			return { beta, 0, 0, total };
		if (alpha < standingPat)
			alpha = standingPat;
	} else {
#ifdef USE_TT
		history = transpositionTable.get(board);
		if (history != nullptr && history->depth >= maxDepth && history->bestMove) { // tt hit
			Score historyScore = history->score;
			if (std::abs(historyScore) > SCORE_WINNING_TRESHOLD)
				history->score += std::copysign(maxDepth, history->score);
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

	const auto& moveList = (*moveTable.table)[currHalfAddress].moves[CARDS_ONE_SIDE[0][cardsI]];
	for (auto moveIt = moveList.begin(); moveIt < moveList.end(); moveIt++) {
		if (moveIt->addressCard == 0)
			break;	
		U32 takenPiece = moveIt->bitboard & otherHalfBoard.bitboard_flipped;
		bool finished = moveIt->halfScore >= SCORE_WINNING_TRESHOLD;
		uint32_t otherHalfAddress = otherHalfBoard.address;
		if (!finished) {
			if (takenPiece) {
				uint32_t prevOtherBitBoard = otherHalfBoard.bitboard;
				// std::cout << (_popcnt32((takenPiece - 1) & otherHalfBoard->bitboard_flipped)) << ' ' << otherHalfBoard->afterTakeBoards[_popcnt32((takenPiece - 1) & otherHalfBoard->bitboard_flipped)] << std::endl;
				otherHalfAddress = otherHalfBoard.afterTakeBoards[_popcnt32((takenPiece - 1) & otherHalfBoard.bitboard_flipped)];
				// std::cout << std::bitset<25>(prevOtherBitBoard) << std::endl << std::bitset<25>(otherHalfBoard->bitboard) << std::endl << std::bitset<25>(takenPiece) << std::endl;
			} else if (quiescent)
				continue;
		}

		if (iterateBoard(*moveIt, moveIt->card, otherHalfAddress))
			goto prune;
	}

	prune:

#ifdef USE_TT
	if (!quiescent)
		transpositionTable.add(board, bestBoard, std::abs(alpha) > SCORE_WINNING_TRESHOLD ? alpha - std::copysign(maxDepth, alpha) : alpha, maxDepth, type);
#endif
	return { alpha, bestHalfBoard, bestCard, total };
}



SearchResult Game::search(U8 depth) {
	// board.print(cards);
    auto halfBoards = moveTable.toHalfBoards(board);
	U8 cardsI = (board.pieces & MASK_CARDS) >> INDEX_CARDS;
    return SearchResult(*this, search<true, false>(halfBoards[0], halfBoards[0].address, halfBoards[1], cardsI, depth, SCORE_MIN, SCORE_MAX));
}








constexpr U32 minDepth = 4;
constexpr U32 maxDepth = 1;
SearchResult Game::searchTime(const U64 timeBudget, const float panicScale, const int verboseLevel, const U8 expectedDepth) {
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

		bool lastIteration = ((predictedTime > timeBudget * 1000 * (std::abs(result.score - lastScore) >= SCORE_PIECE / 4 && !foundWin ? panicScale : 1.f)) && (depth > startDepth + 1)) || (depth >= maxDepth) || exhaustedSearch;

		if ((verboseLevel >= 1 && lastIteration) || verboseLevel >= 2) {
			if (result.score == SCORE_MIN) {
				printf("No moves found.");
				return result;
			}
			if (timeBudget >= 1000)
				printf("%2i. depth %2i in %.2fs (%2lluM/s, EBF=%5.2f): ", turn, depth, (float)time / 1E6, result.total / time, std::pow(result.total, 1. / depth));
			else if (timeBudget >= 10)
				printf("%2i. depth %2i in %3.0fms (%2lluM/s, EBF=%5.2f): ", turn, depth, (float)time / 1E3, result.total / time, std::pow(result.total, 1. / depth));
			else
				printf("%2i. depth %2i in %.1fms (%2lluM/s, EBF=%5.2f): ", turn, depth, (float)time / 1E3, result.total / time, std::pow(result.total, 1. / depth));
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


void Game::bench(const U8 depth) {
	std::cout << "bench started" << std::endl;
	const auto beginTime = std::chrono::steady_clock::now();
	auto result = search(depth);
	const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
	printf("%10llu nodes at depth %2i in %.2fs (%2lluM/s, EBF=%5.2f): ", result.total, depth, (float)time / 1E6, result.total / time, std::pow(result.total, 1. / depth));
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
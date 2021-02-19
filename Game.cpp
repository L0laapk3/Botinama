#include "Game.h"

#include "TableBase.h"

#include <bitset>


Game::Game(std::array<std::string, 5> cardNames, std::string boardString, bool player, bool flipped) :
	Game(CardBoard::fetchGameCards(cardNames, flipped), Board::fromString(boardString, player, flipped)) { }

Game::Game(const GameCards cards, Board board) :
	cards(cards),
	board(board),
	TBThread(TableBase::generate, cards, 4),
	moveTable(cards) {
    TBThread.join();
}
// Game::Game(const GameCards cards, Board board) : cards(cards), board(board), moveTable(cards) { }




template<bool quiescent>
SearchResult Game::search(S32 maxDepth, HalfInfo* currHalfBoard, HalfInfo* otherHalfBoard, const CardsPos& cardsPos, Score alpha, const Score beta) const {
	// negamax with alpha beta pruning

	maxDepth--;

	U64 total = 0;

	uint32_t bestBitboard;
    uint32_t bestCardI;
	Score bestScore = SCORE_MIN;

	if (quiescent) {
		Score standingPat = currHalfBoard->score - otherHalfBoard->score;
		if (standingPat >= beta)
			return { beta, 0, 0, total };
		if (alpha < standingPat)
			alpha = standingPat;
	}

	// std::cout << std::bitset<25>(otherHalfBoard->bitboard_flipped) << ' ' << std::bitset<25>(currHalfBoard->bitboard_flipped) << std::endl;
	// std::cout << std::bitset<25>(otherHalfBoard->kingbit_flipped) << ' ' << std::bitset<25>(currHalfBoard->kingbit_flipped) << std::endl;

	bool foundAny = false;
    U32 cardStuff = cardsPos.players[0];
	// std::cout << std::bitset<25>(currHalfBoard->bitboard) << " " << (cardStuff & 0xff) << " " << ((cardStuff >> 16) & 0xff) << std::endl;
    for (U32 j = 0; j < 2; j++) {
        U32 cardI = cardStuff & 0xff;
	    const CardsPos& newCardsPos = CARDS_LUT_FLIP[(cardStuff >> 8) & 0xff];
        cardStuff >>= 16;
        for (auto it = currHalfBoard->moves[cardI].begin(); it < currHalfBoard->moves[cardI].begin() + 20; it++) {
            if (*it == 0)
                break;
            HalfInfo* newHalfBoard = &(*moveTable.moves)[*it];

			// std::cout << std::bitset<25>(newHalfBoard->bitboard) << ' ' << std::bitset<25>(otherHalfBoard->kingbit_flipped) << std::endl;

            bool finished = newHalfBoard->score == SCORE_WIN;
            if (!finished) {
                U32 takenPiece = newHalfBoard->bitboard & otherHalfBoard->bitboard_flipped;
                if (takenPiece) {
                    finished = newHalfBoard->bitboard & otherHalfBoard->kingbit_flipped;
					if (!finished) {
						uint32_t prevOtherBitBoard = otherHalfBoard->bitboard;
						// std::cout << (_popcnt32((takenPiece - 1) & otherHalfBoard->bitboard_flipped)) << ' ' << otherHalfBoard->afterTakeBoards[_popcnt32((takenPiece - 1) & otherHalfBoard->bitboard_flipped)] << std::endl;
						otherHalfBoard = &(*moveTable.moves)[otherHalfBoard->afterTakeBoards[_popcnt32((takenPiece - 1) & otherHalfBoard->bitboard_flipped)]];
						// std::cout << std::bitset<25>(prevOtherBitBoard) << std::endl << std::bitset<25>(otherHalfBoard->bitboard) << std::endl << std::bitset<25>(takenPiece) << std::endl;
					}
                } else if (quiescent) {
                	total++;
                    continue;
				}
            }
            Score childScore = SCORE_MIN;
            if (finished) {
                childScore = SCORE_WIN + maxDepth;
                total++;
            } else {
                // bool TBHit = false;
                // if (_popcnt32(board.pieces & MASK_PIECES) <= 3 && _popcnt64(board.pieces & (MASK_PIECES << 32)) <= 3) {
                //     const U32 boardComp = TableBase::compress6Men(board)*2+player;
                //     const int8_t result = TableBase::wonBoards[boardComp];
                //     if (result != 0) {
                //         uint16_t depth = (result > 0 ? result : -result) << 1;
                //         Score score = SCORE_WIN + maxDepth - depth;
                //         if (result < 0 == player)
                //             childScore = score;
                //         else
                //             childScore = -score;
                //         TBHit = true;
                //     }
                // }
                // if (!TBHit) {
					if (maxDepth > 0) {
						const auto& childSearch = !maxDepth || quiescent ? search<true>(maxDepth, otherHalfBoard, newHalfBoard, newCardsPos, -beta, -alpha) : search<false>(maxDepth, otherHalfBoard, newHalfBoard, newCardsPos, -beta, -alpha);
						childScore = -childSearch.score;
						total += childSearch.total;
					} else
						total++;
                // }
            }

            // if (quiescent) {
            //     if (childScore >= beta)
            //         return { beta, newHalfBoard->bitboard, cardI, total };
            //     if (childScore > alpha)
            //         alpha = childScore;
            // } else {
            //     if (childScore > bestScore) {
            //         bestScore = childScore;
            //         bestBitboard = newHalfBoard->bitboard;
            //         bestCardI = cardI;
            //         if (childScore > alpha) {
            //             alpha = bestScore;
            //             if (alpha > beta)
            //                 goto pruneLoop;
            //         }
            //     }
            // }
        }
    }
	pruneLoop:

	if (quiescent)
		return { alpha, 0, 0, total };
	else
		return { bestScore, bestBitboard, bestCardI, total };
}


SearchResult Game::search(S32 maxDepth) const {
	// board.print(cards);
    auto addresses = board.toAddresses(moveTable);
	const CardsPos& cardsPos = CARDS_LUT_FLIP[(board.pieces & MASK_CARDS) >> INDEX_CARDS];
    return search<false>(maxDepth, &(*moveTable.moves)[addresses[0]], &(*moveTable.moves)[addresses[1]], cardsPos, SCORE_MIN, SCORE_MAX);
}


U64 Game::perft(S32 maxDepth) const {
	SearchResult result;
	S32 depth = 0;
	for (S32 depth = 1; depth <= maxDepth; depth++) {
		result = search(depth);
		std::cout << depth << ": " << result.total << std::endl;
	}
	return result.total;
}


constexpr U32 startDepth = 0;
constexpr U32 minDepth = 4;
SearchResult Game::searchTime(const U64 timeBudget, const int verboseLevel, const int expectedDepth) const {
	if (verboseLevel >= 2)
		board.print(cards);
	auto lastTime = 1ULL;
	auto predictedTime = 1ULL;
	S32 depth = startDepth;
	S32 shortestEnd = std::numeric_limits<S32>::max();
	SearchResult result;
	while (true) {
		const auto beginTime = std::chrono::steady_clock::now();
		++depth;
		result = search(depth);
		const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
		predictedTime = time * time / lastTime;
		lastTime = time;
		bool foundWin = std::abs(result.score) >= SCORE_WIN - SCORE_QUIESCENCE_WIN_OFFSET / 2;
		bool foundProbableWin = std::abs(result.score) >= SCORE_WIN - SCORE_QUIESCENCE_WIN_OFFSET * 2;

		bool wrongWinDepth = false;
		S32 end = depth - (std::abs(result.score) - (foundWin ? SCORE_WIN : SCORE_WIN - SCORE_QUIESCENCE_WIN_OFFSET));
		if (foundWin) {
			shortestEnd = std::min(end, shortestEnd);
			if (expectedDepth >= 0 && end != expectedDepth) {
				board.print(cards);
				wrongWinDepth = true;
			}
		}

		bool TBWin = depth + 1 < end;
		bool lastIteration = ((predictedTime > timeBudget * 1000) && (depth >= minDepth)) || (depth >= 64) || ((shortestEnd - 1) <= depth) || (foundWin && TBWin && depth <= 2);

		if ((verboseLevel >= 1 && lastIteration) || verboseLevel >= 2) {
			if (timeBudget >= 1000)
				printf("depth %2i in %.2fs (%9llu, %2lluM/s, EBF=%5.2f): ", depth, (float)time / 1E6, result.total, result.total / time, std::pow(result.total, 1. / depth));
			else if (timeBudget >= 10)
				printf("depth %2i in %3.0fms (%9llu, %2lluM/s, EBF=%5.2f): ", depth, (float)time / 1E3, result.total, result.total / time, std::pow(result.total, 1. / depth));
			else
				printf("depth %2i in %.1fms (%9llu, %2lluM/s, EBF=%5.2f): ", depth, (float)time / 1E3, result.total, result.total / time, std::pow(result.total, 1. / depth));
			if (foundProbableWin) {
				bool isLoss = result.score < 0;
				std::cout << (isLoss ? "lose" : "win") << " in " << ((end & ~1) | !isLoss);
				if (!foundWin)
					std::cout << '?';
				// else if (TBWin)
				// 	std::cout << '*';
				if (wrongWinDepth) {
					std::cout << " (expected " << expectedDepth << ")" << std::endl;
					assert(0);
				}
					std::cout << std::endl;
			} else if (verboseLevel >= 1)
				printf("%.2f\n", (float)result.score / SCORE_PIECE);
		}
		if (lastIteration)
			break;
	}
	if (verboseLevel >= 2)
		std::cout << std::endl;
	return result;
}
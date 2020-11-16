
#include "Search.h"


Score Search::minimax(GameCards& gameCards, const Board& board, const bool finished, U32 depth) {
	if (finished || !depth)
		return board.eval(gameCards, finished);
	depth -= 1;

	// player 0 is always maximising. :]
	bool player = board.pieces & MASK_TURN;
	bool maximising = player;
	Score bestScore = player ? SCORE_MAX : SCORE_MIN;
	const auto cb = [&](GameCards& gameCards, const Board& board, const bool finished, U32 depth) {
		Score score = minimax(gameCards, board, finished, depth);
		if (maximising ? score > bestScore : score < bestScore)
			bestScore = score;
	};
	board.forwardMoves<*cb>(gameCards, depth);
	return bestScore;
}
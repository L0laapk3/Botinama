
#include "Solve.h"


U64 count = 0;
void recursive(GameCards& gameCards, const Board& board, const bool finished, U32 depth) {
	/*if (((board.pieces & MASK_CARDS) >> INDEX_CARDS) >= 30) {
		board.print(gameCards, finished);
	}
	board.valid();*/
	//board.print(gameCards, finished);
	if (!depth || finished)
		count++;
	else
		board.reverseMoves<*recursive>(gameCards, depth - 1);
}
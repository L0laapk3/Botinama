
#include "Board.h"



Score Board::eval(GameCards& gameCards, const bool finished) const {
	if (finished)
		return winner() ? SCORE_MAX : SCORE_MIN;
	return _popcnt32(pieces & MASK_PIECES) - _popcnt32((pieces >> 32) & MASK_PIECES);
}
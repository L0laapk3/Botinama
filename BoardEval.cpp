
#include "Board.h"



Score Board::eval(const GameCards& gameCards) const {
	return _popcnt32(pieces & MASK_PIECES) - _popcnt32((pieces >> 32) & MASK_PIECES);
}
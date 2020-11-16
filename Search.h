#pragma once

#include "Board.h"
#include "Score.h"

namespace Search {
	Score minimax(GameCards& gameCards, const Board& board, const bool finished, U32 depth);
}
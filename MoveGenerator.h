#pragma once

#include <array>

#include "Board.h"




class MoveGenerator {
public:
    MoveGenerator(const Game& game, Board& board, const bool quiescent);
    // bool next();
    std::array<Board, 40>::iterator end;
    std::array<Board, 40> boards;
};
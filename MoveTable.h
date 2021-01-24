#pragma once

#include <array>
#include "Botama.h"
#include "Board.h"
#include "Score.h"
#include "Card.h"
#include "CardBoard.h"


namespace MoveTable {
    constexpr std::array<U64, 5> HALF_POSITIONS = { // index is number of pawns on the board
        25ULL,
        25ULL * (24) / (1),
        25ULL * (24*23) / (2*1),
        25ULL * (24*23*22) / (3*2*1),
        25ULL * (24*23*22*21) / (4*3*2*1),
    };

    constexpr U64 TOTAL_HALF_POSITIONS = HALF_POSITIONS[0] + HALF_POSITIONS[1] + HALF_POSITIONS[2] + HALF_POSITIONS[3] + HALF_POSITIONS[4];

    struct Move { // info about one players board pieces including every possible move
        std::array<uint32_t, 40> moves{0};
        std::array<uint32_t, 5> afterTakeBoards{0};
        uint32_t bitboard; // bitboard with all occupied positions
        uint32_t kingbit; // bitboard with king position
        Score score;
    };

    typedef std::array<Move, CARDS_LUT.size() * TOTAL_HALF_POSITIONS> MoveTable;
    std::unique_ptr<MoveTable> build(const GameCards& gameCards);
}
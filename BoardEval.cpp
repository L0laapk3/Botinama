
#include "Board.h"
#include <array>



// +1 point for each square closer to the center
constexpr std::array<U32, 3> SCOREBITS = {
	0b01010'10101'01010'10101'01010ULL,
	0b00100'01110'11011'01110'00100ULL,
	0b00000'00000'00100'00000'00000ULL,
};

Score Board::eval(const GameCards& gameCards) const {
	// piece diff
	Score score = (_popcnt64(pieces & MASK_PIECES) - _popcnt64(pieces & (((U64)MASK_PIECES) << 32))) * SCORE_PIECE;

	// 1 point for every square closer to the center
	score += (_popcnt64(pieces & SCOREBITS[0]) - _popcnt64(pieces & (((U64)SCOREBITS[0]) << 32)));
	score += (_popcnt64(pieces & SCOREBITS[1]) - _popcnt64(pieces & (((U64)SCOREBITS[1]) << 32))) * 2;
	score += ((pieces & SCOREBITS[2]) != 0) - ((pieces & (((U64)SCOREBITS[2]) << 32)) != 0) * 4;

	return score;
}

#include "Board.h"
#include <array>



// +1 point for each square closer to the center
constexpr std::array<U32, 3> SCOREBITS = {
	0b01010'10101'01010'10101'01010ULL,
	0b00100'01110'11011'01110'00100ULL,
	0b00000'00000'00100'00000'00000ULL,
};

Score Board::eval(const GameCards& gameCards) const {
	return ScoreHalf(pieces[0], kings[0]) - ScoreHalf(pieces[1], kings[1]);
}


Score ScoreHalf(uint32_t bitboard, uint32_t kingBit) {
	Score score = _popcnt32(bitboard) * SCORE_PIECE;
	score += _popcnt32(bitboard & SCOREBITS[0]);
	score += _popcnt32(bitboard & SCOREBITS[1]) * 2;
	score += _popcnt32(bitboard & SCOREBITS[2]) * 4;
	return score;
}
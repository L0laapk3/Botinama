#pragma once
#include <cmath>
#include <limits>

typedef int16_t Score;

constexpr Score SCORE_MAX = (std::numeric_limits<Score>::max)();
constexpr Score SCORE_MIN = -SCORE_MAX;

constexpr Score SCORE_WIN = 1 << 13;
constexpr Score SCORE_QUIESCENCE_WIN_OFFSET = 1 << 11;

constexpr Score SCORE_PIECE = (1 << 4);

Score ScoreHalf(uint32_t bitboard, uint32_t kingBit);
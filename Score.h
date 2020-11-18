#pragma once
#include <cmath>
#include <limits>

typedef int32_t Score;

constexpr Score SCORE_MAX = (std::numeric_limits<Score>::max)();
constexpr Score SCORE_MIN = -SCORE_MAX;

constexpr Score SCORE_WIN = 1 << 29;
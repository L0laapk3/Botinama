#pragma once
#include <array>
#include <cstdint>

// project wide definitions


// #define USE_TT
#define USE_TB

constexpr uint_fast32_t TB_MEN = 4;

typedef uint_fast64_t U64;
typedef uint_fast32_t U32;
typedef uint_fast16_t U16;
typedef uint_fast8_t U8;
typedef int_fast64_t S64;
typedef int_fast32_t S32;
typedef int_fast16_t S16;
typedef int_fast8_t S8;

constexpr U64 INDEX_TURN = 25;
constexpr U64 INDEX_FINISHED = 26;
constexpr U64 MASK_TURN = 1ULL << INDEX_TURN;
constexpr U64 INDEX_CARDS = 27;
constexpr U64 MASK_CARDS = 0x1fULL << INDEX_CARDS;
constexpr U64 MASK_PIECES = 0x1ffffffULL;
constexpr std::array<U64, 2> MASK_PLAYER = { 0xffff'ffffULL, 0xffff'ffff'0000'0000 };
constexpr std::array<U32, 2> INDEX_KINGS = { 32 + 25, 32 + 25 + 4 };
constexpr std::array<U32, 2> INDEX_END_POSITIONS = { 22, 2 };
constexpr std::array<U32, 2> MASK_END_POSITIONS = { 1ULL << INDEX_END_POSITIONS[0], 1ULL << INDEX_END_POSITIONS[1] };

constexpr U64 TBSIZE = 25*25*30*(TB_MEN <= 4 ? 25*25 : 25*13*25*13);

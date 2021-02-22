#pragma once

#include <array>
#include <memory>

#include "Botama.h"
#include "Board.h"
#include "Score.h"


constexpr uint32_t TTSIZE = (1ULL << 29) - 1;


class TranspositionTable {
public:
    enum EntryType : uint64_t {
        Exact,
        Upper,
        Lower
    };
#pragma pack (push)
#pragma pack (1)
    struct Entry {
        uint64_t keyHalf : 40;
        uint64_t depth : 6;
        EntryType type : 2;
        uint64_t score : 16;
        Board bestMove;
    };
#pragma pack (pop)

    TranspositionTable();

    void init();

    U64 collisions = 0;
	std::unique_ptr<std::array<Entry, TTSIZE>> table = nullptr;

    Entry& get(const Board& board);

    void add(const Board& board, const Board& best, const Score& score, const uint8_t depth, const EntryType type);

};
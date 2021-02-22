#include "TranspositionTable.h"


TranspositionTable::TranspositionTable() {};


void TranspositionTable::init() {
    table = std::make_unique<std::array<Entry, TTSIZE>>();
}

TranspositionTable::Entry& TranspositionTable::get(const Board& board) {
    return (*table)[board.pieces % TTSIZE];
}

void TranspositionTable::add(const Board& board, const Board& best, const Score& score, const uint8_t depth, const EntryType type) {
    auto& entry = (*table)[board.pieces % TTSIZE];
    if (entry.keyHalf == (uint32_t)board.pieces)
        collisions++;
    entry.keyHalf = (uint32_t)board.pieces;
    entry.bestMove = best;
    entry.score = score;
    entry.depth = depth;
    entry.type = type;
}
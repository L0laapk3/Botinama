#include "TranspositionTable.h"
#include <functional>

// https://web.archive.org/web/20071031100051/http://www.brucemo.com/compchess/programming/hashing.htm


TranspositionTable::TranspositionTable() {};


void TranspositionTable::init() {
	table = std::make_unique<std::array<Entry, TTSIZE>>();
}


U64 TranspositionTable::doHash(const Board& board) const {
	return board.pieces;//std::hash<U64>{}(board.pieces);
}


TranspositionTable::Entry* TranspositionTable::get(const Board& board) {
	const U64 hash = doHash(board);
	auto* entry = &(*table)[hash % TTSIZE];
	reads++;
	if (entry->type != EntryType::Empty) {
		hits++;
		if (entry->hash == hash)
			return entry;
		else
			mismatches++;
	}
	return nullptr;
}

void TranspositionTable::add(const Board& board, const Board& best, const Score& score, const uint8_t depth, const EntryType type) {
	const U64 hash = doHash(board);
	auto& entry = (*table)[hash % TTSIZE];
	writes++;
	if (entry.type != EntryType::Empty && entry.hash == hash && entry.depth >= depth) {
		if (0)
			return;
	}
	entry.hash = hash;
	entry.bestMove = best;
	entry.score = score;
	entry.depth = depth;
	entry.type = type;
}

void TranspositionTable::report() {
	printf("%4llu writes, %4llu reads, %3llu hits (%.0f%%), %3llu mismatches (%.0f%%)\n", writes, reads, hits, 100.f*hits/reads, mismatches, 100.f*mismatches/hits);
	writes = 0;
	reads = 0;
	hits = 0;
	mismatches = 0;
}
#include "TranspositionTable.h"
#include <functional>

// https://web.archive.org/web/20071031100051/http://www.brucemo.com/compchess/programming/hashing.htm


TranspositionTable::TranspositionTable() {};


void TranspositionTable::init() {
	table = std::make_unique<std::array<Entry, TTSIZE>>();
}


U64 TranspositionTable::doHash(const Board& board) const {
	U64 key = board.pieces;
	key ^= key >> 33;
	key *= 0xff51afd7ed558ccd;
	key ^= key >> 33;
	// key *= 0xc4ceb9fe1a85ec53;
	// key ^= key >> 33;
	return key;
}


TranspositionTable::Entry* TranspositionTable::get(const Board& board) {
	const U64 hash = doHash(board);
	auto* entry = &(*table)[hash % TTSIZE];
	reads++;
	if (entry->type != EntryType::Empty) {
		hits++;
		if (entry->pieces == board.pieces)
			return entry;
		else {
			// std::cout << std::bitset<64>(board.pieces) << ' ' << (hash % TTSIZE) << std::endl << std::bitset<64>(entry->pieces) << std::endl;
			collisions++;
		}
	}
	return nullptr;
}

void TranspositionTable::add(const Board& board, const Board& best, const Score& score, const uint8_t depth, const EntryType type) {
	const U64 hash = doHash(board);
	auto& entry = (*table)[hash % TTSIZE];
	writes++;
	if (entry.type != EntryType::Empty && entry.depth >= depth) {
		if (0) // condition for not replacing
			return;
	}
	entry.pieces = board.pieces;
	entry.bestMove = best.pieces;
	entry.score = score;
	assert(depth < 64);
	entry.depth = depth;
	entry.type = type;
}

void TranspositionTable::report() {
	printf("TT %4llu reads, %4.1f%% hit, %3.1f%% collisions\n", reads, 100.f*hits/std::max<U64>(reads, 1), 100.f*collisions/std::max<U64>(hits, 1));
	writes = 0;
	reads = 0;
	hits = 0;
	collisions = 0;
}
#pragma once

#include <array>
#include <memory>

#include "Botama.h"
#include "Board.h"
#include "Score.h"


constexpr U64 TTSIZE = (1ULL << 29) - 1;
// https://web.archive.org/web/20071031100051/http://www.brucemo.com/compchess/programming/hashing.htm

class TranspositionTable {
public:
	enum EntryType : uint8_t {
		Empty = 0,
		Exact,
		Alpha,
		Beta
	};
#pragma pack (push)
#pragma pack (1)
	struct Entry {
		uint64_t hash;
		uint8_t depth : 6;
		EntryType type : 2;
		Score score;
		Board bestMove;
	};
#pragma pack (pop)

	TranspositionTable();

	void init();

	U64 reads = 0;
	U64 hits = 0;
	U64 writes = 0;
	U64 mismatches = 0;
	std::unique_ptr<std::array<Entry, TTSIZE>> table = nullptr;

	Entry* get(const Board& board);

	void add(const Board& board, const Board& best, const Score& score, const uint8_t depth, const EntryType type);

	void report();

private:
	U64 doHash(const Board& board) const;
};
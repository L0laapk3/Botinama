#pragma once

#include <array>
#include <memory>

#include "Botama.h"
#include "Board.h"
#include "Score.h"


#define TT_STATS

constexpr U64 TTSIZE = (1ULL << 29) - 1;
// https://web.archive.org/web/20071031100051/http://www.brucemo.com/compchess/programming/hashing.htm

constexpr uint8_t ENTRYTYPEBITS = 0b11000000;
class TranspositionTable {
public:
	enum EntryType : uint8_t {
		Empty = 0,
		Exact = 1,
		Alpha = 2,
		Beta  = 3,
	};
#pragma pack (push)
#pragma pack (1)
	struct Entry {
		uint64_t pieces;
		// union {
		// 	uint8_t depthType;
		// 	struct {
				uint8_t depth : 6;
				EntryType type : 2;
		// 	};
		// };
		Score score;
		uint64_t bestMove;
	};
#pragma pack (pop)

	TranspositionTable();
	void init();

#ifdef TT_STATS
	U64 reads = 0;
	U64 hits = 0;
	U64 writes = 0;
	U64 collisions = 0;
#endif
	std::unique_ptr<std::array<Entry, TTSIZE>> table = nullptr;

	Entry* get(const Board& board);

	void add(const Board& board, const Board& best, const Score& score, const uint8_t depth, const EntryType type);

	void report();

private:
	U64 getKey(const Board& board) const;
};
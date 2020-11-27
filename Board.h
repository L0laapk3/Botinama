#pragma once

#include <string>
#include <array>
#include <list>
#include "CardBoard.h"
#include <functional>
#include <bitset>
#include <cassert>
#include <iostream>
#include "BitScan.h"
#include "Botama.h"
#include "Score.h"



class Board;
typedef void (*MoveFunc)(const GameCards& gameCards, const Board& board, const bool finished, const int8_t depthVal, const int threadNum);

struct SearchResult;


class Board {
public:
	U64 pieces;
	bool operator <(const Board& rhs) const {
		return pieces < rhs.pieces;
	}
	bool operator==(const Board& rhs) const {
		return pieces == rhs.pieces;
	}

	static Board fromString(std::string str, bool player, bool flip = false);

	void print(const GameCards& gameCards, bool finished = false, bool verbose = false) const;
	static void print(const GameCards& gameCards, std::vector<Board> board, std::vector<bool> finished = { false }, bool verbose = false);

	void valid() const;
	bool winner() const;
	bool currentPlayer() const;
	Board invert() const;

	uint8_t countForwardMoves(const GameCards& gameCards) const;
	bool winInOne(const GameCards& gameCards) const;
	bool searchWinIn(const GameCards& gameCards, const U16 depth) const;
	uint8_t winInTwo(const GameCards& gameCards) const;

	Score eval(const GameCards& gameCards) const;
private:


	//BoardIter
private:
	template<MoveFunc cb, bool reverse>
	void iterateMoves(const GameCards& gameCards, const MoveBoard& moveBoards, U64 piecesWithNewCards, bool player, const bool createPiece, const int8_t depthVal, const int threadNum) const;
public:
	template<MoveFunc cb>
	void forwardMoves(const GameCards& gameCards, const int8_t depthVal, const int threadNum) const;
	template<MoveFunc cb>
	void reverseMoves(const GameCards& gameCards, const U32 maxMen, const U32 maxMenPerSide, const int8_t depthVal, const int threadNum) const;


	//BoardSearch
	template<bool quiescent>
	SearchResult search(const GameCards& gameCards, S32 maxDepth, Score alpha = SCORE_MIN, const Score beta = SCORE_MAX) const;
	SearchResult searchTime(const GameCards& cards, const U64 timeBudget, const int verboseLevel = 1, const int expectedDepth = -1) const;

};

struct BoardHash {
	uint64_t xorshift(const uint64_t& n, int i) const {
		return n^(n>>i);
	}
	size_t operator()(const Board& b) const {
			uint64_t p = 0x5555555555555555ull; // pattern of alternating 0 and 1
			uint64_t c = 17316035218449499591ull;// random uneven integer constant; 
			return c*xorshift(p*xorshift(b.pieces,32),32);
	}
	// size_t operator()(const Board& b) const {
	// 	return std::hash<U64>()(b.pieces);
	// }
};


struct SearchResult {
	Score score;
	Board board;
	U64 total;
};


#include "BoardIter.hpp"
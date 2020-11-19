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
typedef void (*MoveFunc)(const GameCards& gameCards, const Board& board, const bool finished, uint8_t maxDepth);

struct SearchResult;

class Board {
public:
	U64 pieces;
	bool operator <(const Board& rhs) const {
		return pieces < rhs.pieces;
	}
	bool operator==(const Board& rhs) {
		return pieces == rhs.pieces;
	}


	static Board fromString(std::string str, bool player, bool flip = false);

	void print(const GameCards& gameCards, bool finished = false) const;
	static void print(const GameCards& gameCards, std::vector<Board> board, std::vector<bool> finished = { false });

	void valid() const;
	bool winner() const;
	bool currentPlayer() const;
	Board invert() const;

	uint8_t countForwardMoves(const GameCards& gameCards) const;


	Score eval(const GameCards& gameCards) const;
private:
	Score findImmediateWins(const GameCards& gameCards) const;


	//BoardIter
private:
	template<MoveFunc cb>
	void iterateMoves(const GameCards& gameCards, const MoveBoard& moveBoards, U64 piecesWithNewCards, bool player, uint8_t arg) const;
public:
	template<MoveFunc cb>
	void forwardMoves(const GameCards& gameCards, uint8_t arg) const;
	template<MoveFunc cb>
	void reverseMoves(const GameCards& gameCards, uint8_t arg) const;


	//BoardSearch
	SearchResult search(const GameCards& gameCards, S32 maxDepth, Score alpha = SCORE_MIN, const Score beta = SCORE_MAX, const bool quiescent = false) const;
	SearchResult searchTime(const GameCards& cards, const U64 timeBudget, const int verboseLevel = 1) const;

};

struct SearchResult {
	Score score;
	Board board;
	U64 total;
};


#include "BoardIter.hpp"
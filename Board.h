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
typedef void (*MoveFunc)(const GameCards& gameCards, const Board& board, const bool finished, U32 depth);

struct SearchResult;

class Board {
public:
	U64 pieces;

	static Board fromString(std::string str, bool player, bool flip = false);

	void print(const GameCards& gameCards, bool finished = false) const;
	static void print(const GameCards& gameCards, std::vector<Board> board, std::vector<bool> finished = { false });

	void valid() const;
	bool winner() const;
	bool currentPlayer() const;

	U32 countForwardMoves(const GameCards& gameCards) const;


	Score eval(const GameCards& gameCards) const;
private:
	Score findImmediateWins(const GameCards& gameCards) const;


	//BoardIter
private:
	template<MoveFunc cb>
	void iterateMoves(const GameCards& gameCards, const MoveBoard& moveBoards, U64 piecesWithNewCards, bool player, U32 depth) const;
public:
	template<MoveFunc cb>
	void forwardMoves(const GameCards& gameCards, U32 depth) const;
	template<MoveFunc cb>
	void reverseMoves(const GameCards& gameCards, U32 depth) const;


	//BoardSearch
	SearchResult search(const GameCards& gameCards, U32 depth, Score alpha = SCORE_MIN, const Score beta = SCORE_MAX) const;

};

struct SearchResult {
	Score score;
	Board board;
	U64 total;
};


#include "BoardIter.hpp"
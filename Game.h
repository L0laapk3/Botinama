#pragma once

#include "Card.h"
#include "CardBoard.h"
#include "Board.h"
// #include "MoveTable.h"
#include "TranspositionTable.h"

#include <thread>



struct SearchResult {
	Score score;
	Board board;
	U64 total;
};


class Game {
public:
	Game(std::array<std::string, 5> cardNames, std::string boardString, bool player, bool flipped = false);
	Game(const GameCards cards, Board board);
	const GameCards cards;
	Board board;
	TranspositionTable transpositionTable;
private:
    std::thread TBThread;
public:
    // MoveTable moveTable;

public:
	U64 perft (S32 maxDepth) const;	


private:
	template<bool quiescent>
	SearchResult search(const Board& board, S32 maxDepth, Score alpha, const Score beta);
public:
	SearchResult search(const Board& board, S32 maxDepth, const bool quiescent = false, Score alpha = SCORE_MIN, const Score beta = SCORE_MAX);
	SearchResult searchTime(const Board& board, U32 turn, const U64 timeBudget, const int verboseLevel = 1, const int expectedDepth = -1);
};

#pragma once

#include "Card.h"
#include "CardBoard.h"
#include "Board.h"
// #include "MoveTable.h"
#include "TranspositionTable.h"
#include "TableBase.h"

#include <thread>



struct SearchResult {
	Score score;
	Board board;
	U64 total;
};

class Connection;
class Game {
public:
	Game(std::array<std::string, 5> cardNames, std::string boardString = "1121100000000000000033433", bool player = false, bool flipped = false);
	Game(const GameCards& cards, const Board& board = Board::fromString("1121100000000000000033433", false, false));
	Game(Connection& connection);
private:
	void init();
public:

	U32 turn = 0;
	U8 lastDepth = 0;
	Score lastScore = 0;
	GameCards cards;
	Board board;

#ifdef USE_TB
    TableBase tableBase;
#endif
#ifdef USE_TT
	TranspositionTable transpositionTable;
#endif
    // MoveTable moveTable;

	U64 perft (S32 maxDepth) const;	


private:
	template<bool quiescent>
	SearchResult search(const Board& board, U8 maxDepth, Score alpha, const Score beta);
public:
	SearchResult search(const Board& board, U8 maxDepth, const bool quiescent = false, Score alpha = SCORE_MIN, const Score beta = SCORE_MAX);
	SearchResult searchTime(const Board& board, const U64 timeBudget, const float panicScale, const int verboseLevel = 1, const U8 expectedDepth = -1);
};

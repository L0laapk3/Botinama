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


private:
	template<bool quiescent>
	SearchResult search(const Board& board, U8 maxDepth, Score alpha, const Score beta);
public:
	SearchResult search(const Board& board, U8 maxDepth, const bool quiescent, Score alpha, const Score beta);
	SearchResult search(U8 depth, Score alpha = SCORE_MIN, Score beta = SCORE_MAX);
	void bench(U8 depth);
	SearchResult searchTime(const Board& board, const U64 timeBudget, const float panicScale, const int verboseLevel = 1, const U8 expectedDepth = -1);

	U64 perft(U8 depth) const;
private:
	U64 perft(const Board& board, U8 depth) const;
};

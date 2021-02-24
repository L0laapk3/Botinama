#pragma once

#include "Card.h"
#include "CardBoard.h"
#include "Board.h"
#include "MoveTable.h"
#include "TranspositionTable.h"
#include "TableBase.h"

#include <thread>
#include <string>



class Game;
struct IntermediateSearchResult {
	IntermediateSearchResult(const IntermediateSearchResult& other);
	IntermediateSearchResult(Score score, uint32_t halfBoard, U8 card, U64 total);
	Score score;
	U64 total;
};
struct OuterSearchResult : IntermediateSearchResult {
	OuterSearchResult(const OuterSearchResult& other);
	OuterSearchResult(Score score, uint32_t halfBoard, U8 card, U64 total);
	uint32_t halfBoard;
	uint8_t card;
};
struct SearchResult : OuterSearchResult {
	SearchResult();
	SearchResult(const Game& game, const OuterSearchResult& result);
	std::string cardName;
	unsigned long fromIndex;
	unsigned long toIndex;
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
    MoveTable moveTable;
#ifdef USE_TT
	TranspositionTable transpositionTable;
#endif

	U64 perft (S32 maxDepth) const;	


private:
	template<bool firstDepth, bool quiescent>
	typename std::conditional<firstDepth, OuterSearchResult, IntermediateSearchResult>::type search(const MoveTable::Move& currHalfBoard, const uint32_t currHalfAddress, const MoveTable::Move& otherHalfBoard, U8 cardsI, U8 maxDepth, Score alpha, const Score beta);
public:
	void bench(const U8 depth);
	template<bool firstDepth = true>
	typename std::conditional<firstDepth, OuterSearchResult, IntermediateSearchResult>::type search(const MoveTable::Move& currHalfBoard, const uint32_t currHalfAddress, const MoveTable::Move& otherHalfBoard, const U8 cardsI, U8 maxDepth, const bool quiescent, Score alpha, const Score beta);
	SearchResult search(U8 depth);
	SearchResult searchTime(const U64 timeBudget, const float panicScale = 5, const int verboseLevel = 1, const U8 expectedDepth = -1);
};

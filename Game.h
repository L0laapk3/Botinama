#pragma once

#include "Card.h"
#include "CardBoard.h"
#include "Board.h"
#include "MoveTable.h"

#include <thread>


struct SearchResult {
	Score score;
	uint32_t bitboard;
	uint32_t cardI;
	U64 total;
};


class Game {
public:
	Game(std::array<std::string, 5> cardNames, std::string boardString, bool player, bool flipped = false);
	Game(const GameCards cards, Board board);
	const GameCards cards;
	Board board;
private:
    std::thread TBThread;
public:
    MoveTable moveTable;

private:
	template<bool quiescent>
	SearchResult search(S32 maxDepth, HalfInfo* currHalfBoard, HalfInfo* otherHalfBoard, const CardsPos& cardsPos, Score alpha, const Score beta) const;
public:
	U64 perft (S32 maxDepth) const;
	SearchResult search(S32 maxDepth) const;
	SearchResult searchTime(const U64 timeBudget, const int verboseLevel = 1, const int expectedDepth = -1) const;
};

#pragma once

#include <string>
#include <array>
#include <list>
#include "CardBoard.h"
#include <functional>
#include <bitset>
#include <cassert>
#include <iostream>
#include "Bitscan.h"
#include "Botama.h"


void printKings(U64 pieces);

class Board;
typedef void (*MoveFunc)(GameCards& gameCards, const Board& board, const bool finished, U32 depth);

class Board {
public:
	U64 pieces;

	static Board fromString(std::string str, bool player);

	void print(GameCards& gameCards, bool finished = false) const;
	static void print(GameCards& gameCards, std::vector<Board> board, std::vector<bool> finished = { false });

	void valid() const;
	bool winner() const;

private:
	template<MoveFunc cb>
	void iterateMoves(GameCards& gameCards, const MoveBoard& moveBoards, U64 piecesWithNewCards, bool player, U32 depth) const;
public:

	U32 countForwardMoves(GameCards& gameCards) const;

	template<MoveFunc cb>
	void forwardMoves(GameCards& gameCards, U32 depth) const;

	template<MoveFunc cb>
	void reverseMoves(GameCards& gameCards, U32 depth) const;

};



#include "Board.hpp"
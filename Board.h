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
#include "Score.h"



class Board {
public:
	U64 pieces;

	static Board fromString(std::string str, bool player);

	void print(GameCards& gameCards, bool finished = false) const;
	static void print(GameCards& gameCards, std::vector<Board> board, std::vector<bool> finished = { false });

	void valid() const;
	bool winner() const;

	U32 countForwardMoves(GameCards& gameCards) const;


	Score eval(GameCards& gameCards, const bool finished) const;
private:
	Score findImmediateWins(GameCards& gameCards) const;


	//BoardIter
private:
	template <typename MoveFunc>
	void iterateMoves(GameCards& gameCards, const MoveBoard& moveBoards, U64 piecesWithNewCards, bool player, MoveFunc cb) const;
public:
	template <typename MoveFunc>
	void forwardMoves(GameCards& gameCards, MoveFunc cb) const;
	template <typename MoveFunc>
	void reverseMoves(GameCards& gameCards, MoveFunc cb) const;


	//BoardSearch
	Score search(GameCards& gameCards, U32 depth, const bool finished = false, Score alpha = SCORE_MIN, const Score beta = SCORE_MAX) const;

};


#include "BoardIter.hpp"

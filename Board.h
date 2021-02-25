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


class Game;


class Board {
public:
#pragma pack (push)
#pragma pack (1)
	uint64_t pieces;
	uint64_t kings;
	uint8_t cards;
	bool turn;
	union {
		uint8_t sortCriteria;
		struct {
			uint8_t isTake : 1;
			uint8_t isEnd : 1;
		};
	};
#pragma pack (pop)

	static Board fromString(std::string str, bool player, bool flip = false);

	void print(const GameCards& gameCards, bool finished = false, bool verbose = false) const;
	static void print(const GameCards& gameCards, std::vector<Board> board, std::vector<bool> finished = { false }, bool verbose = false);

	// void valid() const;
	// Board invert() const;

	// uint8_t countForwardMoves(const GameCards& gameCards) const;
	// bool winInOne(const GameCards& gameCards) const;
	// uint8_t winInTwo(const GameCards& gameCards) const;

	Score eval(const GameCards& gameCards) const;
// private:


// 	//BoardIter
// private:
// 	template<MoveFunc cb, bool reverse>
// 	void iterateMoves(Game& game, const MoveBoard& moveBoards, U64 piecesWithNewCards, bool player, const bool createPiece, const int8_t depthVal, const int threadNum) const;
// public:
// 	template<MoveFunc cb>
// 	void forwardMoves(Game& game, const int8_t depthVal, const int threadNum) const;
// 	template<MoveFunc cb>
// 	void reverseMoves(Game& game, const U32 maxMen, const U32 maxMenPerSide, const int8_t depthVal, const int threadNum) const;

};
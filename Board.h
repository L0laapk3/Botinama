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
class Board;
typedef void (*MoveFunc)(Game& game, const Board& board, const bool finished, const U32 passTrough);


class TableBase;
class Board {
public:
	U64 pieces;
	U64 kings;
	// bool operator <(const Board& rhs) const {
	// 	return pieces < rhs.pieces;
	// }
	// bool operator==(const Board& rhs) const {
	// 	return pieces == rhs.pieces;
	// }

	static Board fromString(std::string str, bool player, bool flip = false);

	void print(const GameCards& gameCards, bool finished = false, bool verbose = false) const;
	static void print(const GameCards& gameCards, std::vector<Board> board, std::vector<bool> finished = { false }, bool verbose = false);

	void valid() const;
	bool winner() const;
	bool currentPlayer() const;
	Board invert() const;

	uint8_t countForwardMoves(const GameCards& gameCards) const;
	bool winInOne(const GameCards& gameCards) const;
	uint8_t winInTwo(const GameCards& gameCards) const;

	Score eval(const GameCards& gameCards) const;
private:


	//BoardIter
private:
	template<MoveFunc cb, bool reverse>
	void iterateMoves(Game& game, const MoveBoard& moveBoards, U64 piecesWithNewCards, bool player, const bool createPiece, const U32 passTrough) const;
public:
	template<MoveFunc cb>
	void forwardMoves(Game& game, const U32 passTrough) const;
	template<MoveFunc cb>
	void reverseMoves(Game& game, const U32 maxMen, const U32 maxMenPerSide, const U32 passTrough) const;
#ifdef USE_TB
	bool testForwardTB(GameCards& cards, std::array<int8_t, TBSIZE>& tableBase) const;
#endif
public:
	template<bool invert>
	U32 compressToIndex() const;
	template<bool invert>
	static Board decompressIndex(U32 boardComp);
};

struct BoardHash {
	uint64_t xorshift(const uint64_t& n, int i) const {
		return n^(n>>i);
	}
	size_t operator()(const Board& b) const {
			uint64_t p = 0x5555555555555555ull; // pattern of alternating 0 and 1
			uint64_t c = 17316035218449499591ull;// random uneven integer constant; 
			return c*xorshift(p*xorshift(b.pieces,32),32);
	}
	// size_t operator()(const Board& b) const {
	// 	return std::hash<U64>()(b.pieces);
	// }
};


#include "BoardIter.hpp"
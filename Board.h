#pragma once

#include <string>
#include <array>
#include <list>
#include "CardBoard.h"
#include <span>


struct Moves;

class Board {
public:
	std::array<uint32_t, 2> pieces;
	uint32_t kings;

	static Board fromString(std::string str);

	void print(GameCards& gameCards) const;
	static void print(GameCards& gameCards, std::vector<Board> board);

	void valid(GameCards& gameCards) const;
	bool finished() const;
	bool winner() const;
	Moves forwardMoves(GameCards& gameCards) const;
	Moves reverseMoves(GameCards& gameCards) const;
private:
	void iterateMoves(Moves& out, const CardBoard& card, uint32_t newCards, bool player, bool movingPlayer) const;
};


struct Moves {
	unsigned long size = 0;
	std::array<Board, 8 * 5> outputs;
};
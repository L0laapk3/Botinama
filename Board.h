#pragma once

#include <string>
#include <array>
#include <list>
#include <functional>
#include "CardBoard.h"
#include <span>


class Board;
typedef std::function<void(Board board)> MoveFunc;


class Board {
public:
	Board(const Board& board);
	Board(std::string str);
	Board(std::array<uint32_t, 2> pieces, uint32_t kings, bool player, uint32_t cards);
	std::array<uint32_t, 2> pieces{ 0 };
	uint32_t kings = 0;
	bool player = false;
	uint32_t cards = 0b10000'000'01100'000'00011;

	void print(GameCards& gameCards) const;
	static void print(GameCards& gameCards, std::vector<Board> board);

	void valid(GameCards& gameCards) const;
	bool finished() const;
	bool winner() const;
	void forwardMoves(GameCards& gameCards, MoveFunc cb) const;
	void reverseMoves(GameCards& gameCards, MoveFunc cb) const;
private:
	void iterateMoves(const CardBoard& card, uint32_t newCards, bool movingPlayer, MoveFunc& cb) const;
};
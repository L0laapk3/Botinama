#pragma once

#include <string>
#include <array>
#include <list>
#include <functional>
#include "CardBoard.h"


class Board;
typedef std::function<void(const Board& board)> MoveFunc;


class Board {
public:
	Board(std::string str);
	Board(std::array<uint32_t, 2> pieces, uint32_t kings, bool player, uint32_t cards);
	std::array<uint32_t, 2> pieces;
	uint32_t kings;
	bool player;
	uint32_t cards;

	void forwardMoves(std::array<CardBoard, 5>& gameCards, MoveFunc& cb);
	void reverseMoves(std::array<CardBoard, 5>& gameCards, MoveFunc& cb);
private:
	void iterateMoves(const CardBoard& card, uint32_t newCards, bool movingPlayer, MoveFunc& cb);
};
#pragma once

#include "easywsclient.hpp"
#include <memory>
#include <array>

#include "Board.h"
#include "CardBoard.h"

struct Game {
	const GameCards cards;
	Board board;
};

class Connection {
public:
	Connection();
	~Connection();

	Game waitGame();
	void waitTurn(Game& game);
	void submitMove(Game& game, const Board& board);

	std::string matchId;
	std::string token;
	std::string opponent;

	int index;
	bool player;
	bool currentTurn;
	bool ended = false;

private:
	std::unique_ptr<easywsclient::WebSocket> ws = nullptr;
};
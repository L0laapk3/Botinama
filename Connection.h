#pragma once

#include "easywsclient.hpp"
#include <memory>
#include <array>

#include "Game.h"
#include "Board.h"
#include "CardBoard.h"

// struct Game {
// 	Game(const GameCards cards, Board board);
// 	const GameCards cards;
// 	std::unique_ptr<MoveTable::MoveTable> moveTable;
// 	Board board;
// };

struct loadReturn {

};

class Connection {
public:
	Connection();
	Connection(std::string matchId);
	~Connection();

	void sendCreate();
	void sendJoin(const std::string& matchId);
	GameCards load();
	Board loadedBoard;
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
	void handleJoinGame();
	std::unique_ptr<easywsclient::WebSocket> ws = nullptr;
};
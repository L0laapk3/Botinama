#pragma once

#include "easywsclient.hpp"
#include <memory>
#include <array>

class Connection {
public:
	Connection();
	~Connection();

	void getBoard();

	std::string matchId;
	std::string token;
	std::string opponent;
	std::array<std::string, 5> cards;
	std::string board;

	int index;
	bool player;

private:
	easywsclient::WebSocket::pointer ws;
};
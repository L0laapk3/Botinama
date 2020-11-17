#ifdef _WIN32
#pragma comment( lib, "ws2_32" )
#include <WinSock2.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string>
#include <memory>
#include <regex>
#include <iostream>

#include "Connection.h"

#include <windows.h>
#include <shellapi.h>

std::string getRegex(const std::string& message, const std::string reg) {
	std::smatch match;
	if (std::regex_search(message, match, std::regex(reg)) && match.size() > 1)
		return match.str(1);
	return "";
}
std::string getString(const std::string& message, const std::string key) {
	return getRegex(message, "\"" + key + "\":\"([^\"]+)\"");
}
int getInt(const std::string& message, const std::string key) {
	std::string result = getRegex(message, "\"" + key + "\":(\\d+)");
	return result.size() ? std::stoi(result) : -1;
}


Connection::Connection() {
#ifdef _WIN32
	INT rc;
	WSADATA wsaData;

	rc = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (rc) {
		printf("WSAStartup Failed.\n");
	}
#endif
	ws = easywsclient::WebSocket::from_url("ws://litama.herokuapp.com");
	assert(ws);
	assert(ws->getReadyState() != easywsclient::WebSocket::CLOSED);

	ws->send("create Botama");

	assert(ws->getReadyState() != easywsclient::WebSocket::CLOSED);
	while (!matchId.size()) {
		ws->poll(-1);
		assert(ws->getReadyState() != easywsclient::WebSocket::CLOSED);
		ws->dispatch([&](const std::string& message) {
			matchId = getString(message, "matchId");
			token = getString(message, "token");
			index = getInt(message, "index");
		});
	}
	assert(token.size());
	assert(index != -1);

	ws->send("spectate " + matchId);

	std::string webUrl = "https://git.io/onitama#spectate-" + matchId;
	ShellExecute(0, 0, std::wstring(webUrl.begin(), webUrl.end()).c_str(), 0, 0, SW_SHOW);
	std::cout << webUrl << std::endl;

	board = "";
	while (!board.size()) {
		ws->poll(-1);
		assert(ws->getReadyState() != easywsclient::WebSocket::CLOSED);
		ws->dispatch([&](const std::string& message) {
			board = getString(message, "board");
			if (board.size()) {
				player = getInt(message, "red") == index;
				cards[0] = getRegex(message, "\"cards\":[^}]+\"blue\":\\[\"([^\"]+)\"");
				cards[1] = getRegex(message, "\"cards\":[^}]+\"blue\":\\[\"[^\"]+\",\"([^\"]+)\"");
				cards[2] = getRegex(message, "\"cards\":[^}]+\"red\":\\[\"([^\"]+)\"");
				cards[3] = getRegex(message, "\"cards\":[^}]+\"red\":\\[\"[^\"]+\",\"([^\"]+)\"");
				cards[4] = getRegex(message, "\"cards\":[^}]+\"side\":\"([^\"]+)\"");
			}
		});
	}
}

Connection::~Connection() {
#ifdef _WIN32
	WSACleanup();
#endif
	delete ws;
}

void Connection::getBoard() {
	board = "";
	while (!board.size()) {
		ws->poll(-1);
		assert(ws->getReadyState() != easywsclient::WebSocket::CLOSED);
		ws->dispatch([&](const std::string& message) {
			board = getString(message, "board");
		});
	}
}
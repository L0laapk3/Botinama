#pragma once

#include <array>
#include <string>
#include "Card.h"

class CardBoard;
typedef std::array<const CardBoard, 5> GameCards;

class CardBoard : public Card {
public:
	CardBoard(const CardBoard& card);
	CardBoard(const Card* card);
	std::array<uint64_t, 25> moveBoard;

	static GameCards fetchGameCards(std::array<std::string, 5> cardNames);
};
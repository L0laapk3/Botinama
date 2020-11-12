#pragma once

#include <array>
#include <string>
#include "Card.h"

class CardBoard;
typedef std::array<const CardBoard, 5> GameCards;

class CardBoard : public Card {
public:
	CardBoard(CardBoard& card);
	CardBoard(const Card* card);
	std::array<std::array<uint32_t, 25>, 2> moveBoard;

	static GameCards fetchGameCards(std::array<std::string, 5> cardNames);
};
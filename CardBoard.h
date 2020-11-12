#pragma once


#include "Card.h"

class CardBoard : Card {
public:
	CardBoard(CardBoard& card);
	CardBoard(Card& card);
	std::array<std::array<uint32_t, 25>, 2> moveBoard;
};
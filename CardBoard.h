#pragma once


#include "Card.h"

class CardBoard : public Card {
public:
	CardBoard(CardBoard& card);
	CardBoard(const Card& card);
	std::array<std::array<uint32_t, 25>, 2> moveBoard;
};
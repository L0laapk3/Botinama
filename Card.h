#pragma once

#include <string>
#include <array>

typedef unsigned long long bb;
class Card {
public:
	std::string name;
	bb moves;
	Card(std::string name, bb moves);
};

extern std::array<Card, 16> Cards;
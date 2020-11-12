#pragma once

#include <string>
#include <array>
#include <vector>

typedef unsigned long long bb;
class Card {
public:
	const std::string name;
	const bb moves;
	Card(std::string name, bb moves);
	void print() const;
	static void print(std::vector<bb> cards);
};

extern std::array<const Card, 16> CARDS;
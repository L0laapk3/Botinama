#pragma once

#include <string>
#include <array>
#include <vector>

typedef unsigned long long bb;
class Card;
class Card {
public:
	const std::string name;
	const bb moves;

	Card(std::string name, bb moves);
	void print() const;
	static void print(std::vector<bb> cards);
	static void print(std::vector<Card> cards);

	static const Card* findCard(const std::string& name);
};

extern std::array<const Card, 16> CARDS;
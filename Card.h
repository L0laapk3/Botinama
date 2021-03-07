#pragma once

#include <string>
#include <array>
#include <vector>
#include "Botama.h"

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

struct CardsPos {
	std::array<U32, 2> players;
	U8 side;
	static CardsPos construct(U32 blue0, U32 blue1, U32 red0, U32 red1, U8 side, U32 swapBlue0, U32 swapBlue1, U32 swapRed0, U32 swapRed1) {
		return CardsPos{
			{
				(blue0 - 1) | (swapBlue0 << 8) | ((blue1 - 1) << 16) | (swapBlue1 << 24),
				(red0 - 1) | (swapRed0 << 8) | ((red1 - 1) << 16) | (swapRed1 << 24)
			},
			(U8)(side - 1)
		};
	};
};
const std::array<CardsPos, 30> CARDS_LUT = { {
	CardsPos::construct(1, 2, 3, 4, 5,	26,	20,	12,	6),
	CardsPos::construct(1, 3, 2, 4, 5,	28,	14,	18,	7),
	CardsPos::construct(1, 4, 2, 3, 5,	29,	8,	19,	13),
	CardsPos::construct(2, 3, 1, 4, 5,	22,	16,	24,	9),
	CardsPos::construct(2, 4, 1, 3, 5,	23,	10,	25,	15),
	CardsPos::construct(3, 4, 1, 2, 5,	17,	11,	27,	21),
	CardsPos::construct(1, 2, 3, 5, 4,	25,	19,	12,	0),
	CardsPos::construct(1, 3, 2, 5, 4,	27,	13,	18,	1),
	CardsPos::construct(1, 5, 2, 3, 4,	29,	2,	20,	14),
	CardsPos::construct(2, 3, 1, 5, 4,	21,	15,	24,	3),
	CardsPos::construct(2, 5, 1, 3, 4,	23,	4,	26,	16),
	CardsPos::construct(3, 5, 1, 2, 4,	17,	5,	28,	22),
	CardsPos::construct(1, 2, 4, 5, 3,	24,	18,	6,	0),
	CardsPos::construct(1, 4, 2, 5, 3,	27,	7,	19,	2),
	CardsPos::construct(1, 5, 2, 4, 3,	28,	1,	20,	8),
	CardsPos::construct(2, 4, 1, 5, 3,	21,	9,	25,	4),
	CardsPos::construct(2, 5, 1, 4, 3,	22,	3,	26,	10),
	CardsPos::construct(4, 5, 1, 2, 3,	11,	5,	29,	23),
	CardsPos::construct(1, 3, 4, 5, 2,	24,	12,	7,	1),
	CardsPos::construct(1, 4, 3, 5, 2,	25,	6,	13,	2),
	CardsPos::construct(1, 5, 3, 4, 2,	26,	0,	14,	8),
	CardsPos::construct(3, 4, 1, 5, 2,	15,	9,	27,	5),
	CardsPos::construct(3, 5, 1, 4, 2,	16,	3,	28,	11),
	CardsPos::construct(4, 5, 1, 3, 2,	10,	4,	29,	17),
	CardsPos::construct(2, 3, 4, 5, 1,	18,	12,	9,	3),
	CardsPos::construct(2, 4, 3, 5, 1,	19,	6,	15,	4),
	CardsPos::construct(2, 5, 3, 4, 1,	20,	0,	16,	10),
	CardsPos::construct(3, 4, 2, 5, 1,	13,	7,	21,	5),
	CardsPos::construct(3, 5, 2, 4, 1,	14,	1,	22,	11),
	CardsPos::construct(4, 5, 2, 3, 1,	8,	2,	23,	17),
} };
const std::array<U32, 30> CARDS_INVERT = { 5, 4, 3, 2, 1, 0, 11, 10, 9, 8, 7, 6, 17, 16, 15, 14, 13, 12, 23, 22, 21, 20, 19, 18, 29, 28, 27, 26, 25, 24 };

const std::array<std::array<uint8_t, 12>, 5> CARDS_USERS = {{
	{0, 1, 2, 6, 7, 8, 12, 13, 14, 18, 19, 20 },
	{0, 3, 4, 6, 9, 10, 12, 15, 16, 24, 25, 26 },
	{1, 3, 5, 7, 9, 11, 18, 21, 22, 24, 27, 28 },
	{2, 4, 5, 13, 15, 17, 19, 21, 23, 25, 27, 29 },
	{8, 10, 11, 14, 16, 17, 20, 22, 23, 26, 28, 29 },
}};
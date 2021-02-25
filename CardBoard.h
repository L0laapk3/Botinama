#pragma once

#include <array>
#include <string>
#include "Card.h"
#include "Botama.h"

class CardBoard;
typedef std::array<CardBoard, 5> GameCards;
typedef std::array<uint32_t, 26> MoveBoard;

class CardBoard : public Card {
public:
	CardBoard(const CardBoard& card);
	CardBoard(const Card* card);
	std::array<MoveBoard, 2> moveBoards;

	static GameCards fetchGameCards(std::array<std::string, 5> cardNames, bool flipped = false);
	static U32 getCardIndex(const GameCards& gameCards, std::array<std::string, 5> cardNames, bool flipped = false);
};
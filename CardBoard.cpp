
#include "CardBoard.h"

#include <iostream>
#include <bitset>

CardBoard::CardBoard(const CardBoard& card) : Card(card), moveBoard(card.moveBoard) { };
CardBoard::CardBoard(const Card* card) : Card(card->name, card->moves), moveBoard{ 0 } {
    constexpr std::array<unsigned long long, 5> shiftMasks{
        0b11100'11100'11100'11100'11100,
        0b11110'11110'11110'11110'11110,
        0b11111'11111'11111'11111'11111,
        0b01111'01111'01111'01111'01111,
        0b00111'00111'00111'00111'00111,
    };
    for (uint64_t i = 0; i < 25; i++) {
        uint64_t maskedMove = card->moves & shiftMasks[i % 5];
        moveBoard[i] |= i > 12 ? maskedMove << (i - 12) : maskedMove >> (12 - i);
        moveBoard[24ULL - i] |= (moveBoard[i] & (1 << 12)) << 32;
        for (int j = 0; j < 12; j++) {
            if (moveBoard[i] & (1ULL << j))
                moveBoard[24ULL - i] |= 1ULL << (32 + 24 - j);
            if (moveBoard[i] & (1ULL << (24 - j)))
                moveBoard[24ULL - i] |= 1ULL << (32 + j);
        }
    }
}

GameCards CardBoard::fetchGameCards(std::array<std::string, 5> cardNames) {
    return GameCards{
        CardBoard(Card::findCard(cardNames[0])),
        CardBoard(Card::findCard(cardNames[1])),
        CardBoard(Card::findCard(cardNames[2])),
        CardBoard(Card::findCard(cardNames[3])),
        CardBoard(Card::findCard(cardNames[4]))
    };
}

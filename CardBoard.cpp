
#include "CardBoard.h"

#include <iostream>
#include <bitset>

CardBoard::CardBoard(CardBoard& card) : Card(card), moveBoard(card.moveBoard) { };
CardBoard::CardBoard(Card& card) : Card(card.name, card.moves) {
    const std::array<uint32_t, 5> shiftMasks{
        0b11100'11100'11100'11100'11100,
        0b11110'11110'11110'11110'11110,
        0b11111'11111'11111'11111'11111,
        0b01111'01111'01111'01111'01111,
        0b00111'00111'00111'00111'00111,
    };
    for (int i = 0; i < 25; i++) {
        uint32_t maskedMove = card.moves & shiftMasks[i % 5];
        moveBoard[0][i] = i > 12 ? maskedMove << (i - 12) : maskedMove >> (12 - i);
        moveBoard[1][24 - i] = moveBoard[0][i];
    }
} 
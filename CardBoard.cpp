
#include "CardBoard.h"

#include <iostream>
#include <bitset>

CardBoard::CardBoard(CardBoard& card) : Card(card), moveBoard(card.moveBoard) { };
CardBoard::CardBoard(const Card& card) : Card(card.name, card.moves) {
    constexpr std::array<uint32_t, 5> shiftMasks{
        0b11100'11100'11100'11100'11100,
        0b11110'11110'11110'11110'11110,
        0b11111'11111'11111'11111'11111,
        0b01111'01111'01111'01111'01111,
        0b00111'00111'00111'00111'00111,
    };
    for (int i = 0; i < 25; i++) {
        uint32_t maskedMove = card.moves & shiftMasks[i % 5];
        moveBoard[0][i] = i > 12 ? maskedMove << (i - 12) : maskedMove >> (12 - i);
        moveBoard[1][24 - i] = 0;
        for (int j = 0; j < 12; j++) {
            if (moveBoard[0][i] & (1 << j))
                moveBoard[1][24 - i] |= (1 << (24 - j));
            if (moveBoard[0][i] & (1 << (24 - j)))
                moveBoard[1][24 - i] |= (1 << j);
        }
        //Card::print({ moveBoard[0][i], moveBoard[1][24 - i] });
    }
} 
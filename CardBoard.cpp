
#include "CardBoard.h"

#include <iostream>
#include <bitset>
#include <cassert>

CardBoard::CardBoard(const CardBoard& card) : Card(card), moveBoards(card.moveBoards) { };
CardBoard::CardBoard(const Card* card) : Card(card->name, card->moves), moveBoards{ 0 } {
    constexpr std::array<U32, 5> shiftMasks{
        0b11100'11100'11100'11100'11100,
        0b11110'11110'11110'11110'11110,
        0b11111'11111'11111'11111'11111,
        0b01111'01111'01111'01111'01111,
        0b00111'00111'00111'00111'00111,
    };
    for (U64 i = 0; i < 25; i++) {
        U32 maskedMove = card->moves & shiftMasks[i % 5];
        moveBoards[0][i] = (i > 12 ? maskedMove << (i - 12) : maskedMove >> (12 - i)) & MASK_PIECES;
        moveBoards[1][24ULL - i] = moveBoards[0][i] & (1 << 12);
        for (int j = 0; j < 12; j++) {
            if (moveBoards[0][i] & (1 << j))
                moveBoards[1][24ULL - i] |= (1 << (24 - j));
            if (moveBoards[0][i] & (1 << (24 - j)))
                moveBoards[1][24ULL - i] |= (1 << j);
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

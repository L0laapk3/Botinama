
#include "CardBoard.h"
#include "Card.h"

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
    //std::vector<unsigned long long> one{};
    //std::vector<unsigned long long> two{};
    //for (U64 i = 0; i < 25; i++) {
    //    one.push_back(moveBoards[0][i]);
    //    two.push_back(moveBoards[1][i]);
    //}
    //Card::print(one);
    //std::cout << std::endl;
    //Card::print(two);
    //std::cout << std::endl << std::endl << std::endl << std::endl;
}

GameCards CardBoard::fetchGameCards(std::array<std::string, 5> cardNames, bool flipped) {
    return GameCards{
        CardBoard(Card::findCard(cardNames[flipped ? 2 : 0])),
        CardBoard(Card::findCard(cardNames[flipped ? 3 : 1])),
        CardBoard(Card::findCard(cardNames[flipped ? 0 : 2])),
        CardBoard(Card::findCard(cardNames[flipped ? 1 : 3])),
        CardBoard(Card::findCard(cardNames[4]))
    };
}

U32 CardBoard::getCardIndex(const GameCards& gameCards, std::array<std::string, 5> cardNames, bool flipped) {
    for (U32 i = 0; i < 30; i++) {
        const auto& cPos = CARDS_LUT[i];
        std::array<U32, 5> cardOrder = {
            cPos.players[flipped] & 0xff,
            (cPos.players[flipped] >> 16) & 0xff,
            cPos.players[!flipped] & 0xff,
            (cPos.players[!flipped] >> 16) & 0xff,
            cPos.side
        };
        if (gameCards[cardOrder[4]].name != cardNames[4]) // side card
            continue;
        for (U32 j = 0; j < 5; j++) {
            if (j == 4)
                return i;
            if ((gameCards[cardOrder[j]].name != cardNames[j]) && (gameCards[cardOrder[j]].name != cardNames[j ^ 1]))
                break;
        }
    }
    assert(0);
    return -1;
}

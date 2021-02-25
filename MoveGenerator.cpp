
#include "MoveGenerator.h"

#include <algorithm>

#include "Game.h"


MoveGenerator::MoveGenerator(const Game& game, const Board& board, const bool quiescent) {
    it = boards.end() - 1;
	U32 cardStuff = CARDS_LUT[board.cards].players[board.turn];
    for (int i = 0; i < 2; i++) {
        unsigned long cardI = cardStuff & 0xff;
        uint8_t nextCards = (cardStuff >> 8) & 0xff;
		cardStuff >>= 16;
	    const auto& moveBoard = game.cards[cardI].moveBoards[board.turn];
        uint32_t bitScan = board.pieces[board.turn];
        unsigned long fromI;
        while (_BitScanForward(&fromI, bitScan)) {
            uint32_t fromBit = bitScan & -bitScan;
			U32 scan = moveBoard[fromI] & ~board.pieces[board.turn];
            bitScan -= fromBit;
            bool isKingMove = fromBit & board.kings[board.turn];
            if (quiescent)
                scan &= board.pieces[!board.turn] | (isKingMove ? MASK_END_POSITIONS[board.turn] : 0);
            uint32_t piecesWithoutMoved = board.pieces[board.turn] - fromBit;
            const uint32_t endMask = board.kings[!board.turn] | (isKingMove ? MASK_END_POSITIONS[board.turn] : 0);
            while (scan) {
                const U32 landBit = scan & -scan;
                scan -= landBit;
                new (&*it--) Board{
                    {
                        board.turn ? board.pieces[!board.turn] & ~landBit : piecesWithoutMoved | landBit,
                        !board.turn ? board.pieces[!board.turn] & ~landBit : piecesWithoutMoved | landBit,
                    }, {
                        board.turn ? board.kings[!board.turn] : (isKingMove ? landBit : board.kings[board.turn]),
                        !board.turn ? board.kings[!board.turn] : (isKingMove ? landBit : board.kings[board.turn]),
                    },
                    nextCards,
                    !board.turn,
                    (bool)(board.pieces[!board.turn] & landBit),
                    (bool)(endMask & landBit),
                };
                // (end-1)->print(game.cards);
                // std::cout << (U32)nextCards << ' '<< std::endl;
            }
        }
    }
}

bool MoveGenerator::next() {
    if (++it == boards.end())
        return false;
    std::partial_sort(it, it+1, boards.end(), [](const auto& a, const auto& b) {
        return a.cards > b.cards;
    });
    return true;
}
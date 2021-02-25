
#include "MoveGenerator.h"

#include <algorithm>

#include "Game.h"


MoveGenerator::MoveGenerator(const Game& game, Board& board, const bool quiescent) {
    end = boards.begin();
	U32 cardStuff = CARDS_LUT[board.cards].players[board.turn];
    for (U8 ignoreTakes = 0; ignoreTakes < (quiescent ? 1 : 2); ignoreTakes++) {
        for (U8 i = 0; i < 2; i++) {
            unsigned long cardI = cardStuff & 0xff;
            uint8_t nextCards = (cardStuff >> 8) & 0xff;
            cardStuff >>= 16;
            const auto& moveBoard = game.cards[cardI].moveBoards[board.turn];
            uint64_t bitScan = board.pieces;
            unsigned long fromI;
            while (_BitScanForward(&fromI, bitScan)) {
                uint64_t fromBit = bitScan & -bitScan;
                bitScan -= fromBit;
                uint64_t scan = (moveBoard[fromI] & ~board.pieces) << 32;
                bool isKingMove = fromBit & board.kings;
                uint64_t filter = (board.pieces | (isKingMove ? ((uint64_t)MASK_END_POSITIONS[board.turn]) << 32 : 0));
                scan &= (ignoreTakes ? ~filter : filter);
                uint64_t piecesWithoutMoved = swap32(board.pieces & ~(uint64_t)fromBit);
                uint64_t kingsWithoutMoved = swap32(board.kings & ~(uint64_t)fromBit);
                uint64_t endMask = board.kings | (isKingMove ? ((uint64_t)MASK_END_POSITIONS[board.turn] << 32) : 0);
                while (scan) {
                    const uint64_t landBit = scan & -scan;
                    scan -= landBit;
                    end->pieces = piecesWithoutMoved | landBit & ~(landBit >> 32);
                    end->kings = kingsWithoutMoved | (isKingMove ? landBit : 0);
                    end->cards = nextCards;
                    end->turn = !board.turn;
                    end->isEnd = endMask & landBit;
                    // it->print(game.cards);
                    // std::cout << (U32)nextCards << ' '<< std::endl;
                    end++;
                }
            }
        }
    }
}

// bool MoveGenerator::next() {
//     if (++it == end)
//         return false;
//     std::partial_sort(it, it+1, end, [](const auto& a, const auto& b) {
//         return a.isEnd > b.sortCriteria;
//     });
//     return true;
// }
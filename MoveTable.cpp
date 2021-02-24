#include "MoveTable.h"

#include <bitset>
#include <algorithm>


U32 bitReverse(U32 source) {
	U32 result;
    for (int i = 0; i < 25; i++) {
        result <<= 1;
        result |= source & 1;
        source >>= 1;
    }
	return result;
}

MoveTable::Move::Move() : addressCard(0), bitboard(0), bitboard_flipped(0), halfScore(0), afterTakeBoards{} {};
MoveTable::Move::Move(const MoveTable& moveTable, uint32_t bitboard, uint32_t kingbit, uint8_t card) :
    address(moveTable.addressFromBitboards(bitboard, kingbit)),
    card(card),
    bitboard(bitboard),
    bitboard_flipped(bitReverse(bitboard)),
    halfScore(ScoreHalf(bitboard, kingbit)),
    afterTakeBoards{} {
        auto takeIt = afterTakeBoards.begin();
        U32 bitScan = bitboard;
        unsigned long fromI;
        while (_BitScanForward(&fromI, bitScan)) {
            const U32 fromBit = (1ULL << fromI);
            bitScan -= fromBit;
            if (fromBit != kingbit) {
                uint32_t newBoard = bitboard - fromBit;
                *takeIt = moveTable.addressFromBitboards(bitboard, kingbit);
            }
            takeIt++;
        }
    }


uint32_t MoveTable::ttAddressFromBitBoards(uint32_t bitboard, uint32_t kingBit) const {
    return bitboard * 5 + _popcnt32(bitboard & (kingBit - 1));
}

uint32_t MoveTable::addressFromBitboards(uint32_t bitboard, uint32_t kingBit) const {
    if (ttAddressFromBitBoards(bitboard, kingBit) >= (5ULL << 25))
        std::cout << std::bitset<32>(bitboard) << ' ' << std::bitset<32>(kingBit) << std::endl;
    return (*translationTable)[ttAddressFromBitBoards(bitboard, kingBit)] + 1;
}

void MoveTable::createEntry(const GameCards& gameCards, TranslationTable::iterator& tabIt, uint32_t bitboard, U32 kingBit) {
    (*translationTable)[ttAddressFromBitBoards(bitboard, kingBit)] = tabIt++ - translationTable->begin();
}




std::array<MoveTable::Move, 2> MoveTable::toHalfBoards(const Board& board) const {
    uint32_t player1Pieces = board.pieces & MASK_PIECES;
    uint32_t player2Pieces = (board.pieces >> 32) & MASK_PIECES;
    return {
        Move(*this, player1Pieces & MASK_PIECES, _pdep_u32(1ULL << ((board.pieces >> INDEX_KINGS[0]) & 7), player1Pieces) & MASK_PIECES, 0),
        Move(*this, bitReverse(player2Pieces) & MASK_PIECES, bitReverse(_pdep_u32(1ULL << ((board.pieces >> INDEX_KINGS[1]) & 7), player2Pieces)) & MASK_PIECES, 0),
    };
}


void MoveTable::createEntry(const GameCards& gameCards, Table::iterator& tabIt, uint32_t bitboard, U32 kingBit) {
    auto cardIt = tabIt->moves.begin();

    for (U32 cardI1 = 0; cardI1 < 5; cardI1++) 
        for (U32 cardI2 = cardI1 + 1; cardI2 < 5; cardI2++) {
            struct boardKingCard {
                uint32_t bitBoard;
                uint32_t kingBit;
                uint8_t cardI;
            };
            auto moveIt = cardIt->begin();

            unsigned long fromI;
            U32 bitScan = bitboard;
            while (_BitScanForward(&fromI, bitScan)) {
                const U32 fromBit = (1ULL << fromI);
                bitScan -= fromBit;
                U32 otherPieces = bitboard - fromBit;
                U32 scan = (gameCards[cardI1].moveBoards[0][fromI] | gameCards[cardI2].moveBoards[0][fromI]) & ~otherPieces;
                while (scan) {
                    const U32 landBit = scan & -scan;
                    scan -= landBit;
                    uint32_t newBoard = otherPieces | landBit;
                    uint32_t newKingBit = fromBit == kingBit ? landBit : kingBit;
                    for (uint8_t card = 0; card < 2; card++)
                        if (landBit & gameCards[!card ? cardI1 : cardI2].moveBoards[0][fromI])
                            new (&*moveIt++) Move(*this, newBoard, newKingBit, card);
                }
            }

            std::sort(cardIt->begin(), moveIt, [](auto& a, auto& b) {
                return a.halfScore > b.halfScore;
            });
            cardIt++;
        }

    tabIt++;
}

template <typename T, size_t size>
void MoveTable::placePiece(const GameCards& gameCards, typename std::array<T, size>::iterator& tabIt, uint32_t bitboard, U32 remainingPieces, U32 stopAt, U32 kingBit) {
    if (remainingPieces == 0) {
        createEntry(gameCards, tabIt, bitboard, kingBit);
    } else {
        uint32_t currentPiece = 1ULL;
        for (U32 i = 0; i + 1 < stopAt; i++) {
            currentPiece <<= currentPiece == kingBit;
            placePiece<T, size>(gameCards, tabIt, bitboard | currentPiece, remainingPieces - 1, i + 1, kingBit);
            currentPiece <<= 1;
        }
    }
}

template <typename T, size_t size>
void MoveTable::generateCombinations(const GameCards& gameCards, typename std::array<T, size>::iterator& tabIt) {
    for (U32 numPawns = 4; numPawns <= 4; numPawns--) {
        uint32_t kingBit = 1ULL;
        for (U32 kingI = 0; kingI < 25; kingI++) { // place king
            placePiece<T, size>(gameCards, tabIt, kingBit, numPawns, 25, kingBit);
            kingBit <<= 1;
        }
    }
}


void MoveTable::build(const GameCards& gameCards) {
    
    std::cout << "building movetables" << std::endl;

    translationTable = std::make_unique<TranslationTable>();
    auto otherIt = translationTable->begin();
    generateCombinations<uint32_t, 5ULL << 25>(gameCards, otherIt);

    table = std::make_unique<Table>();
    auto tabIt = table->begin() + 1;
    generateCombinations<MoveList, TOTAL_HALF_POSITION_COUNT>(gameCards, tabIt);

    //translationTable.reset();

	atomic_thread_fence(std::memory_order_acq_rel);

    std::cout << "created " << (tabIt - table->begin()) << " entries." << std::endl;
}
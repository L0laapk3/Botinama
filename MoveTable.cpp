#include "MoveTable.h"

#include <bitset>


typedef std::array<U32, 5 << 25> TranslationTable;
void createEntry(const GameCards& gameCards, TranslationTable& tt, TranslationTable::iterator& tabIt, uint32_t bitboard, U32 kingBit) {
    tt[bitboard * 5 + _popcnt32(bitboard & (kingBit - 1))] = tabIt++ - tt.begin();
}

void createEntry(const GameCards& gameCards, TranslationTable& tt, MoveTable::MoveTable::iterator& tabIt, uint32_t bitboard, U32 kingBit) {
    for (U32 cardsI = 0; cardsI < CARDS_LUT.size(); cardsI++) {
        const CardsPos& cardsPos = CARDS_LUT[cardsI];

        tabIt->bitboard = bitboard;
        tabIt->kingbit = kingBit;
        tabIt->score = ScoreHalf(bitboard, kingBit);

        auto moveIt = tabIt->moves.begin();
        
	    U32 cardStuff = cardsPos.players[0];
        const auto& card1 = gameCards[cardStuff & 0xff];
        const auto& card2 = gameCards[(cardStuff >> 16) & 0xff];
        const auto& moveBoard1 = card1.moveBoards[0];
        const auto& moveBoard2 = card2.moveBoards[0];
        unsigned long fromI;
        U32 bitScan = bitboard;
        while (_BitScanForward(&fromI, bitScan)) {
            const U32 fromBit = (1ULL << fromI);
		    bitScan -= fromBit;
            U32 otherPieces = bitboard - fromBit;
			U32 scan = (moveBoard1[fromI] | moveBoard2[fromI]) & ~otherPieces;
			while (scan) {
				const U32 landBit = scan & -scan;
				scan -= landBit;
                uint32_t newBoard = otherPieces | landBit;
                uint32_t newKingBit = fromBit == kingBit ? landBit : kingBit;
                if (landBit & moveBoard1[fromI])
                    *moveIt++ = tt[newBoard * 5 + _popcnt32(newBoard & (newKingBit - 1))] * 30 + ((cardStuff >> 8) & 0xff);
                if (landBit & moveBoard2[fromI])
                    *moveIt++ = tt[newBoard * 5 + _popcnt32(newBoard & (newKingBit - 1))] * 30 + ((cardStuff >> 24) & 0xff);
            }
        }
        // TODO: sort moves

        auto takeIt = tabIt->afterTakeBoards.begin();
        bitScan = bitboard;
        while (_BitScanForward(&fromI, bitScan)) {
            const U32 fromBit = (1ULL << fromI);
		    bitScan -= fromBit;
            uint32_t newBoard = bitboard - fromBit;
            *takeIt++ = tt[newBoard * 5 + _popcnt32(newBoard & (kingBit - 1))] * 30 + cardsI;
        }

        tabIt++;
    }
}

template <typename T, size_t size>
void placePiece(const GameCards& gameCards, TranslationTable& tt, std::array<T, size>::iterator& tabIt, uint32_t bitboard, U32 remainingPieces, U32 stopAt, U32 kingBit) {
    if (remainingPieces == 0) {
        createEntry(gameCards, tt, tabIt, bitboard, kingBit);
    } else {
        uint32_t currentPiece = 1ULL;
        for (U32 i = 0; i + 1 < stopAt; i++) {
            currentPiece <<= currentPiece == kingBit;
            placePiece<T, size>(gameCards, tt, tabIt, bitboard | currentPiece, remainingPieces - 1, i + 1, kingBit);
            currentPiece <<= 1;
        }
    }
}

template <typename T, size_t size>
void generateCombinations(const GameCards& gameCards, TranslationTable& tt, std::array<T, size>::iterator& tabIt) {
    for (U32 numPawns = 4; numPawns <= 4; numPawns--) {
        uint32_t kingBit = 1ULL;
        for (U32 kingI = 0; kingI < 25; kingI++) { // place king
            placePiece<T, size>(gameCards, tt, tabIt, kingBit, numPawns, 25, kingBit);
            kingBit <<= 1;
        }
    }
}


std::unique_ptr<MoveTable::MoveTable> MoveTable::build(const GameCards& gameCards) {
    
    std::cout << "building movetables" << std::endl;

    auto translationTable = std::make_unique<TranslationTable>();
    auto otherIt = translationTable->begin();
    generateCombinations<U32, 5 << 25>(gameCards, *translationTable, otherIt);

    auto table = std::make_unique<MoveTable>();
    auto tabIt = table->begin();
    generateCombinations<Move, CARDS_LUT.size() * TOTAL_HALF_POSITIONS>(gameCards, *translationTable, tabIt);


    std::cout << "created " << (tabIt - table->begin()) << " entries." << std::endl;
    return table;
}
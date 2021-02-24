#pragma once

#include <array>
#include <numeric>
#include <functional>
#include "Botama.h"
#include "Board.h"
#include "Score.h"
#include "Card.h"
#include "CardBoard.h"



class MoveTable;
class MoveTable {
public:
#pragma pack (push)
#pragma pack (1)
    struct Move {
        Move();
        Move(const MoveTable& moveTable, uint32_t bitboard, uint32_t kingbit, uint8_t card);
        union {
            struct {
                uint32_t address : 24;
                uint32_t card : 8;
            };
            uint32_t addressCard;
        };
        std::array<uint32_t, 5> afterTakeBoards; // address 
        uint32_t bitboard; // bitboard with all occupied positions
        // uint32_t kingbit; // bitboard with king position
        uint32_t bitboard_flipped;
        uint32_t kingbit_flipped;
        Score halfScore;
    };
    struct MoveList { // info about one players board pieces including every possible move
        union {
            std::array<std::array<Move, 5*8>, 10> moves{}; // TODO: possibly use templates and reduce that 20 number
            struct {
                std::array<uint32_t, sizeof(Move) * (4*8 + 1)> _padding;
                uint32_t somethingForTB;
            };
        };
    };
#pragma pack (pop)

    static constexpr std::array<U64, 5> HALF_POSITION_COUNT = { // index is number of pawns on the board
        25ULL,
        25ULL * (24) / (1),
        25ULL * (24*23) / (2*1),
        25ULL * (24*23*22) / (3*2*1),
        25ULL * (24*23*22*21) / (4*3*2*1),
    };
    static constexpr U64 TOTAL_HALF_POSITION_COUNT = 1 + std::accumulate(HALF_POSITION_COUNT.begin(), HALF_POSITION_COUNT.end(), 0, std::plus<U64>());

    typedef std::array<U32, 5ULL << 25> TranslationTable;
    typedef std::array<MoveList, TOTAL_HALF_POSITION_COUNT> Table;
    std::unique_ptr<TranslationTable> translationTable;
    std::unique_ptr<Table> table;
    void build(const GameCards& gameCards);
    
    uint32_t addressFromBitboards(uint32_t bitboard, uint32_t kingBit) const;
    
    std::array<Move, 2> toHalfBoards(const Board& board) const;
private:
    uint32_t ttAddressFromBitBoards(uint32_t bitboard, uint32_t kingBit) const;
    void createEntry(const GameCards& gameCards, TranslationTable::iterator& tabIt, uint32_t bitboard, U32 kingBit);
    void createEntry(const GameCards& gameCards, Table::iterator& tabIt, uint32_t bitboard, U32 kingBit);
    template <typename T, size_t size>
    void placePiece(const GameCards& gameCards, typename std::array<T, size>::iterator& tabIt, uint32_t bitboard, U32 remainingPieces, U32 stopAt, U32 kingBit);
    template <typename T, size_t size>
    void generateCombinations(const GameCards& gameCards, typename std::array<T, size>::iterator& tabIt);
};
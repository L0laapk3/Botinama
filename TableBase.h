#pragma once

#include <array>
#include <map>
#include "Botama.h"
#include "Board.h"
#include "CardBoard.h"


// generating the tablebase is not thread safe!! for now
namespace TableBase {

#pragma pack (push)
#pragma pack (1)
	struct BoardValue {
		BoardValue(const std::pair<Board, uint8_t>& pair);
		uint32_t boardComp; // max 6 men
		uint8_t DTW;
	};
#pragma pack (pop)

	extern bool done;

	extern std::vector<int8_t> wonBoards;

	template<bool isFirst, bool isMine>
	void addToTables(const GameCards& gameCards, const Board& board, const bool finished, const int8_t depthVal, const int threadNum);

	void singleDepthThread(const GameCards& gameCards, const int threadNum);
	U64 singleDepth(const GameCards& gameCards);
	
	template<bool templeWin>
	void placePieces(const GameCards& gameCards, U64 pieces, std::array<U32, 2> occupied, U32 beforeKing, U32 beforeOtherKing, U32 startAt, U32 spotsLeft, U32 minSpots0, U32 minSpotsAll, U32 myMaxPawns, U32 otherMaxPawns);
	void placePiecesTemple(const GameCards& gameCards, const Board& board, const bool finished, const int8_t depthVal, const int threadNum);
	void placePiecesDead(const GameCards& gameCards, const Board& board, const bool finished, const int8_t depthVal, const int threadNum);
	void init();
	uint8_t generate(const GameCards& gameCards, const U32 maxMen);

	U32 compress6Men(const Board& board);
	Board decompress6Men(U32 boardComp);

}
#pragma once

#include <array>
#include <map>
#include "Botama.h"
#include "Board.h"
#include "CardBoard.h"
#include <queue>


// generating the tablebase is not thread safe!! for now
namespace TableBase {
	extern std::unordered_map<Board, uint8_t, BoardHash> wonBoards;

	template<bool isMine>
	void addToTables(const GameCards& gameCards, const Board& board, const bool finished = false);

	bool singleDepth(const GameCards& gameCards);
	
	template<bool templeWin>
	void placePieces(const GameCards& gameCards, U64 pieces, std::array<U32, 2> occupied, U32 beforeKing, U32 beforeOtherKing, U32 startAt, U32 spotsLeft, U32 minSpots0, U32 minSpotsAll, U32 myMaxPawns, U32 otherMaxPawns);
	void placePiecesTemple(const GameCards& gameCards, const Board& board, const bool finished);
	void placePiecesDead(const GameCards& gameCards, const Board& board, const bool finished);
	uint8_t generate(const GameCards& gameCards, std::array<U32, 2> maxPawns = { 0, 0 });
}
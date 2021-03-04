#pragma once

#include <array>
#include <map>
#include "Botama.h"
#include "Board.h"
#include "CardBoard.h"


class Game;
class TableBase {
public:
	bool done = false;
	std::unique_ptr<std::array<int8_t, TBSIZE>> table = nullptr;
private:
	typedef std::array<std::atomic<uint64_t>, (TBSIZE+63)/64> BitTable;
	std::unique_ptr<BitTable> queue = nullptr;
	std::unique_ptr<BitTable> nextQueue = nullptr;

	template<bool isMine>
	static void addToTables(Game& game, const Board& board, const bool finished, const int8_t _, const int threadNum);

	static void singleDepthThread(Game& game, const int threadNum);
	static void firstDepthThread(Game& game, const int threadNum);
	U64 singleDepth(Game& game);
	
	template<bool templeWin>
	static void placePieces(Game& game, U64 pieces, std::array<U32, 2> occupied, U64 kings, U32 startAt, U32 spotsLeft, U32 minSpots0, U32 minSpotsAll, U32 myMaxPawns, U32 otherMaxPawns, U32 threadNum);
	static void placePiecesTemple(Game& game, const Board& board, const bool finished, const int8_t threadNum, const int _);
	static void placePiecesDead(Game& game, const Board& board, const bool finished, const int8_t threadNum, const int takenKingPos);

public:
	TableBase();
	void generate(Game& game);
};
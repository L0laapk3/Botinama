#pragma once

#include <array>
#include <map>
#include "Botama.h"
#include "Board.h"
#include "CardBoard.h"



constexpr U32 TBSIZE = 25*25*30*(TB_MEN <= 4 ? 26*26 : 26*26/2*26*26/2);


class Game;
// generating the tablebase is not thread safe!! for now
class TableBase {
public:
	bool done = false;
	std::unique_ptr<std::array<std::atomic<int8_t>, TBSIZE>> table = nullptr;
private:
	std::unique_ptr<std::array<std::atomic<int8_t>, TBSIZE>> pendingBoards = nullptr;
	
	std::vector<std::vector<Board>> queue{};
	std::vector<std::vector<Board>> currQueue{};

	template<bool isMine>
	static void addToTables(Game& game, const Board& board, const bool finished, const int8_t depthVal, const int threadNum);

	static void singleDepthThread(Game& game, const int threadNum);
	static void firstDepthThread(Game& game, const int threadNum);
	U64 singleDepth(Game& game);
	
	template<bool templeWin>
	static void placePieces(Game& game, U64 pieces, std::array<U32, 2> occupied, U32 beforeKing, U32 beforeOtherKing, U32 startAt, U32 spotsLeft, U32 minSpots0, U32 minSpotsAll, U32 myMaxPawns, U32 otherMaxPawns, U32 threadNum);
	static void placePiecesTemple(Game& game, const Board& board, const bool finished, const int8_t threadNum, const int _);
	static void placePiecesDead(Game& game, const Board& board, const bool finished, const int8_t threadNum, const int takenKingPos);

public:
	TableBase();
	void generate(Game& game);

	static U32 compress6Men(const Board& board);
	static U32 invertCompress6Men(const Board& board);
	static Board decompress6Men(U32 boardComp);
};
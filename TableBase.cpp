#include "Botama.h"
#include "TableBase.h"

#include <bitset>
#include <chrono>
#include <vector>
#include <fstream>
#include <mutex>
#include <atomic>

#include "BitScan.h"
#include "Game.h"
#ifdef USE_TB


uint16_t currDepth;
int8_t depthVal;
U64 numThreads = 0;


//TODO: fix race conditions lol
template<bool isMine>
void TableBase::addToTables(Game& game, const Board& board, const bool finished, const int8_t _, const int threadNum) {
	// this function assumed that it is called in the correct distanceToWin order
	// also assumes that there are no duplicate starting boards
	//board.print(game.cards, finished, true);

	if (finished)
		return;

	//board.print(game.cards);
	bool exploreChildren = false;
	U32 compressedBoard;
	if (isMine) {
		// you played this move, immediately add it to won boards
		// only insert and iterate if it doesnt already exist (keeps lowest distance)
		compressedBoard = board.compressToIndex();

		// auto decomp = decompress6Men(compressedBoard);
		// if (decomp.pieces != board.pieces) {
		// 	board.print(game.cards);
		// 	decomp.print(game.cards);
		// 	std::cout << std::bitset<64>(board.pieces) << std::endl << std::bitset<64>(decomp.pieces) << std::endl;
		// 	assert(decompress6Men(compressedBoard).pieces == board.pieces);
		// }

		exploreChildren = ((*game.tableBase.table)[compressedBoard] & 0x7F) == 0;
		if (exploreChildren) {
			(*game.tableBase.table)[compressedBoard] = depthVal;
			if (currDepth > 1)
				game.searchTime(board, 1000, 1, 0, currDepth+1);
		}

	} else {
		// opponents move. All forward moves must lead to a loss first
		// this function should only get called at most countForwardMoves times
		compressedBoard = board.invertCompressToIndex();
		auto& entry = (*game.tableBase.table)[compressedBoard];
		if (entry == 0) {
			exploreChildren = board.testForwardTB(game.cards, *game.tableBase.table);
			if (exploreChildren) {
				// if (currDepth+1 == 4)
				// 	board.print(game.cards);
				game.searchTime(board, 1000, 1, 0, currDepth+1);
			}
			entry = exploreChildren ? depthVal : 0x80;
		}
	}
	if (exploreChildren)
		(*game.tableBase.nextBoards)[compressedBoard/64] |= 1ULL << (compressedBoard % 64);
};

U32 maxMenPerSide;
U32 myMaxPawns;
U32 otherMaxPawns;

void TableBase::firstDepthThread(Game& game, const int threadNum) {
	{ // all temple wins
		U32 king = MASK_END_POSITIONS[0];
		Board board{ king | MASK_TURN };
		int i = threadNum / numThreads;
		board.pieces += i << INDEX_CARDS;
		for (; i < 30 * (threadNum + 1) / numThreads; i++) {
			board.reverseMoves<&TableBase::placePiecesTemple>(game, TB_MEN, 0, threadNum, 0);
			board.pieces += 1ULL << INDEX_CARDS;
		}
	}
	{ // all king kill wins
		U32 takenKingPos = 1ULL;
		U32 pos;
		for (pos = 0; pos < 24 * threadNum / numThreads; pos++)
			takenKingPos <<= (takenKingPos == MASK_END_POSITIONS[1]) + 1;
		for (; pos < 24 * (threadNum + 1) / numThreads; pos++) {
			takenKingPos <<= takenKingPos == MASK_END_POSITIONS[1];
			Board board{ takenKingPos | MASK_TURN };
			for (int i = 0; i < 30; i++) {
				//std::cout << "alive" << ((board.pieces >> INDEX_KINGS[1]) & 7) << std::endl;
				board.reverseMoves<&TableBase::placePiecesDead>(game, TB_MEN, 0, threadNum, takenKingPos);
				board.pieces += 1ULL << INDEX_CARDS;
			}
			board.pieces &= ~(0x1FULL << INDEX_CARDS);
			takenKingPos <<= 1;
		}
	}
}

void TableBase::singleDepthThread(Game& game, const int threadNum) {
	for (U64 chunk = game.tableBase.otherNextBoards->size() * threadNum / numThreads; chunk < game.tableBase.otherNextBoards->size() * (threadNum + 1) / numThreads; chunk++) {
		auto& entry = (*game.tableBase.otherNextBoards)[chunk];
		for (U64 i = chunk * 64; i < std::min((chunk + 1) * 64, TBSIZE); i++)
			if ((entry & (1ULL << (i % 64))) != 0) {
				const auto board = Board::decompressIndex(i, currDepth % 2 == 0);
				
				if (currDepth % 2 == 0)
					board.reverseMoves<&TableBase::addToTables<true>>(game, TB_MEN, maxMenPerSide, 0, threadNum);
				else
					board.reverseMoves<&TableBase::addToTables<false>>(game, TB_MEN, maxMenPerSide, 0, threadNum);
			}
		entry = 0;
	}
}


U64 TableBase::singleDepth(Game& game) {
	currDepth++;
	depthVal = std::min((currDepth / 2) + 1, 127);
	std::swap(nextBoards, otherNextBoards);
	atomic_thread_fence(std::memory_order_acq_rel);

	std::vector<std::thread> threads;
	for (int i = 0; i < numThreads; i++)
		threads.push_back(std::thread(&TableBase::singleDepthThread, std::ref(game), i));
	for (int i = 0; i < numThreads; i++)
		threads[i].join();
	atomic_thread_fence(std::memory_order_acq_rel);
	U64 total = 0;
#ifndef NDEBUGe
	for (U64 i = 0; i < nextBoards->size(); i++)
		total += _popcnt64((*nextBoards)[i]);
#endif
	return total;
}



template<bool templeWin>
void TableBase::placePieces(Game& game, U64 pieces, std::array<U32, 2> occupied, U64 kings, U32 startAt, U32 spotsLeft, U32 minSpots0, U32 minSpotsAll, U32 myMaxPawns, U32 otherMaxPawns, U32 threadNum) {
	if (spotsLeft == minSpotsAll) {
		Board board{ pieces, kings };
		//std::cout << std::bitset<64>((((U64)beforeOtherKing) << 32) & board.pieces) << ' ' << _popcnt64((((U64)beforeOtherKing) << 32) & board.pieces) << std::endl;
		//std::cout << ((board.pieces >> INDEX_KINGS[1]) & 7) << std::endl;
		// board.pieces |= _popcnt64((((U64)beforeOtherKing) << 32) & board.pieces) << INDEX_KINGS[1];
		if (templeWin) {
			// board.pieces |= _popcnt64(beforeKing & board.pieces) << INDEX_KINGS[0];
			//if (board.pieces & (1ULL << 22))
			//	board.print(game.cards);
			addToTables<true>(game, board, false, 0, threadNum);

			// board.pieces += 1ULL << INDEX_CARDS;
		} else {
			// board.pieces &= ~(7ULL << INDEX_KINGS[0]);
			//std::cout << std::bitset<25>(board.pieces >> 32) << ' ' << std::bitset<25>(board.pieces) << std::endl;
			//if (board.pieces & (1ULL << 22))
			//	board.print(game.cards);
			U32 kingI = 1;
			while (true) {
				U32 kingPos = _pdep_u32(kingI, board.pieces);
				if (!(kingPos & MASK_PIECES))
					break;
				kingI <<= 1;
				board.kings += kingPos;
				if (kingPos != MASK_END_POSITIONS[0])
					addToTables<true>(game, board, false, 0, threadNum);
				board.kings -= kingPos;
			}
		}
	} else {
		bool player = spotsLeft <= minSpots0;
		U32 piece = 1ULL << startAt;
		while (piece & MASK_PIECES) {
			startAt++;
			if (piece & ~occupied[player]) {
				placePieces<templeWin>(game, pieces | (((U64)piece) << (player ? 32 : 0)), { occupied[0] | piece, occupied[1] | piece }, kings, spotsLeft == minSpots0 + 1 ? 0 : startAt, spotsLeft - 1, minSpots0, minSpotsAll, myMaxPawns, otherMaxPawns, threadNum);
			}
			piece <<= 1;
		}
	}
}

void TableBase::placePiecesTemple(Game& game, const Board& board, const bool finished, const int8_t threadNum, const int _) {
	if (finished)
		return;
	U32 otherKing = 1ULL;
	for (U32 otherKingPos = 0; otherKingPos < 22; otherKingPos++) {
		while ((otherKing & board.pieces) || (otherKing == MASK_END_POSITIONS[1]) || (otherKing == MASK_END_POSITIONS[0]))
			otherKing <<= 1;
		U64 pieces = board.pieces | (((U64)otherKing) << 32);
		U32 occupied = board.pieces | otherKing;
		placePieces<true>(game, pieces, { occupied | MASK_END_POSITIONS[0], occupied }, (board.pieces & MASK_PIECES) | (((U64)otherKing) << 32), 0, 23, 23 - myMaxPawns, 23 - myMaxPawns - otherMaxPawns, myMaxPawns, otherMaxPawns, threadNum);
		otherKing <<= 1;
	}
}
void TableBase::placePiecesDead(Game& game, const Board& board, const bool finished, const int8_t threadNum, const int takenKingPos) {
	if (finished)
		return;
	U64 pieces = board.pieces | (((U64)takenKingPos) << 32);
	U32 occupied = board.pieces | takenKingPos;
	//std::cout << "dead" << ((board.pieces >> INDEX_KINGS[1]) & 7) << std::endl;
	placePieces<false>(game, pieces, { occupied, occupied }, ((U64)takenKingPos) << 32, 0, 23, 23 - myMaxPawns, 23 - myMaxPawns - otherMaxPawns, myMaxPawns, otherMaxPawns, threadNum);
}


TableBase::TableBase() {
	numThreads = 1;//std::max<U32>(1, std::thread::hardware_concurrency());
	if (table == nullptr)
		table = std::make_unique<std::array<int8_t, TBSIZE>>();
	nextBoards = std::make_unique<std::array<uint64_t, (TBSIZE+63)/64>>();
	otherNextBoards = std::make_unique<std::array<uint64_t, (TBSIZE+63)/64>>();
}

void TableBase::generate(Game& game) {
	std::cout << "generating " << TB_MEN << " men endgame tablebases using " << numThreads << " threads..." << std::endl;

	currDepth = 0;
	depthVal = 1;
	U32 menPerSide = (TB_MEN + 1) / 2;
	maxMenPerSide = std::min<U32>(std::min(menPerSide, TB_MEN - 1), 5);

	auto beginTime = std::chrono::steady_clock::now();
	auto beginTime2 = beginTime;


	for (myMaxPawns = 0; myMaxPawns <= maxMenPerSide - 1; myMaxPawns++)
		for (otherMaxPawns = 0; otherMaxPawns <= std::min(maxMenPerSide - 1, TB_MEN - myMaxPawns - 2); otherMaxPawns++) {
			atomic_thread_fence(std::memory_order_acq_rel);
			std::vector<std::thread> threads;
			for (int i = 0; i < numThreads; i++)
				threads.push_back(std::thread(&TableBase::firstDepthThread, std::ref(game), i));
			for (int i = 0; i < numThreads; i++)
				threads[i].join();
			atomic_thread_fence(std::memory_order_acq_rel);
		}

	U64 wonCount = 0;
	U64 total = 0;
#ifndef NDEBUGe
	for (U64 i = 0; i < nextBoards->size(); i++)
		total += _popcnt64((*nextBoards)[i]);
#endif
	do {
		const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime2).count());
		//printf("%9llu winning depth %2u boards in %6.2fs using %10llu lookups (%5.1fM lookups/s)\n", queue.size(), currDepth + 1, (float)time / 1000000, lookups, (float)lookups / time);
		printf("%10llu winning depth %3u boards in %6.2fs\n", total, currDepth + 1, (float)time / 1000000);
		wonCount += total;
		beginTime2 = std::chrono::steady_clock::now();
	} while ((total = singleDepth(game)));

	
	for (U64 i = 0; i < table->size(); i++)
		if ((*table)[i] == (int8_t)0x80)
			(*table)[i] = 0;
	
	auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
	printf("%10llu winning boards in %.3fs (%.2fM/s)\n", wonCount, (float)time / 1000000, (float)wonCount / time);


	nextBoards.reset();
	otherNextBoards.reset();

	beginTime = std::chrono::steady_clock::now();
	//U32 last = 0;
	//for (auto& d : evenWins) {
	//	U32 tmp = d.boardComp;
	//	d.boardComp -= last;
	//	last = tmp;
	//}
	
	time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());

	done = true;
}





#endif
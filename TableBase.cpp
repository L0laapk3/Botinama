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

int8_t storeDepth() {
	// if ((depth >> 1) >= 127)
	// 	std::cout << "depth overflow!! :(" << std::endl;
	return std::min((currDepth / 2) + 1, 127);
}


//TODO: fix race conditions lol
template<bool isMine>
void TableBase::addToTables(Game& game, const Board& board, const bool finished, const int8_t depthVal, const int threadNum) {
	// this function assumed that it is called in the correct distanceToWin order
	// also assumes that there are no duplicate starting boards
	//board.print(game.cards, finished, true);

	//lookups++;
	int8_t zero = 0;

	if (finished)
		return;

	//board.print(game.cards);
	bool exploreChildren = false;
	if (isMine) {
		// you played this move, immediately add it to won boards
		// only insert and iterate if it doesnt already exist (keeps lowest distance)
		U32 compressedBoard = compress6Men(board);

		// auto decomp = decompress6Men(compressedBoard);
		// if (decomp.pieces != board.pieces) {
		// 	board.print(game.cards);
		// 	decomp.print(game.cards);
		// 	std::cout << std::bitset<64>(board.pieces) << std::endl << std::bitset<64>(decomp.pieces) << std::endl;
		// 	assert(decompress6Men(compressedBoard).pieces == board.pieces);
		// }

		exploreChildren = (*game.tableBase.table)[compressedBoard].compare_exchange_strong(zero, depthVal);

	} else {
		// opponents move. All forward moves must lead to a loss first
		// this function should only get called at most countForwardMoves times
		U32 invCompressedBoard = invertCompress6Men(board);
		auto& pending = (*game.tableBase.pendingBoards)[invCompressedBoard];
		if (pending.compare_exchange_strong(zero, -1)) {
			const int8_t moveCount = board.countForwardMoves(game.cards);
			const int8_t hitCount = pending.fetch_add(moveCount + 1);
			exploreChildren = -hitCount == moveCount;
		} else
			exploreChildren = pending.fetch_sub(1) == 2;
		if (exploreChildren)
			(*game.tableBase.table)[invCompressedBoard].store(-depthVal);
	}
	if (exploreChildren)
		game.tableBase.queue[threadNum].push_back(board);
};

U32 maxMenPerSide;
U32 myMaxPawns;
U32 otherMaxPawns;

constexpr size_t BLOCKSIZE = 1 << 10;
struct Job { size_t index; size_t start; };
std::vector<Job> jobs{};
std::atomic<size_t> jobIndex;

void TableBase::firstDepthThread(Game& game, const int threadNum) {
	{ // all temple wins
		U32 king = MASK_END_POSITIONS[0];
		Board board{ king | MASK_TURN };
		int i = threadNum / game.tableBase.queue.size();
		board.pieces += i << INDEX_CARDS;
		for (; i < 30 * (threadNum + 1) / game.tableBase.queue.size(); i++) {
			board.reverseMoves<&TableBase::placePiecesTemple>(game, TB_MEN, 0, threadNum, 0);
			board.pieces += 1ULL << INDEX_CARDS;
		}
	}
	{ // all king kill wins
		U32 takenKingPos = 1ULL;
		U32 pos;
		for (pos = 0; pos < 24 * threadNum / game.tableBase.queue.size(); pos++)
			takenKingPos <<= (takenKingPos == MASK_END_POSITIONS[1]) + 1;
		for (; pos < 24 * (threadNum + 1) / game.tableBase.queue.size(); pos++) {
			takenKingPos <<= takenKingPos == MASK_END_POSITIONS[1];
			Board board{ takenKingPos | MASK_TURN | (7ULL << INDEX_KINGS[0]) };
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
	const int8_t depth = storeDepth();
	while (true) {
		const auto jobI = jobIndex++;
		// if (threadNum == 1)
		// 	std::cout << jobI << ' ' << jobs.size() << std::endl;
		if (jobI >= jobs.size())
			break;
		const auto& job = jobs[jobI];
		for (int index = job.start; index < std::min(job.start + BLOCKSIZE, game.tableBase.currQueue[job.index].size()); index++) {
			const auto& board = game.tableBase.currQueue[job.index][index];
			if (currDepth % 2 == 0)
				board.reverseMoves<&TableBase::addToTables<true>>(game, TB_MEN, maxMenPerSide, depth, threadNum);
			else
				board.reverseMoves<&TableBase::addToTables<false>>(game, TB_MEN, maxMenPerSide, depth, threadNum);
		}
	}
}


U64 TableBase::singleDepth(Game& game) {
	currDepth++;
	std::swap(queue, currQueue);

	jobs.clear();
	jobIndex = 0;
	for (size_t i = 0; i < queue.size(); i++) {
		size_t start = 0;
		while (start + 1 < currQueue[i].size()) {
			jobs.push_back({ i, start });
			start += BLOCKSIZE;
		}
	}

	std::vector<std::thread> threads;
	for (int i = 0; i < queue.size(); i++)
		threads.push_back(std::thread(&TableBase::singleDepthThread, std::ref(game), i));
	U64 total = 0;
	for (int i = 0; i < queue.size(); i++)
		threads[i].join();
	atomic_thread_fence(std::memory_order_acq_rel);
	for (int i = 0; i < queue.size(); i++) {
		currQueue[i].clear();
		total += queue[i].size();
	}
	return total;
}



template<bool templeWin>
void TableBase::placePieces(Game& game, U64 pieces, std::array<U32, 2> occupied, U32 beforeKing, U32 beforeOtherKing, U32 startAt, U32 spotsLeft, U32 minSpots0, U32 minSpotsAll, U32 myMaxPawns, U32 otherMaxPawns, U32 threadNum) {
	if (spotsLeft == minSpotsAll) {
		Board board{ pieces };
		//std::cout << std::bitset<64>((((U64)beforeOtherKing) << 32) & board.pieces) << ' ' << _popcnt64((((U64)beforeOtherKing) << 32) & board.pieces) << std::endl;
		//std::cout << ((board.pieces >> INDEX_KINGS[1]) & 7) << std::endl;
		board.pieces |= _popcnt64((((U64)beforeOtherKing) << 32) & board.pieces) << INDEX_KINGS[1];
		if (templeWin) {
			board.pieces |= _popcnt64(beforeKing & board.pieces) << INDEX_KINGS[0];
			//if (board.pieces & (1ULL << 22))
			//	board.print(game.cards);
			addToTables<true>(game, board, false, storeDepth(), threadNum);

			board.pieces += 1ULL << INDEX_CARDS;
		} else {
			board.pieces &= ~(7ULL << INDEX_KINGS[0]);
			//std::cout << std::bitset<25>(board.pieces >> 32) << ' ' << std::bitset<25>(board.pieces) << std::endl;
			//if (board.pieces & (1ULL << 22))
			//	board.print(game.cards);
			U32 kingI = 1;
			while (true) {
				U32 kingPos = _pdep_u32(kingI, board.pieces);
				if (!(kingPos & MASK_PIECES))
					break;
				kingI <<= 1;
				if (kingPos != MASK_END_POSITIONS[0])
					addToTables<true>(game, board, false, storeDepth(), threadNum);
				board.pieces += 1ULL << INDEX_KINGS[0];
			}
		}
	} else {
		bool player = spotsLeft <= minSpots0;
		U32 piece = 1ULL << startAt;
		while (piece & MASK_PIECES) {
			startAt++;
			if (piece & ~occupied[player]) {
				placePieces<templeWin>(game, pieces | (((U64)piece) << (player ? 32 : 0)), { occupied[0] | piece, occupied[1] | piece }, beforeKing, beforeOtherKing, spotsLeft == minSpots0 + 1 ? 0 : startAt, spotsLeft - 1, minSpots0, minSpotsAll, myMaxPawns, otherMaxPawns, threadNum);
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
		placePieces<true>(game, pieces, { occupied | MASK_END_POSITIONS[0], occupied }, (board.pieces & MASK_PIECES) - 1, otherKing - 1, 0, 23, 23 - myMaxPawns, 23 - myMaxPawns - otherMaxPawns, myMaxPawns, otherMaxPawns, threadNum);
		otherKing <<= 1;
	}
}
void TableBase::placePiecesDead(Game& game, const Board& board, const bool finished, const int8_t threadNum, const int takenKingPos) {
	if (finished)
		return;
	U64 pieces = board.pieces | (((U64)takenKingPos) << 32);
	U32 occupied = board.pieces | takenKingPos;
	//std::cout << "dead" << ((board.pieces >> INDEX_KINGS[1]) & 7) << std::endl;
	placePieces<false>(game, pieces, { occupied, occupied }, 0, takenKingPos - 1, 0, 23, 23 - myMaxPawns, 23 - myMaxPawns - otherMaxPawns, myMaxPawns, otherMaxPawns, threadNum);
}


TableBase::TableBase() {
	const U32 numThreads = std::max<U32>(1, std::thread::hardware_concurrency());
	queue.resize(numThreads);
	currQueue.resize(numThreads);
	for (int i = 0; i < numThreads; i++) {
		queue[i].reserve(48828 * std::pow(4, TB_MEN) / numThreads);
		currQueue[i].reserve(48828 * std::pow(4, TB_MEN) / numThreads);
	}
	if (table == nullptr)
		table = std::make_unique<std::array<std::atomic<int8_t>, TBSIZE>>();
	pendingBoards = std::make_unique<std::array<std::atomic<int8_t>, TBSIZE>>();
}

void TableBase::generate(Game& game) {
	std::cout << "generating " << TB_MEN << " men endgame tablebases using " << queue.size() << " threads..." << std::endl;

	currDepth = 0;
	U32 menPerSide = (TB_MEN + 1) / 2;
	maxMenPerSide = std::min<U32>(std::min(menPerSide, TB_MEN - 1), 5);

	auto beginTime = std::chrono::steady_clock::now();
	auto beginTime2 = beginTime;


	for (myMaxPawns = 0; myMaxPawns <= maxMenPerSide - 1; myMaxPawns++)
		for (otherMaxPawns = 0; otherMaxPawns <= std::min(maxMenPerSide - 1, TB_MEN - myMaxPawns - 2); otherMaxPawns++) {
			std::vector<std::thread> threads;
			for (int i = 0; i < queue.size(); i++)
				threads.push_back(std::thread(&TableBase::firstDepthThread, std::ref(game), i));
			for (int i = 0; i < queue.size(); i++)
				threads[i].join();
			atomic_thread_fence(std::memory_order_acq_rel);
		}

	U64 wonCount = 0;
	U64 total = 0;
	for (int i = 0; i < queue.size(); i++)
		total += queue[i].size();
	do {
		const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime2).count());
		//printf("%9llu winning depth %2u boards in %6.2fs using %10llu lookups (%5.1fM lookups/s)\n", queue.size(), currDepth + 1, (float)time / 1000000, lookups, (float)lookups / time);
		printf("%10llu winning depth %3u boards in %6.2fs\n", total, currDepth + 1, (float)time / 1000000);
		wonCount += total;
		beginTime2 = std::chrono::steady_clock::now();
	} while ((total = singleDepth(game)));
	
	auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
	printf("%10llu winning boards in %.3fs (%.2fM/s)\n", wonCount, (float)time / 1000000, (float)wonCount / time);


	queue.clear();
	queue.shrink_to_fit();
	currQueue.clear();	
	currQueue.shrink_to_fit();
	pendingBoards.reset();

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










U32 TableBase::compress6Men(const Board& board) {
	U32 boardComp = 0;
	U32 bluePieces = board.pieces & MASK_PIECES;
	U32 redPieces = (board.pieces >> 32) & MASK_PIECES;
	const U32 king = _pdep_u32(1 << ((board.pieces >> INDEX_KINGS[0]) & 7), bluePieces);
	const U32 otherKing = _pdep_u32(1 << ((board.pieces >> INDEX_KINGS[1]) & 7), redPieces);
	unsigned long kingI, otherKingI;
	_BitScanForward(&kingI, king);
	_BitScanForward(&otherKingI, otherKing);
	boardComp = boardComp * 25 + kingI;
	boardComp = boardComp * 25 + otherKingI;
	bluePieces &= ~king;
	redPieces &= ~otherKing;

	unsigned long piece1I = 25, otherPiece1I = 25;
	_BitScanForward(&piece1I, bluePieces);
	_BitScanForward(&otherPiece1I, redPieces);
	bluePieces &= ~(1 << piece1I);
	redPieces &= ~(1 << otherPiece1I);
	
	if (TB_MEN <= 4) {
		boardComp = boardComp * 26 + piece1I;
		boardComp = boardComp * 26 + otherPiece1I;
	} else {
		unsigned long piece2I = 25, otherPiece2I = 25;
		_BitScanForward(&piece2I, bluePieces);
		_BitScanForward(&otherPiece2I, redPieces);
		const U32 pieceValue = piece1I * 26 + piece2I;
		const U32 otherPieceValue = otherPiece1I * 26 + otherPiece2I;

		boardComp = boardComp * 26 * 13 + std::min(pieceValue, 26*26-1 - pieceValue);
		boardComp = boardComp * 26 * 13 + std::min(otherPieceValue, 26*26-1 - otherPieceValue);
	}

	boardComp = boardComp * 30 + ((board.pieces & MASK_CARDS) >> INDEX_CARDS);

	// if (decompress6Men(boardComp).pieces != board.pieces) {
	// 	std::cout << std::bitset<64>(decompress6Men(boardComp).pieces) << std::endl << std::bitset<64>(board.pieces) << std::endl;
	// 	assert(decompress6Men(boardComp).pieces == board.pieces);
	// }

	return boardComp;
}

U32 TableBase::invertCompress6Men(const Board& board) {
	U32 boardComp = 0;
	U32 bluePieces = board.pieces & MASK_PIECES;
	U32 redPieces = (board.pieces >> 32) & MASK_PIECES;
	const U32 king = _pdep_u32(1 << ((board.pieces >> INDEX_KINGS[0]) & 7), bluePieces);
	const U32 otherKing = _pdep_u32(1 << ((board.pieces >> INDEX_KINGS[1]) & 7), redPieces);
	unsigned long kingI, otherKingI;
	_BitScanForward(&kingI, king);
	_BitScanForward(&otherKingI, otherKing);
	boardComp = boardComp * 25 + 24 - otherKingI;
	boardComp = boardComp * 25 + 24 - kingI;
	bluePieces &= ~king;
	redPieces &= ~otherKing;

	unsigned long piece1I = 0, otherPiece1I = 0;
	_BitScanReverse(&piece1I, bluePieces << 1);
	_BitScanReverse(&otherPiece1I, redPieces << 1);
	bluePieces &= ~(1 << (piece1I - 1));
	redPieces &= ~(1 << (otherPiece1I - 1));
	piece1I = 25 - piece1I;
	otherPiece1I = 25 - otherPiece1I;
	
	if (TB_MEN <= 4) {
		boardComp = boardComp * 26 + otherPiece1I;
		boardComp = boardComp * 26 + piece1I;
	} else {
		unsigned long piece2I = 0, otherPiece2I = 0;
		_BitScanReverse(&piece2I, bluePieces << 1);
		_BitScanReverse(&otherPiece2I, redPieces << 1);
		piece2I = 25 - piece2I;
		otherPiece2I = 25 - otherPiece2I;

		const U32 pieceValue = piece1I * 26 + piece2I;
		const U32 otherPieceValue = otherPiece1I * 26 + otherPiece2I;
		boardComp = boardComp * 26 * 13 + std::min(otherPieceValue, 26*26-1 - otherPieceValue);
		boardComp = boardComp * 26 * 13 + std::min(pieceValue, 26*26-1 - pieceValue);
	}

	boardComp = boardComp * 30 + CARDS_INVERT[(board.pieces & MASK_CARDS) >> INDEX_CARDS];
	
	// if (decompress6Men(boardComp).pieces != board.invert().pieces) {
	// 	std::cout << std::bitset<64>(decompress6Men(boardComp).pieces) << std::endl << std::bitset<64>(board.invert().pieces) << std::endl;
	// 	assert(decompress6Men(boardComp).pieces == board.invert().pieces);
	// }

	return boardComp;
}

Board TableBase::decompress6Men(U32 boardComp) {
	U64 pieces = 0;
	pieces |= ((U64)boardComp % 30) << INDEX_CARDS;
	boardComp /= 30;

	U32 piece1I, otherPiece1I;
	
	if (TB_MEN <= 4) {
		otherPiece1I = boardComp % 26;
		boardComp /= 26;
		pieces |= (1ULL << (otherPiece1I | 32)) & (MASK_PIECES << 32);
		piece1I = boardComp % 26;
		boardComp /= 26;
		pieces |= (1ULL << piece1I) & MASK_PIECES;
	} else {
		U32 otherPieceValue = boardComp % (26 * 13);
		otherPiece1I = otherPieceValue % 26;
		U32 otherPiece2I = otherPieceValue / 26;
		if (otherPiece1I <= otherPiece2I) {
			otherPieceValue = 26*26-1 - otherPieceValue;
			otherPiece1I = otherPieceValue % 26;
			otherPiece2I = otherPieceValue / 26;
		}
		pieces |= ((1ULL << (otherPiece1I | 32)) | (1ULL << (otherPiece2I + 32))) & (MASK_PIECES << 32);
		boardComp /= 26 * 13;
		
		U32 pieceValue = boardComp % (26 * 13);
		piece1I = pieceValue % 26;
		U32 piece2I = pieceValue / 26;
		if (piece1I <= piece2I) {
			pieceValue = 26*26-1 - pieceValue;
			piece1I = pieceValue % 26;
			piece2I = pieceValue / 26;
		}
		pieces |= ((1ULL << piece1I) | (1ULL << piece2I)) & MASK_PIECES;
		boardComp /= 26 * 13;
	}

	U32 otherKingI = boardComp % 25;
	boardComp /= 25;
	pieces |= (1ULL << (otherKingI + 32));
	pieces |= ((U64)_popcnt64(((1ULL << (otherKingI + 32)) - (1ULL << 32)) & pieces)) << INDEX_KINGS[1];

	U32 kingI = boardComp % 25;
	boardComp /= 25;
	pieces |= (1ULL << kingI);
	pieces |= ((U64)_popcnt32(((1ULL << kingI) - 1) & pieces)) << INDEX_KINGS[0];

	return Board{ pieces };
}
#endif
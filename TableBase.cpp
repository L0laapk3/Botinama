
#include "TableBase.h"

#include <bitset>
#include <chrono>
#include <vector>
#include <fstream>
#include <mutex>

#include "BitScan.h"



constexpr U32 TABLESIZE = 25*25*26*26/2*26*26/2*30;

std::vector<int8_t> TableBase::wonBoards{};
std::vector<uint8_t> pendingBoards{};

std::vector<Board> queue{};

uint16_t currDepth;

int8_t storeDepth() {
	// if ((depth >> 1) >= 127)
	// 	std::cout << "depth overflow!! :(" << std::endl;
	return std::min((currDepth >> 1) + 1, 127);
}

template<bool doQueue, bool isMine>
void TableBase::addToTables(const GameCards& gameCards, const Board& board, const bool finished, const int8_t depthVal) {
	// this function assumed that it is called in the correct distanceToWin order
	// also assumes that there are no duplicate starting boards
	//board.print(gameCards, finished, true);

	//lookups++;

	if (finished)
		return;

	U32 compressedBoard = compress6Men(board);
	//board.print(gameCards);
	bool exploreChildren = false;
	if (isMine) {
		// you played this move, immediately add it to won boards
		// only insert and iterate if it doesnt already exist (keeps lowest distance)
		
		exploreChildren = wonBoards[compressedBoard*2+isMine] == 0;
		if (exploreChildren) {
			wonBoards[compressedBoard*2+isMine] = depthVal;
			wonBoards[compress6Men(board.invert())*2+!isMine] = -depthVal;
		}

	} else {
		// opponents move. All forward moves must lead to a loss first
		// this function should only get called at most countForwardMoves times
		//assert(wonBoards.end() == wonBoards.find(board));
		if (pendingBoards[compressedBoard] == 0) {
			uint8_t actionCount = 255 - (board.countForwardMoves(gameCards) - 1);
			pendingBoards[compressedBoard] = actionCount;
			exploreChildren = actionCount == 255;
		} else
			exploreChildren = ++pendingBoards[compressedBoard] == 255;
		if (exploreChildren) {
			wonBoards[compressedBoard*2+isMine] = depthVal;
			wonBoards[compress6Men(board.invert())*2+!isMine] = -depthVal;
		}
	}
	if (doQueue && exploreChildren) {
		queue.push_back(board);
		// board.searchTime(gameCards, 1000, 0, currDepth + 1);
		// board.invert().searchTime(gameCards, 1000, 0, currDepth + 1);
		//if (currDepth > 1 || true) {
		//	//std::cout << std::endl << "START THE SEARCH" << std::endl << std::endl;
		//	if (!board.searchWinIn(gameCards, currDepth + 1)) {
		//		std::cout << "win not found" << std::endl;
		//		board.print(gameCards);
		//		board.searchWinIn(gameCards, currDepth + 1);
		//		assert(0);
		//	}
		//}(
	}
};


U32 maxMen;
U32 maxMenPerSide;
std::vector<Board> currQueue{};
bool TableBase::singleDepth(const GameCards& gameCards) {
	currDepth++;
	std::swap(queue, currQueue);
	for (const Board& board : currQueue)
		if (currDepth % 2 == 0)
			board.reverseMoves<*addToTables<true, true>>(gameCards, maxMen, maxMenPerSide, storeDepth());
		else
			board.reverseMoves<*addToTables<true, false>>(gameCards, maxMen, maxMenPerSide, storeDepth());
	currQueue.clear();
	return queue.size();
}



template<bool templeWin>
void TableBase::placePieces(const GameCards& gameCards, U64 pieces, std::array<U32, 2> occupied, U32 beforeKing, U32 beforeOtherKing, U32 startAt, U32 spotsLeft, U32 minSpots0, U32 minSpotsAll, U32 myMaxPawns, U32 otherMaxPawns) {
	if (spotsLeft == minSpotsAll) {
		Board board{ pieces };
		//std::cout << std::bitset<64>((((U64)beforeOtherKing) << 32) & board.pieces) << ' ' << _popcnt64((((U64)beforeOtherKing) << 32) & board.pieces) << std::endl;
		//std::cout << ((board.pieces >> INDEX_KINGS[1]) & 7) << std::endl;
		board.pieces |= _popcnt64((((U64)beforeOtherKing) << 32) & board.pieces) << INDEX_KINGS[1];
		if (templeWin) {
			board.pieces |= _popcnt64(beforeKing & board.pieces) << INDEX_KINGS[0];
			//if (board.pieces & (1ULL << 22))
			//	board.print(gameCards);
			addToTables<true, true>(gameCards, board, false, storeDepth());

			board.pieces += 1ULL << INDEX_CARDS;
		} else {
			board.pieces &= ~(7ULL << INDEX_KINGS[0]);
			//std::cout << std::bitset<25>(board.pieces >> 32) << ' ' << std::bitset<25>(board.pieces) << std::endl;
			//if (board.pieces & (1ULL << 22))
			//	board.print(gameCards);
			U32 kingI = 1;
			while (true) {
				U32 kingPos = _pdep_u32(kingI, board.pieces);
				if (!(kingPos & MASK_PIECES))
					break;
				kingI <<= 1;
				if (kingPos != MASK_END_POSITIONS[0])
					addToTables<true, true>(gameCards, board, false, storeDepth());
				board.pieces += 1ULL << INDEX_KINGS[0];
			}
		}
	} else {
		bool player = spotsLeft <= minSpots0;
		U32 piece = 1ULL << startAt;
		while (piece & MASK_PIECES) {
			startAt++;
			if (piece & ~occupied[player]) {
				placePieces<templeWin>(gameCards, pieces | (((U64)piece) << (player ? 32 : 0)), { occupied[0] | piece, occupied[1] | piece }, beforeKing, beforeOtherKing, spotsLeft == minSpots0 + 1 ? 0 : startAt, spotsLeft - 1, minSpots0, minSpotsAll, myMaxPawns, otherMaxPawns);
			}
			piece <<= 1;
		}
	}
}

U32 myMaxPawns;
U32 otherMaxPawns;
void TableBase::placePiecesTemple(const GameCards& gameCards, const Board& board, const bool finished, const int8_t depthVal) {
	if (finished)
		return;
	U32 otherKing = 1ULL;
	for (U32 otherKingPos = 0; otherKingPos < 22; otherKingPos++) {
		while ((otherKing & board.pieces) || (otherKing == MASK_END_POSITIONS[1]) || (otherKing == MASK_END_POSITIONS[0]))
			otherKing <<= 1;
		U64 pieces = board.pieces | (((U64)otherKing) << 32);
		U32 occupied = board.pieces | otherKing;
		placePieces<true>(gameCards, pieces, { occupied | MASK_END_POSITIONS[0], occupied }, (board.pieces & MASK_PIECES) - 1, otherKing - 1, 0, 23, 23 - myMaxPawns, 23 - myMaxPawns - otherMaxPawns, myMaxPawns, otherMaxPawns);
		otherKing <<= 1;
	}
}
U32 takenKingPos;
void TableBase::placePiecesDead(const GameCards& gameCards, const Board& board, const bool finished, const int8_t depthVal) {
	if (finished)
		return;
	U64 pieces = board.pieces | (((U64)takenKingPos) << 32);
	U32 occupied = board.pieces | takenKingPos;
	//std::cout << "dead" << ((board.pieces >> INDEX_KINGS[1]) & 7) << std::endl;
	placePieces<false>(gameCards, pieces, { occupied, occupied }, 0, takenKingPos - 1, 0, 23, 23 - myMaxPawns, 23 - myMaxPawns - otherMaxPawns, myMaxPawns, otherMaxPawns);
}

void TableBase::init() {
	currDepth = 0;
	// queue.reserve(1E9);
	// currQueue.reserve(1E9);
	wonBoards.resize(TABLESIZE*2, 0);
	// pendingBoards.resize(TABLESIZE);
}

uint8_t TableBase::generate(const GameCards& gameCards, const U32 men) {


	queue.reserve(1E9);
	currQueue.reserve(1E9);
	wonBoards.resize(TABLESIZE*2, 0);
	pendingBoards.resize(TABLESIZE);

	std::cout << "generating endgame.." << std::endl;

	U32 menPerSide = (men + 1) / 2;
	maxMen = men;
	maxMenPerSide = std::min<U32>(std::min(menPerSide, men - 1), 5);

	auto beginTime = std::chrono::steady_clock::now();
	auto beginTime2 = beginTime;

	for (myMaxPawns = 0; myMaxPawns <= maxMenPerSide - 1; myMaxPawns++)
		for (otherMaxPawns = 0; otherMaxPawns <= std::min(maxMenPerSide - 1, maxMen - myMaxPawns - 2); otherMaxPawns++) {
			{ // all temple wins
				U32 king = MASK_END_POSITIONS[0];
				Board board{ king | MASK_TURN };
				for (int i = 0; i < 30; i++) {
					board.reverseMoves<*placePiecesTemple>(gameCards, maxMen, 0, 0);
					board.pieces += 1ULL << INDEX_CARDS;
				}
			}

			{ // all king kill wins
				takenKingPos = 1ULL;
				for (U32 pos = 0; pos < 24; pos++) {
					takenKingPos <<= takenKingPos == MASK_END_POSITIONS[1];
					Board board{ takenKingPos | MASK_TURN | (7ULL << INDEX_KINGS[0]) };
					for (int i = 0; i < 30; i++) {
						//std::cout << "alive" << ((board.pieces >> INDEX_KINGS[1]) & 7) << std::endl;
						board.reverseMoves<*placePiecesDead>(gameCards, maxMen, 0, 0);
						board.pieces += 1ULL << INDEX_CARDS;
					}
					board.pieces &= ~(0x1FULL << INDEX_CARDS);
					takenKingPos <<= 1;
				}
			}
		}
	U64 wonCount = 0;
	do {
		const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime2).count());
		//printf("%9llu winning depth %2u boards in %6.2fs using %10llu lookups (%5.1fM lookups/s)\n", queue.size(), currDepth + 1, (float)time / 1000000, lookups, (float)lookups / time);
		printf("%9llu winning depth %3u boards in %6.2fs\n", queue.size(), currDepth + 1, (float)time / 1000000);
		wonCount += queue.size();
		beginTime2 = std::chrono::steady_clock::now();
	} while (singleDepth(gameCards));
	
	auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
	//std::cout << wonb (float)time / 1000 << "ms" << std::endl;
	printf("%9llu winning boards in %.3fs (%.0fk/s)\n", wonCount, (float)time / 1000000, (float)wonCount * 1000 / time);

	queue.clear();
	queue.shrink_to_fit();
	currQueue.clear();
	currQueue.shrink_to_fit();
	pendingBoards.clear();
	pendingBoards.shrink_to_fit();

	beginTime = std::chrono::steady_clock::now();
	//U32 last = 0;
	//for (auto& d : evenWins) {
	//	U32 tmp = d.boardComp;
	//	d.boardComp -= last;
	//	last = tmp;
	//}
	
	time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());

	return currDepth;
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
	
	unsigned long piece2I = 25, otherPiece2I = 25;
	_BitScanForward(&piece2I, bluePieces);
	_BitScanForward(&otherPiece2I, redPieces);
	const U32 pieceValue = piece1I * 26 + piece2I;
	const U32 otherPieceValue = otherPiece1I * 26 + otherPiece2I;
	boardComp = boardComp * 26 * 13 + std::min(pieceValue, 26*26-1 - pieceValue);
	boardComp = boardComp * 26 * 13 + std::min(otherPieceValue, 26*26-1 - otherPieceValue);


	//boardComp = boardComp * 2  + ((pieces & MASK_TURN) >> INDEX_TURN);
	boardComp = boardComp * 30 + ((board.pieces & MASK_CARDS) >> INDEX_CARDS);

	//U64 decomp = decompress6Men(boardComp).pieces;
	//if (decomp != (board.pieces & ~(U64)MASK_TURN)) {
	//	std::cout << std::bitset<64>(board.pieces) << std::endl;
	//	std::cout << std::bitset<64>(decomp) << std::endl;
	//	assert(decomp == (board.pieces & ~(U64)MASK_TURN));
	//}

	return boardComp;
}

Board TableBase::decompress6Men(U32 boardComp) { // still bugged lol
	U64 pieces = 0;
	pieces |= ((U64)boardComp % 30) << INDEX_CARDS;
	boardComp /= 30;

	U32 otherPieceValue = boardComp % (26 * 13);
	U32 otherPiece1I = otherPieceValue % 26, otherPiece2I = otherPieceValue / 26;
	if (otherPiece1I <= otherPiece2I) {
		otherPieceValue = 26*26-1 - otherPieceValue;
		otherPiece1I = otherPieceValue % 26;
		otherPiece2I = otherPieceValue / 26;
	}
	pieces |= ((1ULL << (otherPiece1I | 32)) | (1ULL << (otherPiece2I | 32))) & (MASK_PIECES << 32);
	boardComp /= 26 * 13;
	
	U32 pieceValue = boardComp % (26 * 13);
	U32 piece1I = pieceValue % 26, piece2I = pieceValue / 26;
	if (piece1I <= piece2I) {
		pieceValue = 26*26-1 - pieceValue;
		piece1I = pieceValue % 26;
		piece2I = pieceValue / 26;
	}
	pieces |= ((1ULL << piece1I) | (1ULL << piece2I)) & MASK_PIECES;
	boardComp /= 26 * 13;

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


TableBase::BoardValue::BoardValue(const std::pair<Board, uint8_t>& pair) : DTW(pair.second), boardComp(compress6Men(pair.first)) { }

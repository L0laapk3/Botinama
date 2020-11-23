
#include "TableBase.h"

#include <bitset>
#include <chrono>
#include <vector>
#include <fstream>
#include <mutex>

#include "BitScan.h"
//#include "bxzstr/bxzstr.hpp"



constexpr U32 TABLESIZE = 25*25*26*26/2*26*26/2*30;

std::vector<uint8_t> wonEvenBoards{};
std::vector<uint8_t> wonOddBoards{};
std::vector<uint8_t> pendingBoards{};

std::vector<Board> queue{};

uint16_t currDepth;

uint8_t storeDepth() {
	return currDepth + 1;
	//return ~((uint8_t)0) - (currDepth >> 3);
}

U64 lookups = 0;
template<bool isMine>
void TableBase::addToTables(const GameCards& gameCards, const Board& board, const bool finished) {
	// this function assumed that it is called in the correct distanceToWin order
	// also assumes that there are no duplicate starting boards
	//board.print(gameCards, finished, true);

	//lookups++;

	if (finished)
		return;

	U32 compressedBoard = compress6Men(board.pieces);
	//board.print(gameCards);
	bool exploreChildren = false;
	if (isMine) {
		// you played this move, immediately add it to won boards
		// only insert and iterate if it doesnt already exist (keeps lowest distance)

		exploreChildren = wonOddBoards[compressedBoard] == 0;
		if (exploreChildren)
			wonOddBoards[compressedBoard] = storeDepth();

	} else {
		// opponents move. All forward moves must lead to a loss first
		// this function should only get called at most countForwardMoves times
		//assert(wonBoards.end() == wonBoards.find(board));
		if (pendingBoards[compressedBoard] == 0) {
			pendingBoards[compressedBoard] = 255 - (board.countForwardMoves(gameCards) - 1);
		} else {
			exploreChildren = ++pendingBoards[compressedBoard] == 255;
			if (exploreChildren)
				wonEvenBoards[compressedBoard] = storeDepth(); //should be unique and not require lock
		}
	}
	if (exploreChildren) {
		queue.push_back(board);
		//board.searchTime(gameCards, 1000, 0, currDepth + 1);
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
void TableBase::singleDepthThread(const GameCards& gameCards, const int threadNum, const int threadCount) {
	int i = currQueue.size() * threadNum / threadCount;
	const int stopAt = currQueue.size() * (threadNum + 1) / threadCount;
}

bool TableBase::singleDepth(const GameCards& gameCards) {
	currDepth++;
	std::swap(queue, currQueue);
	for (const Board& board : currQueue)
		if (currDepth % 2 == 0)
			board.reverseMoves<*addToTables<true>>(gameCards, maxMen, maxMenPerSide);
		else
			board.reverseMoves<*addToTables<false>>(gameCards, maxMen, maxMenPerSide);
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
			addToTables<true>(gameCards, board);

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
					addToTables<true>(gameCards, board);
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
void TableBase::placePiecesTemple(const GameCards& gameCards, const Board& board, const bool finished) {
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
void TableBase::placePiecesDead(const GameCards& gameCards, const Board& board, const bool finished) {
	if (finished)
		return;
	U64 pieces = board.pieces | (((U64)takenKingPos) << 32);
	U32 occupied = board.pieces | takenKingPos;
	//std::cout << "dead" << ((board.pieces >> INDEX_KINGS[1]) & 7) << std::endl;
	placePieces<false>(gameCards, pieces, { occupied, occupied }, 0, takenKingPos - 1, 0, 23, 23 - myMaxPawns, 23 - myMaxPawns - otherMaxPawns, myMaxPawns, otherMaxPawns);
}



uint8_t TableBase::generate(const GameCards& gameCards, const U32 men) {
	return generate(gameCards, men, (men + 1) / 2);
}
// generates the tables for all the boards where player 0 wins
uint8_t TableBase::generate(const GameCards& gameCards, const U32 men, const U32 menPerSide) {

	maxMen = men;
	maxMenPerSide = std::min<U32>(std::min(menPerSide, men - 1), 5);

	currDepth = 0;
	queue.clear();
	wonEvenBoards.resize(TABLESIZE);
	wonOddBoards.resize(TABLESIZE);
	pendingBoards.resize(TABLESIZE);

	auto beginTime = std::chrono::steady_clock::now();
	auto beginTime2 = beginTime;

	for (myMaxPawns = 0; myMaxPawns <= maxMenPerSide - 1; myMaxPawns++)
		for (otherMaxPawns = 0; otherMaxPawns <= std::min(maxMenPerSide - 1, maxMen - myMaxPawns - 2); otherMaxPawns++) {
			{ // all temple wins
				U32 king = MASK_END_POSITIONS[0];
				Board board{ king | MASK_TURN };
				for (int i = 0; i < 30; i++) {
					board.reverseMoves<*placePiecesTemple>(gameCards, maxMen, 0);
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
						board.reverseMoves<*placePiecesDead>(gameCards, maxMen, 0);
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
		printf("%9llu winning depth %2u boards in %6.2fs\n", queue.size(), currDepth + 1, (float)time / 1000000);
		wonCount += queue.size();
		lookups = 0;
		beginTime2 = std::chrono::steady_clock::now();
	} while (singleDepth(gameCards));
	
	auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
	//std::cout << wonb (float)time / 1000 << "ms" << std::endl;
	printf("%9llu winning boards in %.3fs (%.0fk/s)\n", wonCount, (float)time / 1000000, (float)wonCount * 1000 / time);

	wonOddBoards.clear();
	wonOddBoards.shrink_to_fit();
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

	// std::ofstream f("0269c.bin", std::ios::binary | std::ios::out);
	// // bxz::ofstream f("0269c.bin", bxz::lzma, 9);
	// // f.write(reinterpret_cast<char*>(evenWins.data()), evenWins.size()*sizeof(BoardValue));
	// f.write(reinterpret_cast<char*>(wonEvenBoards.data()), wonEvenBoards.size()*sizeof(uint8_t));
	// f.close();

	return currDepth;
}

U64 decompress6Men(U32 boardComp);
U32 compress6Men(U64 pieces) {
	U32 boardComp = 0;
	U32 bluePieces = pieces & MASK_PIECES;
	U32 redPieces = (pieces >> 32) & MASK_PIECES;
	const U32 king = _pdep_u32(1 << ((pieces >> INDEX_KINGS[0]) & 7), bluePieces);
	const U32 otherKing = _pdep_u32(1 << ((pieces >> INDEX_KINGS[1]) & 7), redPieces);
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
	boardComp = boardComp * 30 + ((pieces & MASK_CARDS) >> INDEX_CARDS);

	//U64 decomp = decompress6Men(boardComp);
	//assert(decomp == pieces);

	return boardComp;
}

U64 decompress6Men(U32 boardComp) { // still bugged lol
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

	return pieces;
}


TableBase::BoardValue::BoardValue(const std::pair<Board, uint8_t>& pair) : DTW(pair.second), boardComp(compress6Men(pair.first.pieces)) { }
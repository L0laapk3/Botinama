
#include "TableBase.h"

#include <bitset>
#include <chrono>
#include <vector>
#include <fstream>

#include "BitScan.h"
//#include "bxzstr/bxzstr.hpp"



std::vector<Board> queue{};
TableBase::MapType pendingBoards{};
TableBase::MapType TableBase::wonOddBoards{};
TableBase::MapType TableBase::wonEvenBoards{};
uint16_t currDepth;

uint8_t storeDepth() {
	return currDepth;
	return ~((uint8_t)0) - (currDepth >> 3);
}

U64 lookups = 0;
template<bool isMine>
void TableBase::addToTables(const GameCards& gameCards, const Board& board, const bool finished) {
	// this function assumed that it is called in the correct distanceToWin order
	// also assumes that there are no duplicate starting boards
	//board.print(gameCards, finished, true);

	lookups++;

	if (finished)
		return;

	//board.print(gameCards);
	bool exploreChildren = false;
	if (isMine) {
		// you played this move, immediately add it to won boards
		// only insert and iterate if it doesnt already exist (keeps lowest distance)
		const auto& result = wonOddBoards.emplace(board, storeDepth());
		exploreChildren = result.second;
	} else {
		// opponents move. All forward moves must lead to a loss first
		// this function should only get called at most countForwardMoves times
		//assert(wonBoards.end() == wonBoards.find(board));

		const auto it = pendingBoards.find(board);
		if (it == pendingBoards.end()) {
			pendingBoards.insert({ board, board.countForwardMoves(gameCards) - 1 });
		} else {
			if (--(it->second) == 0) {
				wonEvenBoards.insert({ board, storeDepth() }); // use last call with highest distance
				//pendingBoards.erase(it);
				exploreChildren = true;
			}
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
		//}
	}
};


U32 maxMen;
U32 maxMenPerSide;
std::vector<Board> currQueue{};
bool TableBase::singleDepth(const GameCards& gameCards) {
	currDepth++;
	std::swap(queue, currQueue);
	for (const Board& board : currQueue) {
		if (currDepth % 2 == 0)
			board.reverseMoves<*addToTables<true>>(gameCards, maxMen, maxMenPerSide);
		else
			board.reverseMoves<*addToTables<false>>(gameCards, maxMen, maxMenPerSide);
	}
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

	queue.clear();
	pendingBoards.clear();
	// pendingBoards.set_empty_key(Board{ ~0ULL });
	// pendingBoards.set_deleted_key(Board{ 0 });
	// wonOddBoards.set_empty_key(Board{ ~0ULL });
	// wonEvenBoards.set_empty_key(Board{ ~0ULL });
	// wonBoards.empty();
	currDepth = 0;
    queue.reserve(1E7); 
	pendingBoards.reserve(1E8);
    wonOddBoards.reserve(2E7); 
    wonEvenBoards.reserve(2E7); 

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
	do {
		const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime2).count());
		printf("%9llu winning depth %2u boards (%9llu pending, in %5.2fs, %9llu lookups, %4.1fM/s)\n", queue.size(), currDepth + 1, pendingBoards.size(), (float)time / 1000000, lookups, (float)lookups / time);
		lookups = 0;
		beginTime2 = std::chrono::steady_clock::now();
	} while (singleDepth(gameCards));
	
	auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
	//std::cout << wonb (float)time / 1000 << "ms" << std::endl;
	const U64 wonCount = wonOddBoards.size() + wonEvenBoards.size();
	printf("%9llu winning boards in %.3fs (%.0fk/s)\n", wonCount, (float)time / 1000000, (float)wonCount * 1000 / time);

	pendingBoards.clear();
	pendingBoards.shrink_to_fit();
	queue.shrink_to_fit();
	wonOddBoards.clear();
	wonOddBoards.shrink_to_fit();

	beginTime = std::chrono::steady_clock::now();

	std::vector<BoardValue> evenWins(wonEvenBoards.begin(), wonEvenBoards.end());
	wonEvenBoards.clear();
	wonEvenBoards.shrink_to_fit();
	std::sort(evenWins.begin(), evenWins.end(), [](const auto& a, const auto& b) { return a.boardComp < b.boardComp; });
	//U32 last = 0;
	//for (auto& d : evenWins) {
	//	U32 tmp = d.boardComp;
	//	d.boardComp -= last;
	//	last = tmp;
	//}
	std::vector<uint8_t> boardArray = { };
	boardArray.resize(((U64)25)*25*26*26/2*26*26/2*30);

	for (auto& d : evenWins) {
		boardArray[d.boardComp] = ~d.DTW;
	}
	
	time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
	printf("sorted %9llu boards in %.3fs\n", evenWins.size(), (float)time / 1000000);

	std::ofstream f("0269c.bin", std::ios::binary | std::ios::out);
	// bxz::ofstream f("0269c.bin", bxz::lzma, 9);
	// f.write(reinterpret_cast<char*>(evenWins.data()), evenWins.size()*sizeof(BoardValue));
	f.write(reinterpret_cast<char*>(boardArray.data()), boardArray.size()*sizeof(uint8_t));
	f.close();

	return currDepth;
}


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

	unsigned long piece1I = 26, otherPiece1I = 26;
	_BitScanForward(&piece1I, bluePieces);
	_BitScanForward(&otherPiece1I, redPieces);
	bluePieces &= ~(1 << piece1I);
	redPieces &= ~(1 << otherPiece1I);
	
	unsigned long piece2I = 26, otherPiece2I = 26;
	_BitScanForward(&piece2I, bluePieces);
	_BitScanForward(&otherPiece2I, redPieces);
	const U32 pieceValue = piece1I * 26 + piece2I;
	const U32 otherPieceValue = otherPiece1I * 26 + otherPiece2I;
	boardComp = boardComp * 26 * 13 + std::min(pieceValue, 26*26-1 - pieceValue);
	boardComp = boardComp * 26 * 13 + std::min(otherPieceValue, 26*26-1 - otherPieceValue);

	//boardComp = boardComp * 2  + ((pieces & MASK_TURN) >> INDEX_TURN);
	boardComp = boardComp * 30 + ((pieces & MASK_CARDS) >> INDEX_CARDS);

	return boardComp;
}


TableBase::BoardValue::BoardValue(const std::pair<Board, uint8_t>& pair) : DTW(pair.second), boardComp(compress6Men(pair.first.pieces)) { }
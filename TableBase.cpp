
#include "TableBase.h"

#include <bitset>
#include <chrono>



std::deque<Board> queue{};
google::dense_hash_map<Board, uint8_t, BoardHash> pendingBoards{};
google::dense_hash_map<Board, uint8_t, BoardHash> TableBase::wonBoards{};
uint16_t currDepth;

uint16_t storeDepth() {
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
		const auto& result = wonBoards.emplace(board, storeDepth());
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
				wonBoards.insert({ board, storeDepth() }); // use last call with highest distance
				//pendingBoards.erase(it);
				exploreChildren = true;
			}
		}
	}
	if (exploreChildren) {
		queue.push_back(board);
		//board.searchTime(gameCards, 1000, 0, currDepth + 1);
	}
};


std::array<uint8_t, 2> maxPieces;
bool TableBase::singleDepth(const GameCards& gameCards) {
	currDepth++;
	U32 stop = queue.size();
	for (int i = 0; i < stop; i++) {
		const Board& board = queue.front();
		//std::cout << std::endl << std::endl << "NEW ONE" << std::endl << std::endl;
		//board.print(gameCards);
		if (currDepth % 2 == 0)
			board.reverseMoves<*addToTables<true>>(gameCards, maxPieces);
		else
			board.reverseMoves<*addToTables<false>>(gameCards, maxPieces);
		queue.pop_front();
	}
	return queue.size();
}



template<bool templeWin>
void TableBase::placePieces(const GameCards& gameCards, U64 pieces, std::array<U32, 2> occupied, U32 beforeKing, U32 beforeOtherKing, U32 startAt, U32 spotsLeft, U32 minSpots0, U32 minSpotsAll, U32 myMaxPawns, U32 otherMaxPawns) {
	if (spotsLeft == minSpotsAll) {
		Board board{ pieces };
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
			for (int kingI = 0; kingI < myMaxPawns + 1; kingI++) {
				U32 kingPos = _pdep_u32(1 << kingI, board.pieces);
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

uint8_t myMaxPawns;
uint8_t otherMaxPawns;
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
	placePieces<false>(gameCards, pieces, { occupied, occupied }, 0, takenKingPos - 1, 0, 23, 23 - myMaxPawns, 23 - myMaxPawns - otherMaxPawns, myMaxPawns, otherMaxPawns);
}

// generates the tables for all the boards where player 0 wins
uint8_t TableBase::generate(const GameCards& gameCards, std::array<U32, 2> maxPawns) {

	maxPieces = { (uint8_t)(maxPawns[0] + 1), (uint8_t)(maxPawns[1] + 1) };
	queue.empty();
	pendingBoards.set_empty_key(Board{ ~0ULL });
	//pendingBoards.set_deleted_key(Board{ 0 });
	pendingBoards.empty();
	wonBoards.set_empty_key(Board{ ~0ULL });
	// wonBoards.empty();
	currDepth = 0;

	const auto beginTime = std::chrono::steady_clock::now();
	auto beginTime2 = beginTime;

	for (myMaxPawns = 0; myMaxPawns <= maxPawns[0]; myMaxPawns++)
		for (otherMaxPawns = 0; otherMaxPawns <= maxPawns[1]; otherMaxPawns++) {
			{ // all temple wins
				U32 king = MASK_END_POSITIONS[0];
				Board board{ king | MASK_TURN };
				for (int i = 0; i < 30; i++) {
					board.reverseMoves<*placePiecesTemple>(gameCards, { maxPieces[0], 0 });
					board.pieces += 1ULL << INDEX_CARDS;
				}
			}

			{ // all king kill wins
				takenKingPos = 1ULL;
				for (U32 pos = 0; pos < 24; pos++) {
					takenKingPos <<= takenKingPos == MASK_END_POSITIONS[1];
					Board board{ takenKingPos | MASK_TURN | (7ULL << INDEX_KINGS[0]) };
					for (int i = 0; i < 30; i++) {
						board.reverseMoves<*placePiecesDead>(gameCards, { maxPieces[0], 0 });
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
	
	const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
	//std::cout << wonb (float)time / 1000 << "ms" << std::endl;
	printf("%9llu winning boards in %.3fs (%.0fk/s)", wonBoards.size(), (float)time / 1000000, (float)wonBoards.size() * 1000 / time);

	for (int depth = 1; depth < currDepth; depth++) {
		U64 count = 0;
		for (auto const& [board, distance] : wonBoards)
			if (distance == depth) {
				count++;
				/*if (depth > 1)
					board.searchTime(gameCards, 1000, 0, depth);*/
				//if (depth == 70)
				//	board.print(gameCards);
			}
		//printf("%7llu boards are win in %2u\n", count, depth);
	}
	return currDepth;
}

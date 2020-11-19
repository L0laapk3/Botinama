
#include "TableBase.h"

#include <bitset>



std::array<std::deque<Board>, TableBase::MAXDEPTH> todoBoards = {};
std::map<Board, uint8_t> TableBase::pendingBoards{};
std::map<Board, uint8_t> TableBase::wonBoards{};




void TableBase::addToTables(const GameCards& gameCards, const Board& board, const bool finished, uint8_t distanceToWin) {
	// this function assumed that it is called in the correct distanceToWin order
	// also assumes that there are no duplicate starting boards
	//board.print(gameCards, finished);

	if (finished)
		return;
	bool isMine = distanceToWin % 2 == 1;

	bool exploreChildren = false;
	if (isMine) {
		// you played this move, immediately add it to won boards
		// only insert and iterate if it doesnt already exist (keeps lowest distance)
		const auto& result = wonBoards.emplace(board, distanceToWin);
		exploreChildren = result.second;
	} else {
		// opponents move. All forward moves must lead to a loss first
		// this function should only get called at most countForwardMoves times
		assert(wonBoards.end() == wonBoards.find(board));

		const auto it = pendingBoards.find(board);
		if (it == pendingBoards.end()) {
			pendingBoards[board] = board.countForwardMoves(gameCards) - 1;
		}
		else {
			if (--(it->second) == 0) {
				wonBoards.insert({ board, distanceToWin }); // use last call with highest distance
				pendingBoards.erase(it);
				exploreChildren = true;
			}
		}
	}
	if (exploreChildren && (distanceToWin < MAXDEPTH))
		todoBoards[distanceToWin].push_back(board);
};


void TableBase::loopDepth(const GameCards& gameCards, const int depth) {
	U32 startCount = wonBoards.size();
	auto& queue = todoBoards[depth];
	int i = 0;
	while (queue.size()) {
		if (i++ > 100000) {
			std::cout << depth << '\t' << queue.size() << '\t' << pendingBoards.size() << '\t' << wonBoards.size() << std::endl;
			i = 0;
		}
		const Board board = queue.back();
		queue.pop_back();
		board.reverseMoves<*addToTables>(gameCards, depth + 1);
	}
}



template<bool templeWin>
void TableBase::placePieces(const GameCards& gameCards, U64 pieces, U32 occupied, U32 beforeKing, U32 beforeOtherKing, U32 startAt, U32 spotsLeft, U32 minSpots0, U32 minSpotsAll, U32 myMaxPawns, U32 otherMaxPawns) {
	if (spotsLeft == minSpotsAll) {
		Board board{ pieces };
		board.pieces |= _popcnt64((((U64)beforeOtherKing) << 32) & board.pieces) << INDEX_KINGS[1];
		if (templeWin) {
			board.pieces |= _popcnt64(beforeKing & board.pieces) << INDEX_KINGS[0];
			for (int i = 0; i < 30; i++) {
				todoBoards[0].push_back(board);
				//loopUntilExhausted(gameCards);
				board.pieces += 1ULL << INDEX_CARDS;
			}
		} else {
			for (int kingI = 0; kingI < myMaxPawns + 1; kingI++) {
				//board.print(gameCards);
				addToTables(gameCards, board, false, 1);
				//loopUntilExhausted(gameCards);
				board.pieces += 1ULL << INDEX_KINGS[0];
			}
		}
		return;
	}
	bool player = spotsLeft <= minSpots0;
	U64 piece = 1ULL << startAt;
	while (piece & MASK_PIECES) {
		startAt++;
		if (piece & ~occupied)
			placePieces<templeWin>(gameCards, pieces | (((U64)piece) << (player ? 32 : 0)), occupied | piece, beforeKing, beforeOtherKing, spotsLeft == minSpots0 ? 0 : startAt, spotsLeft - 1, minSpots0, minSpotsAll, myMaxPawns, otherMaxPawns);
		piece <<= 1;
	}
}

U32 myMaxPawns;
U32 otherMaxPawns;
U32 takenKingPos;
void TableBase::placePiecesKingTake(const GameCards& gameCards, const Board& board, const bool finished, uint8_t _) {
	if (finished)
		return;
	U64 pieces = board.pieces | (((U64)takenKingPos) << 32);
	placePieces<false>(gameCards, pieces, board.pieces | takenKingPos, 0, takenKingPos - 1, 0, 23, 23 - myMaxPawns, 23 - myMaxPawns - otherMaxPawns, myMaxPawns, otherMaxPawns);
}

// generates the tables for all the boards where player 0 wins
void TableBase::generate(const GameCards& gameCards, std::array<U32, 2> maxPawns) {
	for (myMaxPawns = 0; myMaxPawns <= maxPawns[0]; myMaxPawns++)
		for (otherMaxPawns = 0; otherMaxPawns <= maxPawns[1]; otherMaxPawns++) {
			// all temple wins
			U32 king = MASK_END_POSITIONS[0];
			U32 otherKing = 1ULL;
			for (U32 otherKingPos = 0; otherKingPos < 23; otherKingPos++) {
				otherKing <<= (otherKing == king) || (otherKing == MASK_END_POSITIONS[1]);
				U64 pieces = king | (((U64)otherKing) << 32) | MASK_TURN;
				placePieces<true>(gameCards, pieces, king | otherKing, king - 1, otherKing - 1, 0, 23, 23 - myMaxPawns, 23 - myMaxPawns - otherMaxPawns, myMaxPawns, otherMaxPawns);
				otherKing <<= 1;
			}
		}
	loopDepth(gameCards, 0); // push everything to depth 2

	for (myMaxPawns = 0; myMaxPawns <= maxPawns[0]; myMaxPawns++)
		for (otherMaxPawns = 0; otherMaxPawns <= maxPawns[1]; otherMaxPawns++) {
			// all king kill wins
			takenKingPos = 1ULL;
			for (U32 pos = 0; pos < 24; pos++) {
				takenKingPos <<= takenKingPos == MASK_END_POSITIONS[1];
				Board board{ takenKingPos | MASK_TURN };
				for (int i = 0; i < 30; i++) {
					board.reverseMoves<*placePiecesKingTake>(gameCards, 0);
					board.pieces += 1ULL << INDEX_CARDS;
				}
				board.pieces &= ~(0x1FULL << INDEX_CARDS);
				takenKingPos <<= 1;
			}
		}
	for (int depth = 1; depth < MAXDEPTH; depth++)
		loopDepth(gameCards, depth);

	for (int depth = 1; depth < MAXDEPTH; depth++) {
		U64 count = 0;
		for (auto const& [board, distance] : wonBoards)
			if (distance == depth) {
				count++;
				board.searchTime(gameCards, 1000);
			}
		printf("%4llu boards are win in %2u\n", count, depth);
	}
}


#include <iostream>
#include <iomanip>
#include <string>
#include <bitset>
#include <chrono>
#include <algorithm>

#include "Board.h"
#include "Card.h"
#include "CardBoard.h"
#include "TableBase.h"
#include "Connection.h"


//U64 count = 0;
//void perft(const GameCards& gameCards, const Board& board, const bool finished, U32 depth) {
//	if (!depth || finished)
//		count++;
//	else
//		board.forwardMoves<*perft>(gameCards, depth - 1);
//}
//void perftCheat(const GameCards& gameCards, const Board& board, const bool finished, U32 depth) {
//	if (finished)
//		count++;
//	else if (depth == 1)
//		count += board.countForwardMoves(gameCards);
//	else
//		board.forwardMoves<*perftCheat>(gameCards, depth - 1);
//}
//void bench(const GameCards& gameCards, Board board, U32 depth) {
//	auto start = std::chrono::steady_clock::now();
//	count = 0;
//	perft(gameCards, board, false, depth);
//	auto end = std::chrono::steady_clock::now();
//	float nps = std::roundf(count / (std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() * .001f));
//	std::cout << depth << "  " << nps << "M/s \t" << count << std::endl;
//}
//void time(const GameCards& gameCards, Board board, U32 depth) {
//	auto start = std::chrono::steady_clock::now();
//	//perftCheat(gameCards, board, false, depth);
//	auto result = board.search(gameCards, depth);
//	auto end = std::chrono::steady_clock::now();
//	std::cout << depth << "\t" << "in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms  \t" << result.score << "\t" << std::bitset<64>(result.board.pieces) << std::endl;
//	result.board.print(gameCards);
//}



int main(int argc, char** argv) {

	//GameCards cards = CardBoard::fetchGameCards({ "crab", "ox", "horse", "boar", "elephant" });
	//Board board = Board::fromString("4000000000000000000020000", true);
	//board.print(cards);
	//std::cout << (U32)board.findImmediateWins(cards) << std::endl;
	//return 0;


	// PERFT CARDS - 0269C
	GameCards cards = CardBoard::fetchGameCards({ "boar", "ox", "elephant", "horse", "crab" });

	//GameCards cards = CardBoard::fetchGameCards({ "ox", "boar", "horse", "crane", "eel" }, false);


	//GameCards cards = CardBoard::fetchGameCards({ "crane", "ox", "horse", "eel", "boar" });
	// Board board = Board::fromString("0000020000001040000003000", false, false);
	// board.print(cards);
	// // board.searchTime(cards, 60*1000, 2);

	// std::cout << (U32)TableBase::wonEvenBoards[board] << std::endl;
	// std::cout << (U32)TableBase::wonOddBoards[board] << std::endl;

	TableBase::generate(cards, 5);
	return 0;
	// std::cout << board.eval(cards) << std::endl;


	//U32 mostBoards = 36000;
	//for (int i1 = 0; i1 < 16; i1++)
	//for (int i2 = i1+1; i2 < 16; i2++)
	//for (int i3 = i2 + 1; i3 < 16; i3++)
	//for (int i4 = i3 + 1; i4 < 16; i4++)
	//for (int i5 = i4 + 1; i5 < 16; i5++) {
	//	GameCards cards = GameCards{ { {&CARDS[i1]}, {&CARDS[i2]}, {&CARDS[i3]}, {&CARDS[i4]}, {&CARDS[i5]} } };
	//	U32 maxDepth = TableBase::generate(cards, { 0, 0 });
	//	U32 numBoards = TableBase::wonBoards.size();
	//	if (mostBoards > numBoards) {
	//		mostBoards = numBoards;
	//		std::cout << numBoards << ' ' << cards[0].name << ' ' << cards[1].name << ' ' << cards[2].name << ' ' << cards[3].name << ' ' << cards[4].name << std::endl;
	//	}
	//}

	// TableBase::generate(cards, { 0, 0 });
	// TableBase::generate(cards, { 0, 1 });
	// TableBase::generate(cards, { 1, 1 });
	//return 0;

	auto conn = Connection();
	if (argc > 1)
		conn.joinGame(argv[1]);
	else
		conn.createGame();
	Game game = conn.waitGame();

	while (true) {
		// game.board.print(game.cards);
		// std::cout << game.board.eval(game.cards) << std::endl;
		if (!game.board.currentPlayer()) {
			auto bestMove = game.board.searchTime(game.cards, 1000);
			conn.submitMove(game, bestMove.board);
		}

		conn.waitTurn(game);
		if (conn.ended) {
			std::cout << (game.board.winner() ? "lost" : "won") << std::endl;
			break;
		}
	}

	//Board board = Board::fromString("1020101010000000303030403", true);
	//Board board = Board::fromString("0031000100342101000300300", true);
	//board.print(gameCards);
	//BitBoard::generate(gameCards, { 1, 1 });

	//for (int depth = 1; depth < 20; depth++)
	//	time(gameCards, board, depth);
}
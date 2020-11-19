
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

	GameCards cards = CardBoard::fetchGameCards({ "crane", "eel", "horse", "ox", "boar" });
	// Board board = Board::fromString("0000020000000000000000040", false);
	// GameCards cards = CardBoard::fetchGameCards({ "crane", "ox", "horse", "eel", "boar" });
	// Board board = Board::fromString("0000000000000020000000004", false);
	//searchTime(cards, board, 1000 * 60, true);
	// std::cout << board.eval(cards) << std::endl;

	TableBase::generate(cards, { 0, 0 });
	std::cout << TableBase::wonBoards.size() << std::endl;

	return 0;

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
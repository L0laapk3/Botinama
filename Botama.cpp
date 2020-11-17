
#include <iostream>
#include <string>
#include <bitset>
#include <chrono>
#include <algorithm>

#include "Board.h"
#include "Card.h"
#include "CardBoard.h"
#include "BitBoard.h"
#include "Connection.h"


U64 count = 0;
void perft(const GameCards& gameCards, const Board& board, const bool finished, U32 depth) {
	if (!depth || finished)
		count++;
	else
		board.forwardMoves<*perft>(gameCards, depth - 1);
}
void perftCheat(const GameCards& gameCards, const Board& board, const bool finished, U32 depth) {
	if (finished)
		count++;
	else if (depth == 1)
		count += board.countForwardMoves(gameCards);
	else
		board.forwardMoves<*perftCheat>(gameCards, depth - 1);
}
void bench(const GameCards& gameCards, Board board, U32 depth) {
	auto start = std::chrono::steady_clock::now();
	count = 0;
	perft(gameCards, board, false, depth);
	auto end = std::chrono::steady_clock::now();
	float nps = std::roundf(count / (std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() * .001f));
	std::cout << depth << "  " << nps << "M/s \t" << count << std::endl;
}
void time(const GameCards& gameCards, Board board, U32 depth) {
	auto start = std::chrono::steady_clock::now();
	//perftCheat(gameCards, board, false, depth);
	auto result = board.search(gameCards, depth);
	auto end = std::chrono::steady_clock::now();
	std::cout << depth << "\t" << "in " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms  \t" << result.score << "\t" << std::bitset<64>(result.board.pieces) << std::endl;
	result.board.print(gameCards);
}


SearchResult searchTime(const Game& game, const U64 timeBudget) {
	auto lastTime = 1ULL;
	auto predictedTime = 1ULL;
	U32 depth = 0;
	SearchResult result;
	const auto beginTime = std::chrono::steady_clock::now();
	while ((depth < 2 || predictedTime < timeBudget * 1000) && depth < 63) {
		result = game.board.search(game.cards, ++depth);
		++depth;
		const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
		predictedTime = time * time / lastTime;
		lastTime = time;
		bool end = std::abs(result.score) > (1 << 29);
		
			std::cout << "depth " << depth << " \t" << "in " << time / 1000 << "ms  \t";
			if (!end)
				std::cout << "score: " << result.score << std::endl;
			else if (result.score > 0)
				std::cout << "win" << std::endl;
			else
				std::cout << "lose" << std::endl;
		
	}
	return result;
}

int main() {
	auto conn = Connection();
	Game game = conn.waitGame();

	while (true) {
		//game.board.print(game.cards);
		if (!game.board.currentPlayer()) {
			auto bestMove = searchTime(game, 1000);
			conn.submitMove(game, bestMove.board);
			std::cout << std::endl;
		}

		conn.waitTurn(game);
	}

	//Board board = Board::fromString("1020101010000000303030403", true);
	//Board board = Board::fromString("0031000100342101000300300", true);
	//board.print(gameCards);
	//BitBoard::generate(gameCards, { 1, 1 });

	//for (int depth = 1; depth < 20; depth++)
	//	time(gameCards, board, depth);
}
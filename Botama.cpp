
#include <iostream>
#include <iomanip>
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


constexpr U32 startDepth = 0;
constexpr U32 minDepth = 5;
SearchResult searchTime(const GameCards& cards, const Board& board, const U64 timeBudget) {
	auto lastTime = 1ULL;
	auto predictedTime = 1ULL;
	S32 depth = startDepth;
	S32 shortestEnd = std::numeric_limits<S32>::max();
	SearchResult result;
	while (true) {
	const auto beginTime = std::chrono::steady_clock::now();
		result = board.search(cards, ++depth);
		const auto time = std::max(1ULL, (unsigned long long)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - beginTime).count());
		predictedTime = time * time / lastTime;
		lastTime = time;
		bool foundWin = std::abs(result.score) >= SCORE_WIN;
		bool foundProbableWin = std::abs(result.score) > SCORE_WIN - 64;
		bool lastIteration = ((predictedTime > timeBudget * 1000) && (depth >= minDepth)) || (depth >= 64);
		if (lastIteration || foundWin/* || time > 50000*/) {
			printf("depth %2i in %.2fs (%2lluM/s): ", depth, (float)time / 1E6, result.total / time);
			if (!foundProbableWin)
				printf("%.2f\n", (float)result.score / SCORE_PIECE);
			else {
				S32 end = depth - (std::abs(result.score) - SCORE_WIN);
				bool quiescentFind = end > depth;
				if (!quiescentFind)
					shortestEnd = std::min(end, shortestEnd);
				std::cout << (result.score > 0 ? "win" : "lose") << " in " << end << (quiescentFind ? "?" : "") << std::endl;
			}
			if (lastIteration || shortestEnd <= depth)
				break;
		}
		
	}
	return result;
}

int main() {

	// GameCards cards = CardBoard::fetchGameCards({ "boar", "horse", "elephant", "ox", "crab" });
	// Board board = Board::fromString("0000000002000004000000000", false);
	// board.print(cards);
	// searchTime(cards, board, 1000);
	// std::cout << board.eval(cards) << std::endl;
	// std::cout << std::endl << cards[0].name << std::endl;
	// cards[0].print();
	// SearchResult result = board.search<true>(cards, 1);
	// std::cout << 1 - (result.score - SCORE_WIN) << std::endl;

	auto conn = Connection();
	Game game = conn.waitGame();

	while (true) {
		// game.board.print(game.cards);
		// std::cout << game.board.eval(game.cards) << std::endl;
		if (!game.board.currentPlayer()) {
			auto bestMove = searchTime(game.cards, game.board, 1000);
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
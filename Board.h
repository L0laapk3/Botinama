#pragma once

#include <string>
#include <array>
#include <list>
#include "CardBoard.h"
#include <span>
#include <functional>

constexpr uint32_t MASK_TURN = 1 << 25;
constexpr uint32_t MASK_FINISH = 1 << 26;
constexpr uint32_t MASK_CARDS = 0x1f << 27;

constexpr std::array<uint32_t, 2> END_POSITIONS = {
	0b00100 << 20,
	0b00100
};

template<bool player>
class Board;

template<bool player>
using MoveFunc = void (*)(GameCards& gameCards, const Board<player>& board, const bool finished, unsigned long long depth);


template<bool player>
class Board {
public:
	std::array<uint32_t, 2> pieces;
	uint32_t kings;

	static Board fromString(std::string str) {
		Board board{ { 0b00011 << 27, 0b01100 << 27 }, 0 };

		for (int i = 0; i < 25; i++) {
			if (str[i] == '1' || str[i] == '2')
				board.pieces[0] |= 1 << i;
			if (str[i] == '3' || str[i] == '4')
				board.pieces[1] |= 1 << i;
			if (str[i] == '2' || str[i] == '4')
				board.kings |= 1 << i;
		}
		if (false) // red starts
			board.pieces[0] |= MASK_TURN;
		return board;
	}

	void print(GameCards& gameCards, std::vector<Board> board);
	void print(GameCards& gameCards) const {
		//Board<player>::print(gameCards, { *this });
	};

	void valid(GameCards& gameCards) const;
	bool finished() const;
	bool winner() const;

private:
	template<MoveFunc<!player> cb>
	void iterateMoves(GameCards& gameCards, const CardBoard& card, uint32_t playerPieces, bool movingPlayer, unsigned long long depth) const {
		//card.print();
		unsigned long from;
		uint32_t bitScan = pieces[movingPlayer] & 0x1ffffff;
		while (_BitScanForward(&from, bitScan)) {
			bitScan &= ~(1 << from);
			std::array<uint32_t, 2> boardsWithoutPiece{ pieces };
			boardsWithoutPiece[movingPlayer] = playerPieces & ~(1 << from);
			boardsWithoutPiece[0] = (boardsWithoutPiece[0] & ~MASK_TURN) | ((!player) << 25);
			uint32_t kingsWithoutPiece = kings & ~(1 << from);
			uint32_t isMovingKing = kingsWithoutPiece == kings ? 0 : ~0;
			uint32_t scan = card.moveBoard[player][from] & ~pieces[movingPlayer] & 0x1fffffff;
			uint32_t endMask = (END_POSITIONS[movingPlayer] & isMovingKing) | kings; // to be &'d with nextBit. kind lands on temple | piece takes king
			while (scan) {
				uint32_t nextBit = scan & -scan;
				scan &= ~nextBit;
				Board<!player> board;
				board.pieces[movingPlayer] = boardsWithoutPiece[movingPlayer] | nextBit;
				board.pieces[!movingPlayer] = boardsWithoutPiece[!movingPlayer] & ~nextBit;
				board.kings = kingsWithoutPiece | (nextBit & isMovingKing);
				const bool finished = nextBit & endMask;
				cb(gameCards, board, finished, depth);
			}
		}
	}
public:
	template<MoveFunc<!player> cb>
	void forwardMoves(GameCards& gameCards, unsigned long long depth) const {
		uint32_t cardScan = pieces[player] & MASK_CARDS;
		uint32_t playerPiecesWithNew = pieces[player] | (MASK_CARDS & ~pieces[!player]);
		for (int i = 0; i < 2; i++) {
			unsigned long cardI;
			_BitScanForward(&cardI, cardScan);
			cardScan &= ~(1ULL << cardI);
			uint32_t playerPieces = playerPiecesWithNew & ~(1ULL << cardI);
			const auto& card = gameCards[cardI - 27ULL];
			iterateMoves<cb>(gameCards, card, playerPieces, player, depth);
		}
	}
	//void reverseMoves(GameCards& gameCards, MoveFunc cb, int depth) const;
};

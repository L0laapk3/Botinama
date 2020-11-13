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

class Board;
typedef void (*MoveFunc)(GameCards& gameCards, const Board& board, const bool finished, unsigned long long depth);

class Board {
public:
	std::array<uint32_t, 2> pieces;
	uint32_t kings;

	static Board fromString(std::string str);

	void print(GameCards& gameCards) const;
	static void print(GameCards& gameCards, std::vector<Board> board);

	void valid(GameCards& gameCards) const;
	bool finished() const;
	bool winner() const;

private:
	template<MoveFunc cb>
	void iterateMoves(GameCards& gameCards, const CardBoard& card, uint32_t playerPieces, bool player, bool movingPlayer, unsigned long long depth) const {
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
				Board board;
				board.pieces[movingPlayer] = boardsWithoutPiece[movingPlayer] | nextBit;
				board.pieces[!movingPlayer] = boardsWithoutPiece[!movingPlayer] & ~nextBit;
				board.kings = kingsWithoutPiece | (nextBit & isMovingKing);
				const bool finished = nextBit & endMask;
				cb(gameCards, board, finished, depth);
			}
		}
	}
public:
	template<MoveFunc cb>
	void forwardMoves(GameCards& gameCards, unsigned long long depth) const {
		bool player = pieces[0] & MASK_TURN;
		uint32_t cardScan = pieces[player] & MASK_CARDS;
		uint32_t playerPiecesWithNew = pieces[player] | (MASK_CARDS & ~pieces[!player]);
		for (int i = 0; i < 2; i++) {
			unsigned long cardI;
			_BitScanForward(&cardI, cardScan);
			cardScan &= ~(1ULL << cardI);
			uint32_t playerPieces = playerPiecesWithNew & ~(1ULL << cardI);
			const auto& card = gameCards[cardI - 27];
			iterateMoves<cb>(gameCards, card, playerPieces, player, player, depth);
		}
	}
	/*
	template<MoveFunc cb>
	void reverseMoves(GameCards& gameCards, unsigned long long depth) const {
		bool player = pieces[0] & MASK_TURN;
		unsigned long swapCardI;
		_BitScanForward(&swapCardI, cards & CARDS_SWAPMASK);
		uint32_t cardScan = cards & CARDS_PLAYERMASK[!player];
		unsigned long playerCardI;
		_BitScanForward(&playerCardI, cardScan);
		uint32_t swapCardPlayerMask = (cards & CARDS_SWAPMASK) >> (!player ? 8 : 16);
		uint32_t firstCard = 1ULL << playerCardI;
		uint32_t secondCard = cards & CARDS_PLAYERMASK[!player] & ~firstCard;
		uint32_t newCards = (cards & 0xffffULL) | swapCardPlayerMask;
		iterateMoves(gameCards[swapCardI & 7], (newCards & ~firstCard) | (firstCard << (!player ? 8 : 16)), player, !player, cb);
		iterateMoves(gameCards[swapCardI & 7], (newCards & ~secondCard) | (secondCard << (!player ? 8 : 16)), player, !player, cb);
	}*/

};


struct Moves {
	unsigned long size = 0;
	std::array<Board, 8 * 5> outputs;
};
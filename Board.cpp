
#include "Board.h"

#include <bitset>
#include <iostream>
#include <intrin.h>

#pragma intrinsic(_BitScanForward)


constexpr std::array<uint32_t, 2> CARDS_PLAYERMASK{ 0xffULL, 0xff00ULL };
constexpr uint32_t CARDS_SWAPMASK = 0xff0000ULL;


Board::Board(std::string str) : pieces{ 0 }, kings{ 0 } {
	for (int i = 0; i < 25; i++) {
		if (str[i] == '1' || str[i] == '2')
			pieces[0] |= 1 << i;
		if (str[i] == '3' || str[i] == '4')
			pieces[1] |= 1 << i;
		if (str[i] == '2' || str[i] == '4')
			kings |= 1 << i;
	}

	unsigned long i = 4;
	std::cout << (int)_BitScanForward(&i, kings) << " ";
	std::cout << i << "\n";
}


Board::Board(std::array<uint32_t, 2> pieces, uint32_t kings, bool player, uint32_t cards) : pieces{ pieces }, kings{ kings }, player{ player }, cards{ cards } { }


void Board::iterateMoves(const CardBoard& card, uint32_t newCards, bool movingPlayer, MoveFunc& cb) {
	uint32_t playerPieces = pieces[movingPlayer];
	unsigned long from;
	while (_BitScanForward(&from, playerPieces)) {
		playerPieces &= ~(1ULL << from);
		std::array<uint32_t, 2> boardsWithoutPiece{ pieces };
		pieces[movingPlayer] &= ~(1ULL << from);
		uint32_t kingsWithoutPiece = kings & ~(1ULL << from);
		uint32_t newKingMask = kingsWithoutPiece == kings ? ~0ULL : 0ULL;
		unsigned long to;
		uint32_t scan = card.moveBoard[player][from] & ~pieces[movingPlayer];
		while (_BitScanForward(&to, scan)) {
			scan &= ~(1ULL << to);
			std::array<uint32_t, 2> newBoards{ boardsWithoutPiece };
			newBoards[movingPlayer] &= ~(1ULL << from);
			cb(Board(newBoards, newKingMask | ((1ULL << to) & newKingMask), !player, newCards));
		}
	}
}

void Board::forwardMoves(std::array<CardBoard, 5>& gameCards, MoveFunc& cb) {
	uint32_t cardScan = cards & CARDS_PLAYERMASK[player];
	for (int i = 0; i < 2; i++) {
		unsigned long cardI;
		_BitScanForward(&cardI, cardScan);
		cardScan &= ~(1ULL << cardI);
		uint32_t newCards = cards & ~(1ULL << cardI) | ((cards & CARDS_SWAPMASK) >> (player ? 8 : 16));
		const auto& card = gameCards[player ? cardI >> 8 + 2 : cardI];
		iterateMoves(card, newCards, player, cb);
	}
}

void Board::reverseMoves(std::array<CardBoard, 5>& gameCards, MoveFunc& cb) {
	unsigned long swapCardI;
	_BitScanForward(&swapCardI, cards & CARDS_SWAPMASK);
	uint32_t cardScan = cards & CARDS_PLAYERMASK[!player];
	unsigned long playerCardI;
	_BitScanForward(&playerCardI, cardScan);
	uint32_t swapCardPlayerMask = (cards & CARDS_SWAPMASK) >> (!player ? 8 : 16);
	uint32_t firstCard = 1ULL << (playerCardI);
	uint32_t secondCard = cards & CARDS_PLAYERMASK[!player] & ~firstCard;
	uint32_t newCards = cards & 0xffffULL | swapCardPlayerMask;
	iterateMoves(gameCards[swapCardI >> 16 + 4], newCards & ~firstCard | (firstCard << (!player ? 8 : 16)), !player, cb);
	iterateMoves(gameCards[swapCardI >> 16 + 4], newCards & ~secondCard | (secondCard << (!player ? 8 : 16)), !player, cb);
}
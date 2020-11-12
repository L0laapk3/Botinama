#include "Card.h"

#include <iostream>


Card::Card(std::string name, bb moves) : name(name), moves(moves) { }

void Card::print() const {
	Card::print({ *this });
}
void Card::print(std::vector<Card> cards) {
	for (int r = 5; r-- > 0;) {
		for (const auto& card : cards) {
			for (int c = 0; c < 5; c++) {
				const int mask = 1 << (5 * r + c);
				if (r == 2 && c == 2)
					std::cout << 'x';
				else
					std::cout << ((card.moves & mask) ? 'o' : ' ');
			}
			std::cout << ' ';
		}
		std::cout << std::endl;
	}
}
void Card::print(std::vector<bb> moves) {
	for (int r = 5; r-- > 0;) {
		for (const auto& move : moves) {
			for (int c = 0; c < 5; c++) {
				const int mask = 1 << (5 * r + c);
				std::cout << ((move & mask) ? 'o' : '-');
			}
			std::cout << ' ';
		}
		std::cout << std::endl;
	}
}

const Card* Card::findCard(const std::string& name) {
	return &CARDS[0] + (std::find_if(CARDS.begin(), CARDS.end(), [&](const Card& card) { return card.name == name; }) - CARDS.begin());
}

std::array<const Card, 16> CARDS = {
	Card("rabbit", 0b0000001000100000001000000ULL),
	Card("monkey", 0b0000001010000000101000000ULL),
	Card("boar", 0b0000000100010100000000000ULL),
	Card("goose", 0b0000000010010100100000000ULL),
	Card("cobra", 0b0000001000000100100000000ULL),
	Card("crab", 0b0000000100100010000000000ULL),
	Card("horse", 0b0000000100000100010000000ULL),
	Card("dragon", 0b0000010001000000101000000ULL),
	Card("rooster", 0b0000001000010100001000000ULL),
	Card("crane", 0b0000000100000000101000000ULL),
	Card("elephant", 0b0000001010010100000000000ULL),
	Card("mantis", 0b0000001010000000010000000ULL),
	Card("tiger", 0b0010000000000000010000000ULL),
	Card("frog", 0b0000000010000010100000000ULL),
	Card("ox", 0b0000000100010000010000000ULL),
	Card("eel", 0b0000000010010000001000000ULL),
};
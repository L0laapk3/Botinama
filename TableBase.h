#pragma once
#include <array>
#include "Botama.h"
#include "CardBoard.h"


namespace TableBase {
	void generate(GameCards& gameCards, std::array<U32, 2> maxPawns = { 0, 0 });
}
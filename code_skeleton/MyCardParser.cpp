#include "MyCardParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace sevens {


// TODO: For example, create a standard 52-card deck. Or read from an input file.
void MyCardParser::read_cards() {
    
    cards_hashmap.clear();

    std::cout << "[MyCardParser::read_cards] Generating the 52 cards standard deck\n";

    uint64_t card_id = 0;

    for (uint64_t suit = 0; suit < 4; ++suit) {
        for (uint64_t rank = 1; rank <= 13; ++rank) {

            // cards_hashmap is a map of <int, struct> that is defined in Generic_game_parser

            cards_hashmap[card_id++] = Card{
                static_cast<int>(suit),
                static_cast<int>(rank)  // static_cast for compatibility
            };
        }
    }
}

} // namespace sevens

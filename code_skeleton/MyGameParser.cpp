#include "MyGameParser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

namespace sevens {


// TODO: e.g., set table_layout[suit][rank] for the start of Sevens
void MyGameParser::read_game() {

    table_layout.clear();

    std::cout << "[MyGameParser::read_game] // We start with the 7♦ on the table (suit 2 = diamonds)\n";

    table_layout[2][7] = true;

}

void sevens::MyGameParser::read_cards() {
    // dummy override, because Generic_game_parser inherits from Generic_card_parser so the method must be defined
}


} // namespace sevens

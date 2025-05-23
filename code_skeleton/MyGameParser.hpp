#pragma once

#include "Generic_game_parser.hpp"

namespace sevens {

/**
 * Derived from Generic_game_parser.
 * Subclasses must fill in read_game(...) to initialize the table layout.
 */
class MyGameParser : public Generic_game_parser {
public:
    MyGameParser() = default;
    ~MyGameParser() = default;

    void read_cards() override; // dummy override, because Generic_game_parser inherits from Generic_card_parser so the method must be defined
    void read_game() override;

};

} // namespace sevens

#pragma once

#include "Generic_card_parser.hpp"
#include <unordered_map>
#include <string>

namespace sevens {

/**
 * Extends Generic_card_parser to handle game-state data for Sevens:
 *   - table_layout[suit][rank] = true if that rank is on the table.
 * Subclasses must override read_game(...) to set up the initial table.
 */
class Generic_game_parser : public Generic_card_parser {
public:

    // initialize the table method
    virtual void read_game() = 0;

    // Provide read-only access to the table layout
    const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& get_table_layout() const {
        return this->table_layout;
    }

protected:
    // For each suit -> rank -> bool (true if on table)
    std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>> table_layout;
};

} // namespace sevens

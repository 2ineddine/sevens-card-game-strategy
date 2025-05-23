#pragma once

#include "Generic_game_mapper.hpp"
#include "PlayerStrategy.hpp"
#include "Generic_card_parser.hpp"
#include "Generic_game_parser.hpp"
#include "MyCardParser.hpp"
#include "MyGameParser.hpp"

#include <random>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

namespace sevens {

/**
 * Enhanced Sevens simulation with strategy support:
 *  - Possibly internal mode or competition mode
 */
class MyGameMapper : public Generic_game_mapper {
public:
    MyGameMapper();
    ~MyGameMapper() = default;

    std::vector<std::pair<uint64_t, uint64_t>>
    compute_game_progress(uint64_t numPlayers) override;

    std::vector<std::pair<uint64_t, uint64_t>>
    compute_and_display_game(uint64_t numPlayers) override;
    
    std::vector<std::pair<std::string, uint64_t>>
    compute_game_progress(const std::vector<std::string>& playerNames) override;
    
    std::vector<std::pair<std::string, uint64_t>>
    compute_and_display_game(const std::vector<std::string>& playerNames) override;

    // New method for playing multiple rounds until a player reaches 50 points
    std::vector<std::pair<uint64_t, uint64_t>>
    compute_multiple_rounds_to_score(uint64_t numPlayers, uint64_t maxScore);

    // Required by Generic_card_parser and Generic_game_parser
    void read_cards() override;
    void read_game() override;
    
    // Strategy management
    
    void registerStrategy(uint64_t playerID, std::shared_ptr<PlayerStrategy> strategy) override;
    bool hasRegisteredStrategies() const override;

private:
    // data structures needed to track the game

    // random number generator
    std::mt19937 rng;

    // List of all cards
    std::unordered_map<uint64_t, Card> cards_hashmap;

    // Table Layout
    std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>> table_layout;

    // Players strategies
    std::unordered_map<uint64_t, std::shared_ptr<PlayerStrategy>> strategies;
};

} // namespace sevens
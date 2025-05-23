#pragma once

#include "Generic_card_parser.hpp"
#include <vector>
#include <memory>

namespace sevens {

/**
 * Interface for player strategy implementations.
 * Students will implement this interface to create their competitive agents.
 */
class PlayerStrategy {
public:
    virtual ~PlayerStrategy() = default;
    
    // Initialize the strategy with player ID and any other setup
    virtual void initialize(uint64_t playerID) = 0;
    
    // Select a card to play from the player's hand
    // Returns index of the card in hand to play, or -1 if no playable card
    virtual int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) = 0;
        
    // Called to inform the strategy about other players' moves
    virtual void observeMove(uint64_t playerID, const Card& playedCard) = 0;
    
    // Called when a player passes their turn
    virtual void observePass(uint64_t playerID) = 0;
    
    // Get a name for this strategy (for display purposes)
    virtual std::string getName() const = 0;
};

// Type for strategy factory functions (for dynamic loading)
typedef PlayerStrategy* (*CreateStrategyFn)();

} // namespace sevens

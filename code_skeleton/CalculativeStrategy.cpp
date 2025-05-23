#include "PlayerStrategy.hpp"
#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <unordered_map>
#include <cmath>
#include <random>
#include <chrono>
#include <iostream>

namespace sevens {

class CalculativeStrategy : public PlayerStrategy {
public:
    CalculativeStrategy() {
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }

    ~CalculativeStrategy() override = default;

    void initialize(uint64_t playerID) override {
        myID = playerID;
        
        // Initialize data structures for tracking
        playerHands.clear();
        playerPasses.clear();
        playedCards.clear();
        
        // Track suits that players seem to have or lack
        playerSuitStrengths.clear();
        playerSuitWeaknesses.clear();
    }

    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        // Update our tracked hand
        myHand = hand;
        
        // Track what suits we have
        std::map<int, int> mySuitCounts;
        for (const auto& card : hand) {
            mySuitCounts[card.suit]++;
        }
        
        // Get all playable cards and their indices
        std::vector<std::pair<int, Card>> playableCards;
        for (size_t i = 0; i < hand.size(); ++i) {
            if (isPlayable(hand[i], tableLayout)) {
                playableCards.emplace_back(static_cast<int>(i), hand[i]);
            }
        }
        
        if (playableCards.empty()) {
            return -1; // No playable cards, must pass
        }
        
        // SCORING SYSTEM FOR EACH PLAYABLE CARD
        std::vector<std::pair<double, int>> scoredMoves; // score, index
        
        for (const auto& [idx, card] : playableCards) {
            double score = calculateMoveScore(card, hand, tableLayout);
            scoredMoves.emplace_back(score, idx);
        }
        
        // Sort by descending score
        std::sort(scoredMoves.begin(), scoredMoves.end(), 
                 [](const auto& a, const auto& b) { return a.first > b.first; });
        
        // Add some randomness if there are multiple high-scoring moves
        // but within top 20% of scores to avoid being predictable
        if (scoredMoves.size() > 1) {
            double topScore = scoredMoves[0].first;
            std::vector<int> topIndices;
            
            for (const auto& [score, idx] : scoredMoves) {
                // Consider moves within 20% of the top score
                if (score >= topScore * 0.8) {
                    topIndices.push_back(idx);
                } else {
                    break;
                }
            }
            
            // If we have multiple good choices, add a bit of randomness
            if (topIndices.size() > 1) {
                std::uniform_int_distribution<int> dist(0, static_cast<int>(topIndices.size()) - 1);
                return topIndices[dist(rng)];
            }
        }
        
        // Return the highest-scoring move
        return scoredMoves[0].second;
    }

    void observeMove(uint64_t playerID, const Card& playedCard) override {
        if (playerID == myID) return; // We already know our own moves
        
        // Track that this card has been played
        playedCards.emplace_back(playedCard);
        
        // Player revealed they have this suit
        playerSuitStrengths[playerID].insert(playedCard.suit);
        
        // Update our model of each player's hand
        auto& playerHand = playerHands[playerID];
        
        // Remove the played card if we thought they had it
        auto it = std::find_if(playerHand.begin(), playerHand.end(),
                            [&playedCard](const Card& c) {
                                return c.suit == playedCard.suit && c.rank == playedCard.rank;
                            });
        if (it != playerHand.end()) {
            playerHand.erase(it);
        }
        
        // Reset pass count since they played a card
        playerPasses[playerID] = 0;
    }

    void observePass(uint64_t playerID) override {
        if (playerID == myID) return; // We already know our own passes
        
        // Increment pass count for this player
        playerPasses[playerID]++;
        
        // After multiple passes, infer which suits they might be weak in
        // by analyzing what cards could have been played but weren't
        if (playerPasses[playerID] >= 2) {
            inferPlayerWeaknesses(playerID);
        }
    }

    std::string getName() const override {
        return "CalculativeStrategy";
    }

private:
    uint64_t myID;
    std::mt19937 rng;
    std::vector<Card> myHand;
    
    // Tracking structures
    std::unordered_map<uint64_t, std::vector<Card>> playerHands;
    std::unordered_map<uint64_t, int> playerPasses;
    std::vector<Card> playedCards;
    
    // Track which suits each player seems to have or lack
    std::unordered_map<uint64_t, std::set<int>> playerSuitStrengths;
    std::unordered_map<uint64_t, std::set<int>> playerSuitWeaknesses;
    
    // Helper function to check if a card is playable
    bool isPlayable(const Card& card, 
                   const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const {
        // Check if it's a 7
        if (card.rank == 7) {
            // If the 7 is not on the table, it can be played
            return tableLayout.count(card.suit) == 0 || 
                   tableLayout.at(card.suit).count(7) == 0 || 
                   !tableLayout.at(card.suit).at(7);
        }
        
        // Check for cards adjacent to this one
        bool hasLower = false;
        if (card.rank > 1 && tableLayout.count(card.suit) > 0) {
            hasLower = tableLayout.at(card.suit).count(card.rank - 1) > 0 && 
                      tableLayout.at(card.suit).at(card.rank - 1);
        }
        
        bool hasUpper = false;
        if (card.rank < 13 && tableLayout.count(card.suit) > 0) {
            hasUpper = tableLayout.at(card.suit).count(card.rank + 1) > 0 && 
                      tableLayout.at(card.suit).at(card.rank + 1);
        }
        
        return hasLower || hasUpper;
    }
    
    // Calculate card play score - higher is better
    double calculateMoveScore(const Card& card, 
                             const std::vector<Card>& hand,
                             const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) {
        double score = 0.0;
        
        // PRIORITY 1: Play higher value cards (10-King) first when possible
        if (card.rank >= 10) {
            score += 30 + (card.rank - 9); // 31-34 points for 10-K
        }
        // PRIORITY 2: Play Ace when possible (also high value)
        else if (card.rank == 1) {
            score += 30; // 30 points for Ace
        }
        
        // PRIORITY 3: Play cards that unlock opportunities for more plays
        // Check if playing this card will enable us to play more cards
        int unlockedCards = countCardsUnlockedByPlaying(card, hand, tableLayout);
        score += unlockedCards * 20; // Very high bonus for unlocking our own cards
        
        // PRIORITY 4: Consider suit strategy
        int suitCount = countCardsOfSuit(card.suit, hand);
        
        // Try to get rid of suits with few cards
        if (suitCount <= 2) {
            score += 15; // Good to eliminate suits
        }
        // Or focus on suits where we have many cards (7 or more)
        else if (suitCount >= 7) {
            score += 10; // Also good to specialize in a suit
        }
        
        // PRIORITY 5: Block opponents if they seem to specialize in a suit
        bool isSuitStrengthForOpponent = false;
        for (const auto& [playerID, strengths] : playerSuitStrengths) {
            if (playerID != myID && strengths.count(card.suit) > 0) {
                isSuitStrengthForOpponent = true;
                break;
            }
        }
        
        if (isSuitStrengthForOpponent) {
            // This is a key suit for an opponent - check if playing this would
            // create a gap that blocks them
            bool createsGap = wouldCreateBlockingGap(card, tableLayout);
            if (createsGap) {
                score += 25; // Very high bonus for blocking opponents
            }
        }
        
        // PRIORITY 6: Play 7s early if we have them
        if (card.rank == 7) {
            score += 5; // Modest bonus for playing 7s (they're always playable)
        }
        
        // PRIORITY 7: Slight preference for middle ranks (6-8) over extreme ranks
        // This helps keep options open
        int distanceFromMiddle = std::abs(7 - card.rank);
        score -= distanceFromMiddle * 0.5; // Small penalty for extreme ranks
        
        return score;
    }
    
    // Count how many of our cards would become playable after playing this card
    int countCardsUnlockedByPlaying(const Card& card, 
                                   const std::vector<Card>& hand,
                                   const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>> tableLayout) {
        int count = 0;
        
        // Create a copy of the table layout with this card added
        auto newTableLayout = tableLayout;
        newTableLayout[card.suit][card.rank] = true;
        
        // Check which cards would become playable that weren't before
        for (const auto& potentialCard : hand) {
            // Skip the card we're playing
            if (potentialCard.suit == card.suit && potentialCard.rank == card.rank) {
                continue;
            }
            
            // If it wasn't playable before but would be after, count it
            if (!isPlayable(potentialCard, tableLayout) && 
                 isPlayable(potentialCard, newTableLayout)) {
                count++;
            }
        }
        
        return count;
    }
    
    // Count cards of a specific suit in hand
    int countCardsOfSuit(int suit, const std::vector<Card>& hand) {
        return std::count_if(hand.begin(), hand.end(), 
                           [suit](const Card& c) { return c.suit == suit; });
    }
    
    // Check if playing a card would create a gap that blocks opponents
    bool wouldCreateBlockingGap(const Card& card, 
                               const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) {
        // The blocking happens when we create a discontinuity like: 5 6 8 9
        // Where the 7 is missing and blocks progress
        
        // Check for potential blocking gaps
        if (card.rank <= 5) { // Playing lower card - check for gaps above it
            bool hasRankPlus1 = tableLayout.count(card.suit) > 0 && 
                              tableLayout.at(card.suit).count(card.rank + 1) > 0 && 
                              tableLayout.at(card.suit).at(card.rank + 1);
                              
            bool hasRankPlus2 = card.rank <= 11 && tableLayout.count(card.suit) > 0 && 
                               tableLayout.at(card.suit).count(card.rank + 2) > 0 && 
                               tableLayout.at(card.suit).at(card.rank + 2);
                               
            // This would create a gap like: card, card+2 (missing card+1)
            return !hasRankPlus1 && hasRankPlus2;
        }
        else if (card.rank >= 9) { // Playing higher card - check for gaps below it
            bool hasRankMinus1 = tableLayout.count(card.suit) > 0 && 
                               tableLayout.at(card.suit).count(card.rank - 1) > 0 && 
                               tableLayout.at(card.suit).at(card.rank - 1);
                               
            bool hasRankMinus2 = card.rank >= 3 && tableLayout.count(card.suit) > 0 && 
                                tableLayout.at(card.suit).count(card.rank - 2) > 0 && 
                                tableLayout.at(card.suit).at(card.rank - 2);
                                
            // This would create a gap like: card-2, card (missing card-1)
            return !hasRankMinus1 && hasRankMinus2;
        }
        
        return false;
    }
    
    // Infer which suits a player might be weak in based on passes
    void inferPlayerWeaknesses(uint64_t playerID) {
        // Look at what cards could have been played based on table state
        // but weren't played by this player who passed multiple times
        
        auto& weaknesses = playerSuitWeaknesses[playerID];
        auto& strengths = playerSuitStrengths[playerID];
        
        // If we definitely know they have a suit (they played it before)
        // but they're passing while that suit has playable cards,
        // they might be out of that suit or missing specific ranks
        
        // For now, just mark suits they've never played as potential weaknesses
        for (int suit = 0; suit < 4; ++suit) {
            if (strengths.count(suit) == 0) {
                weaknesses.insert(suit);
            }
        }
    }
};

} // namespace sevens

// Export function for the loader — DO NOT place in the namespace
extern "C" sevens::PlayerStrategy* createStrategy() {
    return new sevens::CalculativeStrategy();
}
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

class Sentinel7 : public PlayerStrategy {
public:
    Sentinel7() {
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }

    ~Sentinel7() override = default;

    void initialize(uint64_t playerID) override {
        myID = playerID;
        
        // Initialize data structures for tracking
        playerHands.clear();
        playerPasses.clear();
        playedCards.clear();
        
        // Track suits that players seem to have or lack
        playerSuitStrengths.clear();
        playerSuitWeaknesses.clear();
        
        // Track critical cards (7s, 8s, 6s)
        playerHasCriticalCards.clear();
        
        // Track the number of cards each player has
        playerCardCounts.clear();
        
        // Reset game progression tracking
        gameProgress = 0;
        cardsPlayedPerSuit.clear();
        
        // Initialize estimated hand sizes
        for (uint64_t pid = 0; pid < 8; ++pid) { // assuming max 8 players
            if (pid != myID) {
                playerCardCounts[pid] = 13; // initial estimate (may vary by player count)
            }
        }
    }

    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        // Update our tracked hand
        myHand = hand;
        
        // Update game progress (0-100%)
        updateGameProgress(tableLayout);
        
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
            double score = calculateMoveScore(card, hand, tableLayout, mySuitCounts);
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
        
        // Track special cards (7s, 6s, and 8s)
        if (playedCard.rank == 7 || playedCard.rank == 6 || playedCard.rank == 8) {
            playerHasCriticalCards[playerID][playedCard.suit][playedCard.rank] = false; // They no longer have this card
        }
        
        // Update estimated card count for the player
        if (playerCardCounts.count(playerID) > 0) {
            playerCardCounts[playerID]--;
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
        return "Sentinel7";
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
    
    // Track players who have critical cards (7s, 6s, 8s)
    std::unordered_map<uint64_t, std::unordered_map<int, std::unordered_map<int, bool>>> playerHasCriticalCards;
    
    // Track estimated number of cards each player has
    std::unordered_map<uint64_t, int> playerCardCounts;
    
    // Game progression (0-100%)
    int gameProgress;
    
    // Cards played per suit (to avoid recounting)
    std::unordered_map<int, int> cardsPlayedPerSuit;
    
    // Update game progression based on cards played
    void updateGameProgress(const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) {
        int playedCardCount = 0;
        cardsPlayedPerSuit.clear();
        
        for (const auto& [suit, ranks] : tableLayout) {
            cardsPlayedPerSuit[suit] = ranks.size();
            playedCardCount += ranks.size();
        }
        
        // Estimate game progress (0-100%)
        gameProgress = std::min(100, static_cast<int>(playedCardCount * 100.0 / 52.0));
    }
    
    // Helper function to check if a card has an adjacent card on the table
    bool hasAdjacent(const Card& card, 
                    const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) const {
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
        return hasAdjacent(card, tableLayout);
    }
    
    // Calculate card play score - higher is better
    double calculateMoveScore(const Card& card, 
                             const std::vector<Card>& hand,
                             const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout,
                             const std::map<int, int>& mySuitCounts) {
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
        int suitCount = mySuitCounts.at(card.suit);
        
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
        
        // NEW PRIORITY: Hold on to critical cards (7s, 6s, 8s) as long as possible
        // But adjust based on game state and opponents' card counts
        if (card.rank == 7 || card.rank == 6 || card.rank == 8) {
            // Only hold onto critical cards if we have alternatives and it's not end game
            std::vector<std::pair<int, Card>> playableCards;
            for (size_t i = 0; i < hand.size(); ++i) {
                if (isPlayable(hand[i], tableLayout)) {
                    playableCards.emplace_back(static_cast<int>(i), hand[i]);
                }
            }
            
            bool hasAlternatives = playableCards.size() > 1;
            
            // Check if any opponent is close to winning (has few cards)
            bool opponentIsCloseToWinning = false;
            for (const auto& [playerID, cardCount] : playerCardCounts) {
                if (playerID != myID && cardCount <= 3) {
                    opponentIsCloseToWinning = true;
                    break;
                }
            }
            
            // In early game, hold critical cards
            if (gameProgress < 50 && hasAlternatives) {
                score -= 50; // Big penalty for playing critical cards too early
            }
            // In mid game, hold critical cards if they could block opponents
            else if (gameProgress < 75 && hasAlternatives && opponentIsCloseToWinning) {
                score -= 40;
            }
            // In late game, prioritize getting rid of all cards
            else if (gameProgress >= 75) {
                // If we have few cards left, prioritize playing anything
                if (hand.size() <= 5) {
                    score += 20; // Bonus for getting rid of any card in end game
                }
                // If it's the diamond 7 and we have it early, play it to start the game
                if (card.rank == 7 && card.suit == 1 && gameProgress == 0) {
                    score += 100; // Always play diamond 7 to start
                }
            }
        }
        
        // NEW PRIORITY: Adapt strategy based on hand size and game progress
        // If we have few cards left, prioritize getting rid of any card
        if (hand.size() <= 5) {
            score += 15; // General bonus for playing any card when hand is small
        }
        
        // NEW PRIORITY: Play cards that create runs we can follow up on
        int potentialRun = calculatePotentialRun(card, hand, tableLayout);
        if (potentialRun >= 2) {
            score += potentialRun * 8; // Bonus for potential to play a run next turn
        }
        
        // PRIORITY 6: Play 7s early if we have them (adjusted for diamond 7)
        if (card.rank == 7) {
            // Diamond 7 starts the game
            if (card.suit == 1 && gameProgress == 0) {
                score += 100; // Always play diamond 7 first if we have it
            }
            // Other 7s strategy
            else {
                score += 5; // Modest bonus for playing 7s (they're always playable)
            }
        }
        
        // PRIORITY 7: Slight preference for middle ranks (6-8) over extreme ranks
        // This helps keep options open
        int distanceFromMiddle = std::abs(7 - card.rank);
        score -= distanceFromMiddle * 0.5; // Small penalty for extreme ranks
        
        return score;
    }
    
    // Calculate how many cards in a potential run we can play
    int calculatePotentialRun(const Card& card, 
                           const std::vector<Card>& hand,
                           const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) {
        // Simulate playing this card
        auto newTableLayout = tableLayout;
        newTableLayout[card.suit][card.rank] = true;
        
        // Check if we have cards that would form a run after playing this one
        int runSize = 1; // Start with the card we're playing
        
        // Look for ascending sequence (current rank + 1, +2, etc.)
        runSize += runLength(card.suit, card.rank, 1, hand, newTableLayout);
        
        // Look for descending sequence (current rank - 1, -2, etc.)
        runSize += runLength(card.suit, card.rank, -1, hand, newTableLayout);
        
        return runSize;
    }
    
    // Helper method to calculate run length in a specific direction
    int runLength(int suit, int startRank, int direction, 
                 const std::vector<Card>& hand,
                 std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>> layout) {
        int length = 0;
        int rank = startRank + direction;
        
        // Define bounds based on direction
        int maxRank = (direction > 0) ? 14 : 0;
        
        while (rank != maxRank) {
            bool haveCard = false;
            for (const auto& handCard : hand) {
                if (handCard.suit == suit && handCard.rank == rank) {
                    // Check if this would be playable after we play our card
                    if (isPlayable(handCard, layout)) {
                        haveCard = true;
                        break;
                    }
                }
            }
            if (haveCard) {
                length++;
                // Update simulated table for next iteration
                layout[suit][rank] = true;
                rank += direction;
            } else {
                break;
            }
        }
        
        return length;
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
    return new sevens::Sentinel7();
}
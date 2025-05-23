#include "PlayerStrategy.hpp"
#include <algorithm>
#include <vector>
#include <string>
#include <random>
#include <chrono>

namespace sevens {

class RandomAgressiveStrategy : public PlayerStrategy {
public:
    RandomAgressiveStrategy() {
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }

    ~RandomAgressiveStrategy() override = default;

    void initialize(uint64_t playerID) override {
        myID = playerID;
    }

    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        std::vector<int> playable_indices;

        for (size_t i = 0; i < hand.size(); ++i) {
            const Card& card = hand[i];
            uint64_t suit = card.suit;
            uint64_t rank = card.rank;

            // Check if it's a 7 - special case that can always be played if not already on table
            bool is_seven = (rank == 7);
            if (is_seven) {
                // Check if the 7 is already on the table
                // Safely check if suit exists in tableLayout
                if (tableLayout.find(suit) == tableLayout.end() || 
                    tableLayout.at(suit).find(7) == tableLayout.at(suit).end() || 
                    !tableLayout.at(suit).at(7)) {
                    playable_indices.push_back(static_cast<int>(i));
                    continue;
                }
            }

            // For non-7 cards, check if there's an adjacent card on the table
            bool has_lower = false; 
            bool has_upper = false;
            
            // Safely check for lower adjacent card
            if (rank > 1 && tableLayout.find(suit) != tableLayout.end()) {
                auto& suit_map = tableLayout.at(suit);
                has_lower = (suit_map.find(rank - 1) != suit_map.end() && suit_map.at(rank - 1));
            }
            
            // Safely check for upper adjacent card
            if (rank < 13 && tableLayout.find(suit) != tableLayout.end()) {
                auto& suit_map = tableLayout.at(suit);
                has_upper = (suit_map.find(rank + 1) != suit_map.end() && suit_map.at(rank + 1));
            }
            
            // Card is playable if it has an adjacent card already on the table
            if (has_lower || has_upper) {
                playable_indices.push_back(static_cast<int>(i));
            }
        }

        if (playable_indices.empty()) {
            return -1; // No playable card → pass
        }

        std::uniform_int_distribution<int> dist(0, static_cast<int>(playable_indices.size()) - 1);
        return playable_indices[dist(rng)];
    }

    void observeMove(uint64_t playerID, const Card& playedCard) override {
        // For a more advanced strategy, you can store played cards here
        (void)playerID;
        (void)playedCard;
    }

    void observePass(uint64_t playerID) override {
        // You can keep track of players who passed
        (void)playerID;
    }

    std::string getName() const override {
        return "RandomAgressiveStrategy";
    }

private:
    uint64_t myID;
    std::mt19937 rng;
};

} // namespace sevens

// Export function for the loader — DO NOT place in the namespace
extern "C" sevens::PlayerStrategy* createStrategy() {
    return new sevens::RandomAgressiveStrategy();
}
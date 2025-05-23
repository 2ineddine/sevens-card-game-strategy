#include "PlayerStrategy.hpp"
#include <algorithm>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <random>
#include <chrono>

namespace sevens {

// Custom hash function for std::pair<uint64_t, uint64_t>
struct PairHash {
    std::size_t operator()(const std::pair<uint64_t, uint64_t>& p) const {
        return std::hash<uint64_t>()(p.first) ^ (std::hash<uint64_t>()(p.second) << 1);
    }
};

class PrudentStrategy : public PlayerStrategy {
public:
    PrudentStrategy() {
        auto seed = static_cast<unsigned long>(
            std::chrono::system_clock::now().time_since_epoch().count()
        );
        rng.seed(seed);
    }

    ~PrudentStrategy() override = default;

    void initialize(uint64_t playerID) override {
        myID = playerID;
        playedCards.clear();
        playerPassCount.clear();
    }

    int selectCardToPlay(
        const std::vector<Card>& hand,
        const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& tableLayout) override 
    {
        std::vector<std::pair<int, int>> scoredChoices; // (index in hand, score)

        for (size_t i = 0; i < hand.size(); ++i) {
            const Card& card = hand[i];
            uint64_t suit = card.suit;
            uint64_t rank = card.rank;
            bool isSeven = (rank == 7);

            // Check if 7 is already played
            bool sevenOnTable = tableLayout.count(suit) && tableLayout.at(suit).count(7) && tableLayout.at(suit).at(7);
            
            if (isSeven && !sevenOnTable) {
                int suitCount = countSuit(hand, suit);
                int score = (suitCount > 2 ? 10 : -10); // Only open a suit if we have enough cards in it
                scoredChoices.emplace_back(i, score);
                continue;
            }

            bool lower = (rank > 1 && tableLayout.count(suit) && tableLayout.at(suit).count(rank - 1) && tableLayout.at(suit).at(rank - 1));
            bool upper = (rank < 13 && tableLayout.count(suit) && tableLayout.at(suit).count(rank + 1) && tableLayout.at(suit).at(rank + 1));

            if (lower || upper) {
                int score = 0;

                // Avoid edge cards (A, 2, Q, K) unless necessary
                if (rank == 1 || rank == 13 || rank == 2 || rank == 12)
                    score -= 5;
                else
                    score += 2;

                // Bonus if this card keeps both lower and upper branches open
                if (lower && upper)
                    score += 2;

                // Prefer playing cards from suits with more cards in hand
                score += countSuit(hand, suit);

                scoredChoices.emplace_back(i, score);
            }
        }

        if (scoredChoices.empty()) {
            return -1; // No valid play
        }

        // Choose the best scoring card
        std::sort(scoredChoices.begin(), scoredChoices.end(),
                  [](auto& a, auto& b) { return a.second > b.second; });

        return scoredChoices.front().first;
    }

    void observeMove(uint64_t playerID, const Card& playedCard) override {
        (void)playerID;
        playedCards.insert({playedCard.suit, playedCard.rank});
    }

    void observePass(uint64_t playerID) override {
        playerPassCount[playerID]++;
    }

    std::string getName() const override {
        return "PrudentStrategy";
    }

private:
    uint64_t myID;
    std::mt19937 rng;

    // Data tracking
    std::unordered_set<std::pair<uint64_t, uint64_t>, PairHash> playedCards;
    std::unordered_map<uint64_t, int> playerPassCount;

    // Utility to count how many cards of a suit are in hand
    int countSuit(const std::vector<Card>& hand, uint64_t suit) {
        int count = 0;
        for (const Card& c : hand) {
            if (c.suit == suit) count++;
        }
        return count;
    }
};

} // namespace sevens

extern "C" sevens::PlayerStrategy* createStrategy() {
    return new sevens::PrudentStrategy();
}

#include "MyGameMapper.hpp"
#include "MyCardParser.hpp"
#include "MyGameParser.hpp"
#include <iostream>
#include <algorithm>
#include <random>
#include <chrono>
#include <iomanip>

namespace sevens {

/**
 * Card Suits Display Symbols
 * Index mapping: 0=Spades, 1=Hearts, 2=Diamonds, 3=Clubs
 */
static const char* SUIT_SYM[4] = {"♠","♥","♦","♣"};

/**
 * Utility function to print a single card
 * Handles special cases for face cards (A, J, Q, K)
 */
static void printCard(const Card& c) {
    std::string r = (c.rank == 1) ? "A" : 
                    (c.rank == 11) ? "J" :
                    (c.rank == 12) ? "Q" :
                    (c.rank == 13) ? "K" : std::to_string(c.rank);
    std::cout << r << SUIT_SYM[c.suit];
}

/**
 * Utility function to print a player's hand
 */
static void printHand(const std::vector<Card>& h, uint64_t id) {
    std::cout << "Player " << id << " : ";
    for(auto& c : h) { printCard(c); std::cout << ' '; }
    std::cout << '\n';
}

/**
 * Utility function to print all players' hands
 */
static void printAllHands(const std::vector<std::vector<Card>>& H) {
    std::cout << "\n--- Players' Hands ---\n";
    for(uint64_t p = 0; p < H.size(); ++p) printHand(H[p], p);
    std::cout << "----------------------\n";
}

/**
 * Utility function to print the current game table
 * Shows which cards have been played for each suit
 */
static void printTable(const std::unordered_map<uint64_t, 
                     std::unordered_map<uint64_t, bool>>& T)
{
    std::cout << "\n----- TABLE -----\n";
    for(uint64_t s = 0; s < 4; ++s) {
        std::cout << SUIT_SYM[s] << ' ';
        for(uint64_t r = 1; r <= 13; ++r) {
            bool on = T.count(s) && T.count(s) && T.at(s).count(r) && T.at(s).at(r);
            if(on) {
                std::string rs = (r == 1) ? "A" : 
                                (r == 11) ? "J" :
                                (r == 12) ? "Q" :
                                (r == 13) ? "K" : std::to_string(r);
                std::cout << rs << ' ';
            } else std::cout << ". ";
        }
        std::cout << '\n';
    }
    std::cout << "----------------\n";
}

// Constructor
MyGameMapper::MyGameMapper() { 
    table_layout.clear(); 
    strategies.clear(); 
    rng.seed(std::chrono::steady_clock::now().time_since_epoch().count());
}

/**
 * Load card definitions from file
 */
void MyGameMapper::read_cards() {
    MyCardParser p; 
    p.read_cards(); 
    cards_hashmap = p.get_cards_hashmap();
}

/**
 * Initialize game state - start with only 7 of diamonds on table
 */
void MyGameMapper::read_game() {
    MyGameParser g;
    g.read_game(); 
    table_layout = g.get_table_layout();
}

/**
 * Check if strategies have been registered
 */
bool MyGameMapper::hasRegisteredStrategies() const { 
    return !strategies.empty(); 
}

/**
 * Register a player strategy
 */
void MyGameMapper::registerStrategy(uint64_t id, std::shared_ptr<PlayerStrategy> s) {
    strategies[id] = std::move(s);
}

/**
 * Determine if a card is playable based on game rules
 * - 7s can be played if not already on table
 * - Other cards can be played if adjacent card of same suit is on table
 */
static bool isPlayable(const Card& c, 
                     const std::unordered_map<uint64_t, std::unordered_map<uint64_t, bool>>& table) {
    // 7s can be played if not already on table
    if(c.rank == 7) return !table.count(c.suit) || !table.at(c.suit).count(7) || !table.at(c.suit).at(7);
    
    // Other cards need adjacent card of same suit on table
    bool lower = c.rank > 1  && table.count(c.suit) && table.at(c.suit).count(c.rank-1) && table.at(c.suit).at(c.rank-1);
    bool upper = c.rank < 13 && table.count(c.suit) && table.at(c.suit).count(c.rank+1) && table.at(c.suit).at(c.rank+1);
    return lower || upper;
}









// TODO: implement a quiet simulation
std::vector<std::pair<uint64_t, uint64_t>>
MyGameMapper::compute_game_progress(uint64_t nP) {


    // Initialize Cards and Table
    read_cards(); 
    read_game(); 


    // Distribute Cards to Players
    // Prepare a shuffled deck (all 52 cards)
    std::vector<Card> deck;
    for (auto& [id, c] : cards_hashmap) deck.push_back(c);

    // shuffle deck
    std::shuffle(deck.begin(), deck.end(), rng);

    // Choose a random starting player
    int start_player = rng() % nP;

    // Deal cards to players starting from random player
    std::vector<std::vector<Card>> hands(nP);
    for (size_t i = 0; i < deck.size(); ++i) {
        int player = (start_player + i) % nP;
        hands[player].push_back(deck[i]);
    }

    // Search for and remove 7♦ (suit=2, rank=7)
    for (auto& hand : hands) {
        auto it = std::find_if(hand.begin(), hand.end(), [](const Card& c) {
            return c.suit == 2 && c.rank == 7;
        });
        if (it != hand.end()) {
            hand.erase(it);
            break; // Found and removed, done
        }
    }


    // Initialize player scores and strategies
    std::vector<uint64_t> scores(nP);
    for(uint64_t i = 0; i < nP; ++i) {
        if(strategies.count(i)) strategies[i]->initialize(i);
        scores[i] = hands[i].size();
    }

    // Main game loop
    uint64_t current_player = (start_player + 1) % nP;  // the loops start with the player after the first player who played the opening 7 of diamond
    bool game_over = false;
    std::vector<bool> passed(nP, false);

    while(!game_over) {

        // Load current player strategy and hand, and call the player's selectCardToPlay() method
        auto& strategy = strategies[current_player];
        auto& hand = hands[current_player];
        int selected_card_idx = strategy->selectCardToPlay(hand, table_layout);

        // Check if the player played a valid card
        bool played_successfully = false;
        if(selected_card_idx >= 0 && (size_t)selected_card_idx < hand.size() && isPlayable(hand[selected_card_idx], table_layout)) {
            
            // Place card on table
            Card played_card = hand[selected_card_idx];
            table_layout[played_card.suit][played_card.rank] = true;

            // Notify all players of the move
            for(auto& [id, s] : strategies) s->observeMove(current_player, played_card);

            // Remove card from hand
            hand.erase(hand.begin() + selected_card_idx);
            scores[current_player] = hand.size();
            
            // Check if player has emptied their hand (game ends)
            if(hand.empty()) {
                game_over = true;
            }
            
            passed[current_player] = false;
            played_successfully = true;
        }

        // Handle pass
        if(!played_successfully) { 
            strategy->observePass(current_player); 
            passed[current_player] = true; 
        }

        // Next player's turn
        current_player = (current_player + 1) % nP;

        // Check if all players have passed (game ends)
        if(std::all_of(passed.begin(), passed.end(), [](bool p) { return p; })) {
            game_over = true;
        }
    }

    // Calculate final rankings
    std::vector<std::pair<uint64_t, uint64_t>> scoreWithId;
    for(uint64_t i = 0; i < nP; ++i) {
        scoreWithId.emplace_back(i, scores[i]);
    }
    
    // Sort by score (ascending - fewer cards is better)
    std::sort(scoreWithId.begin(), scoreWithId.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
    
    return scoreWithId;
}










// TODO: implement a verbose simulation
std::vector<std::pair<uint64_t, uint64_t>>
MyGameMapper::compute_and_display_game(uint64_t nP) {


    // Initialize Cards and Table
    read_cards(); 
    read_game();


    // Distribute Cards to Players
    // Prepare a shuffled deck (all 52 cards)
    std::vector<Card> deck;
    for (auto& [id, c] : cards_hashmap) deck.push_back(c);

    // shuffle deck
    std::shuffle(deck.begin(), deck.end(), rng);

    // Choose a random starting player
    int start_player = rng() % nP;

    // Deal cards to players starting from random player
    std::vector<std::vector<Card>> hands(nP);
    for (size_t i = 0; i < deck.size(); ++i) {
        int player = (start_player + i) % nP;
        hands[player].push_back(deck[i]);
    }

    // Search for and remove 7♦ (suit=2, rank=7)
    for (auto& hand : hands) {
        auto it = std::find_if(hand.begin(), hand.end(), [](const Card& c) {
            return c.suit == 2 && c.rank == 7;
        });
        if (it != hand.end()) {
            hand.erase(it);
            break; // Found and removed, done
        }
    }


    // Initialize player scores and strategies
    std::vector<uint64_t> scores(nP);
    for(uint64_t i = 0; i < nP; ++i) {
        if(strategies.count(i)) strategies[i]->initialize(i);
        scores[i] = hands[i].size();
    }

    // Display initial game state
    std::cout << "\n7♦ is on the table at the start of the game.\n";
    printAllHands(hands);
    printTable(table_layout);

    // Main game loop
    uint64_t current_player = (start_player + 1) % nP;  // the loops start with the player after the first player who played the opening 7 of diamond
    bool game_over = false;
    std::vector<bool> passed(nP, false);

    while(!game_over) {
        // Display player turn
        std::cout << "\n\nPlayer " << current_player << "'s turn\n";
        printAllHands(hands);

        // Load current player strategy and hand, and call the player's selectCardToPlay() method
        auto& strategy = strategies[current_player];
        auto& hand = hands[current_player];
        int selected_card_idx = strategy->selectCardToPlay(hand, table_layout);

        // Check if the player played a valid card
        bool played_successfully = false;
        if(selected_card_idx >= 0 && (size_t)selected_card_idx < hand.size() && isPlayable(hand[selected_card_idx], table_layout)) {
            
            // Place card on table
            Card played_card = hand[selected_card_idx];
            table_layout[played_card.suit][played_card.rank] = true;

            // Display the move
            std::cout << "\nPlayer " << current_player << " plays "; 
            printCard(played_card); 
            std::cout << '\n';
            printTable(table_layout);

            // Notify all players of the move
            for(auto& [id, s] : strategies) s->observeMove(current_player, played_card);

            // Remove card from hand
            hand.erase(hand.begin() + selected_card_idx);
            scores[current_player] = hand.size();
            
            // Check if player has emptied their hand (game ends)
            if(hand.empty()) {
                game_over = true;
                std::cout << "\n\nPlayer " << current_player << " has emptied their hand! Game over.\n";
            }
            
            passed[current_player] = false;
            played_successfully = true;
        }

        // Handle pass
        if(!played_successfully) { 
            strategy->observePass(current_player); 
            passed[current_player] = true; 
        }

        // Next player's turn
        current_player = (current_player + 1) % nP;

        // Check if all players have passed (game ends)
        if(std::all_of(passed.begin(), passed.end(), [](bool p) { return p; })) {
            std::cout << "\n\nAll players have passed! Game over.\n";
            game_over = true;
        }
    }

    // Calculate final rankings
    std::vector<std::pair<uint64_t, uint64_t>> scoreWithId;
    for(uint64_t i = 0; i < nP; ++i) {
        scoreWithId.emplace_back(i, scores[i]);
    }
    
    // Sort by score (ascending - fewer cards is better)
    std::sort(scoreWithId.begin(), scoreWithId.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Display final scores
    std::cout << "\n--- Final Scores ---\n";
    for(const auto& [id, score] : scoreWithId) {
        std::cout << "Player " << id << " : " << score << " cards remaining\n";
    }
    
    return scoreWithId;
}










/**
 * Wrapper function for game with player names
 */
std::vector<std::pair<std::string, uint64_t>>
MyGameMapper::compute_game_progress(const std::vector<std::string>& names) {
    auto results = compute_game_progress(names.size());
    std::vector<std::pair<std::string, uint64_t>> named_results;
    for(auto& [id, rank] : results) named_results.emplace_back(names[id], rank);
    return named_results;
}

/**
 * Wrapper function to compute game with player names and display results
 */
std::vector<std::pair<std::string, uint64_t>>
MyGameMapper::compute_and_display_game(const std::vector<std::string>& names) {
    auto results = compute_game_progress(names);
    for(auto& [name, rank] : results) std::cout << name << " → Rank " << rank << '\n';
    return results;
}









/**
 * Multi-round game mode that continues until a player reaches maxScore
 * Each round, players accumulate points based on cards left in hand
 */
std::vector<std::pair<uint64_t, uint64_t>>
MyGameMapper::compute_multiple_rounds_to_score(uint64_t numPlayers, uint64_t maxScore) {

    // Track total score and number of wins for each player
    std::vector<uint64_t> totalScores(numPlayers, 0);
    std::vector<uint64_t> winCounts(numPlayers, 0);

    uint64_t roundNumber = 1;
    bool gameOver = false;

    while (!gameOver) {
        std::cout << "\n\n=== ROUND " << roundNumber << " ===\n";

        // Show current total scores
        std::cout << "--- Current Scores ---\n";
        for (uint64_t i = 0; i < numPlayers; ++i) {
            std::cout << "Player " << i << ": " << totalScores[i] << " points\n";
        }

        // Simulate one round: returns {playerId, score}
        auto roundScores = MyGameMapper::compute_game_progress(numPlayers);

        // Find the best (lowest) score in this round
        uint64_t bestScore = std::numeric_limits<uint64_t>::max();
        for (const auto& [id, score] : roundScores) {
            bestScore = std::min(bestScore, score);
        }

        std::cout << "--- Round Results ---\n";

        // Update total scores and register wins
        for (const auto& [id, score] : roundScores) {
            totalScores[id] += score;

            if (score == bestScore) {
                winCounts[id]++;  // player won this round
            }

            std::cout << "Player " << id << ": +" << score 
                      << " points (Total: " << totalScores[id] << ")\n";
        }

        // Check if any player reached the maxScore (game over condition)
        for (uint64_t i = 0; i < numPlayers; ++i) {
            if (totalScores[i] >= maxScore) {
                gameOver = true;
                break;
            }
        }

        roundNumber++;
    }

    uint64_t totalRounds = roundNumber - 1;

    // --- Prepare final ranking info ---
    struct PlayerStats {
        uint64_t id;
        uint64_t score;
        uint64_t wins;
    };

    std::vector<PlayerStats> players;
    for (uint64_t i = 0; i < numPlayers; ++i) {
        players.push_back({i, totalScores[i], winCounts[i]});
    }

    // Sort by: lowest score first, highest win count second
    std::sort(players.begin(), players.end(), [](const PlayerStats& a, const PlayerStats& b) {
        if (a.score != b.score) return a.score < b.score;
        return a.wins > b.wins;
    });

    // --- Display Final Results ---
    std::cout << "\n=== FINAL RESULTS AFTER " << totalRounds << " ROUNDS ===\n";

    std::vector<std::pair<uint64_t, uint64_t>> finalResults;  // {playerId, rank}
    for (size_t i = 0; i < players.size(); ++i) {
        uint64_t rank = i + 1;

        // Handle ties (same score and win count)
        if (i > 0 && players[i].score == players[i - 1].score &&
                    players[i].wins  == players[i - 1].wins) {
            rank = finalResults.back().second;
        }

        finalResults.emplace_back(players[i].id, rank);

        double winRate = (totalRounds > 0) ? (100.0 * players[i].wins / totalRounds) : 0.0;

        std::cout << "Rank " << rank
                  << " | Player " << players[i].id
                  << " | Score: " << players[i].score
                  << " | Wins: " << players[i].wins
                  << " | Win Rate: " << winRate << "%"
                  << " | Name: " << strategies[players[i].id]->getName()
                  << "\n";
    }

    // Optional: sort final results by player ID before returning
    std::sort(finalResults.begin(), finalResults.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });

    return finalResults;
}


} // namespace sevens
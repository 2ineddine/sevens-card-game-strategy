
#include <iostream>
#include <string>
#include <vector>
#include <memory>

// Inclure les fichiers de ton framework
#include "MyGameMapper.hpp"
#include "StrategyLoader.hpp"

// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: ./sevens_game "
                     "[internal|demo|competition|tournament] "
                     "[args...] [deck.txt table.txt]\n";
        return 1;
    }

    std::string mode = argv[1];

    // -------------------------------------------------------------------------
    // (Optionnel) fichiers deck & table – toujours présents dans la ligne de
    // commande d’origine, bien qu’ils ne soient pas utilisés par MyGameMapper.
    // -------------------------------------------------------------------------
    std::string deckFile  = "";
    std::string tableFile = "";
    if (argc >= 4 &&
        std::string(argv[argc - 2]).find(".so") == std::string::npos &&
        std::string(argv[argc - 1]).find(".so") == std::string::npos)
    {
        deckFile  = argv[argc - 2];
        tableFile = argv[argc - 1];
        argc -= 2; // on retire ces deux arguments du comptage des .so
    }

    // -------------------------------------------------------------------------
    // INTERNAL  ────────────────────────────────────────────────────────────────
    // -------------------------------------------------------------------------
    if (mode == "internal") {
        std::cout << "[main] Internal mode → RandomAgressiveStrategy via .so\n";

        int numPlayers = (argc >= 3) ? std::stoi(argv[2]) : 4;

        sevens::MyGameMapper game;
        std::vector<std::shared_ptr<sevens::PlayerStrategy>> strategies;

        game.read_cards();  // lit le paquet standard
        game.read_game();   // place 7♦ au centre

        for (int i = 0; i < numPlayers; ++i) {
            auto strat = sevens::StrategyLoader::loadFromLibrary("./RandomAgressiveStrategy.so");
            strat->initialize(i);
            game.registerStrategy(i, strat);
            strategies.push_back(strat);
            std::cout << "J" << i << " → RandomAgressiveStrategy\n";
        }
        game.compute_and_display_game(numPlayers);
    }

    // -------------------------------------------------------------------------
    // DEMO  (modifié pour RandomAgressive / Calculative)  ─────────────────────
    // -------------------------------------------------------------------------
    else if (mode == "demo") {
        std::cout << "[main] Demo mode → alternance RandomAgressive/Calculative via .so\n";

        int numPlayers = (argc >= 3) ? std::stoi(argv[2]) : 4;

        sevens::MyGameMapper game;
        std::vector<std::shared_ptr<sevens::PlayerStrategy>> strategies;

        game.read_cards();
        game.read_game();

        for (int i = 0; i < numPlayers; ++i) {
            std::string path = (i % 2 == 0)
                             ? "./RandomAgressiveStrategy.so"
                             : "./CalculativeStrategy.so";
            auto strat = sevens::StrategyLoader::loadFromLibrary(path);
            strat->initialize(i);
            game.registerStrategy(i, strat);
            strategies.push_back(strat);
            std::cout << "J" << i << " → " << strat->getName() << '\n';
        }
        game.compute_and_display_game(numPlayers);
    }

    // -------------------------------------------------------------------------
    // COMPETITION  ─────────────────────────────────────────────────────────────
    // -------------------------------------------------------------------------
    else if (mode == "competition") {
        if (argc < 4) {
            std::cerr << "[main] Usage: ./sevens_game competition strat1.so strat2.so [...]\n";
            return 1;
        }

        std::cout << "[main] Competition mode → chargement dynamique de stratégies\n";

        sevens::MyGameMapper game;
        std::vector<std::shared_ptr<sevens::PlayerStrategy>> strategies;

        game.read_cards();
        game.read_game();

        for (int i = 2; i < argc; ++i) {
            std::string path = argv[i];
            auto strat = sevens::StrategyLoader::loadFromLibrary(path);
            strat->initialize(i - 2);
            game.registerStrategy(i - 2, strat);
            strategies.push_back(strat);
            std::cout << "J" << i - 2 << " → " << strat->getName() << '\n';
        }
        game.compute_and_display_game(strategies.size());
    }

    // -------------------------------------------------------------------------
    // TOURNAMENT (multi-round jusqu’à 50 pts)  ────────────────────────────────
    // -------------------------------------------------------------------------
    else if (mode == "tournament") {
        if (argc < 4) {
            std::cerr << "[main] Usage: ./sevens_game tournament strat1.so strat2.so [...]\n";
            return 1;
        }

        std::cout << "[main] CompetitiveTo50 mode → multiple rounds until a player reaches 50 points\n";

        sevens::MyGameMapper game;
        std::vector<std::shared_ptr<sevens::PlayerStrategy>> strategies;

        game.read_cards();
        game.read_game();

        for (int i = 2; i < argc; ++i) {
            std::string path = argv[i];
            auto strat = sevens::StrategyLoader::loadFromLibrary(path);
            strat->initialize(i - 2);
            game.registerStrategy(i - 2, strat);
            strategies.push_back(strat);
            std::cout << "J" << i - 2 << " → " << strat->getName() << '\n';
        }

        std::cout << "\n=== Starting multi-round competition until 50 points ===\n";
        game.compute_multiple_rounds_to_score(strategies.size(), 50);
    }

    // -------------------------------------------------------------------------
    // MODE INCONNU  ────────────────────────────────────────────────────────────
    // -------------------------------------------------------------------------
    else {
        std::cerr << "[main] Unknown mode: " << mode << std::endl;
    }

    return 0;
}

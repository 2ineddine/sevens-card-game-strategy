#pragma once

#include "PlayerStrategy.hpp"
#include <memory>
#include <string>

namespace sevens {

/**
 * Utility class for loading player strategies from shared libraries.
 */
class StrategyLoader {
public:
    // Déclaration seulement — l'implémentation est dans StrategyLoader.cpp
    static std::shared_ptr<PlayerStrategy> loadFromLibrary(const std::string& libraryPath);
};

} // namespace sevens
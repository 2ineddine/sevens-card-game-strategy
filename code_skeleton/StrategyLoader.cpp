#include "StrategyLoader.hpp"
#include <dlfcn.h>
#include <stdexcept>
#include <iostream>

namespace sevens {

std::shared_ptr<PlayerStrategy> StrategyLoader::loadFromLibrary(const std::string& libraryPath) {
    std::cout << "[StrategyLoader] Tentative de chargement : " << libraryPath << std::endl;

    // Ouvrir la bibliothèque partagée
    void* handle = dlopen(libraryPath.c_str(), RTLD_LAZY);
    if (!handle) {
        std::string err = dlerror();
        throw std::runtime_error("Impossible de charger la bibliothèque : " + libraryPath + "\nErreur : " + err);
    }

    dlerror(); // Réinitialiser les erreurs précédentes

    // Récupérer le symbole de création de stratégie
    using CreateStrategyFn = PlayerStrategy* (*)();
    CreateStrategyFn create = reinterpret_cast<CreateStrategyFn>(dlsym(handle, "createStrategy"));

    const char* error = dlerror();
    if (error != nullptr) {
        dlclose(handle);
        throw std::runtime_error("Erreur lors du chargement de createStrategy : " + std::string(error));
    }

    // Créer et retourner la stratégie
    PlayerStrategy* strategy = create();
    if (!strategy) {
        dlclose(handle);
        throw std::runtime_error("Échec de la création de la stratégie depuis " + libraryPath);
    }

    std::cout << "[StrategyLoader] Stratégie chargée avec succès depuis : " << libraryPath << std::endl;
    return std::shared_ptr<PlayerStrategy>(strategy);
}

} // namespace sevens

/*
Cette classe pour la génération des cartes  

*/
#pragma once
#include <cstdint>

#include <string>
#include <unordered_map>
#include <iostream>   

namespace sevens {

/**
 * Represents a single playing card for Sevens:
 *   suit: 0..3  (0=Clubs,1=Diamonds,2=Hearts,3=Spades)
 *   rank: 1..13 (1=Ace,2=2,...,13=King)
 */
struct Card {
    int suit; // 0 : ♠, 1 : ♥, 2 : ♦, 3 : ♣
    int rank; // numéro de la carte (1-13)

    friend std::ostream& operator<<(std::ostream& os, const Card& card) {
        os << "Card(suit=" << card.suit << ", rank=" << card.rank << ")";
        return os;
    }
};

/**
 * Base class for reading or generating cards.
 * Subclasses must override read_cards(...) to populate cards_hashmap.
 */

 /*
 std::unordered_map <uint64_t, Card> ; est le format standard en cpp pour les dictionnaires <int, card>, ici card 
 aussi est une structure qui est une sorte de dictionnaire , ce dictionnaire n'est pas trié  ; 
 
 */
class Generic_card_parser {
public:
    virtual void read_cards() = 0;

    // Provide read-only access to the internal card map
    const std::unordered_map<uint64_t, Card>& get_cards_hashmap() const {
        return this->cards_hashmap;
    }

protected:
    // Key = card ID (0..51), Value = Card struct
    std::unordered_map<uint64_t, Card> cards_hashmap;
};

} // namespace sevens

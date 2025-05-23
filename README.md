# Sevens Card Game – C++ Implementation and Strategy Development

## Team Members

* **Student 1**: Zineddine BOUHADJIRA  
* **Student 2**: Massyl ADJAL

---

## Project Overview

This project involves the implementation of the card game **Sevens** using C++.  
The aim was twofold: to simulate the game with multiple strategies, and to design and evaluate a custom strategy capable of competing with strategies developed by our classmates.

The core of the project was to understand the architecture of the provided codebase, extend it to simulate complete games with multiple players and strategies, and analyze gameplay using our implemented strategy.

---

## Game Rules and Specifics

Our version of the **Sevens** game adheres to the following rules and particularities:

* The game starts with the **7♦ (Seven of Diamonds)** on the table.  
* Only a **Seven** can be used to **open a new suit**.  
* Players can play a **Seven at any time**, not necessarily when they first receive it.  
* A valid play consists of:
  * A **Seven** (to open a suit), or  
  * A card **adjacent** to those already played in a suit (cards range from Ace (1) to King (13)).  
* Players **may choose to pass**, even if they have valid playable cards.  
* The game ends when a player **empties their hand**.  
* Scoring is based on the **number of remaining cards**. The player with the fewest cards gets the best rank.  
* Each player knows:
  * Their **own hand**,  
  * The **table layout** (cards already played),  
  * The **move history** (who played or passed).

---

## Codebase Architecture

Before implementing our strategy, we analyzed and extended the provided modular project structure:
```
main                              // Entry point - game mode selector
├── StrategyLoader                // Dynamically loads player strategies - dlopen/dlsym wrapper for .so strategies
├── PlayerStrategy                // Base class for strategies
│ ├── RandomAgressiveStrategy
│ ├── CalculativeStrategy
| ├── PrudentStrategy 
│ └── Sentinel7                   // Our custom strategy
├── Generic_card_parser           // Defines Card structure, and cards_hashmap map
│ └── MyCardParser                // Builds the deck of 52 cards
├── Generic_game_parser           // Defines table_layout map matrix
│ └── MyGameParser                // Initializes the game table
├── Generic_game_mapper
│ └── MyGameMapper                // Handles gameplay simulation and display: shuffling, dealing, turn loop, scores

``` 


---

## Build Instructions

### 1. Compile the strategy libraries
```
# Baseline aggressive random bot
g++ -std=c++17 -Wall -Wextra -fPIC -shared RandomAgressiveStrategy.cpp -o RandomAgressiveStrategy.so

# Baseline calculative bot
g++ -std=c++17 -Wall -Wextra -fPIC -shared CalculativeStrategy.cpp -o CalculativeStrategy.so

# Our Sentinel7 bot 
g++ -std=c++17 -Wall -Wextra -O3 -fPIC -DBUILD_SHARED_LIB -shared Sentinel7.cpp -o Sentinel7.so
```
### 2. Compile the framework executable
```
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -pedantic-errors -O3 -ldl \
main.cpp MyGameMapper.cpp MyGameParser.cpp MyCardParser.cpp StrategyLoader.cpp \
-o sevens_game
```

### 3. Running the Program
| Mode         | What happens                                                                                                   | Example command                                    |
|--------------|----------------------------------------------------------------------------------------------------------------|----------------------------------------------------|
| `internal`   | Every player uses **RandomAgressiveStrategy.so**.                                                              | `./sevens_game internal 4`                         |
| `demo`       | Players alternate between **RandomAgressiveStrategy.so** and **CalculativeStrategy.so**.                      | `./sevens_game demo 4`                             |
| `competition`| Explicit list of strategy libraries (one per player).                                                          | `./sevens_game competition Bot1.so Bot2.so …`      |
| `tournament` | Same arguments as **competition**, but rounds continue until someone hits **50 pts**.                          | `./sevens_game tournament Bot1.so Bot2.so …`       |

PS : The max score of the tournament mode can be changed in main.cpp  
  
  
   
---

## Key Implementations

* **MyCardParser::read_cards()** – Constructs and returns the full deck.  
* **MyGameParser::read_game()** – Sets up the table with 7♦ only.  
* **MyGameMapper**:
  * `printCard()` / `printTable()` – Debug display.  
  * `compute_game_progress()` – Runs a single round.  
  * `compute_multiple_rounds_to_score()` – Plays successive rounds until a score limit (default 50 pts).  
* **main.cpp** – Supports four modes: `internal`, `demo`, `competition`, `tournament`.


---

## Implemented Strategy and Justification

### Name  : **Sentinel7**
**Sentinel7** – a *semi-defensive blocker* balancing self-progress with opponent throttling.

### Decision Workflow

1. **Enumerate playable cards**.  
2. **Score** each candidate on seven weighted criteria.  
3. Pick the top-scoring move; if several are within 20% of the best score, pick one at random for unpredictability.

| # | Feature (weight) | Rationale |
|---|------------------|-----------|
| 1 | High ranks (10–K) & Ace | Dump hard-to-place high cards early |
| 2 | Cards that **unlock** the most of our hand | Snowball tempo |
| 3 | Suit management:<br> • shed *short* suits (≤ 2)<br> • exploit *long* suits (≥ 7) | Keeps options open / builds runs |
| 4 | **Blocking gaps** in opponents’ key suits | Slows them down |
| 5 | Critical cards 7 / 6 / 8 policy | Hold early, release under pressure |
| 6 | Potential to play a **run** next turn | Multi-turn payoff |
| 7 | End-game pressure (hand ≤ 5) & slight penalty for extreme ranks | Finish quickly without locking oneself |

---

## Sample Performance

We ran tournaments (using our fourth game mode `tournament`) with either 3 or 4 players per game, and a maximum score of 100000 unless otherwise specified:

| Line-up                                           | Max Score | Rounds | Sentinel7 Rank | Win Rate (%) | Wins   | Notes                                                  |
|--------------------------------------------------|-----------|--------|----------------|--------------|--------|--------------------------------------------------------|
| Sentinel7 + RandomAggressive ×2                  | 1000      | 769    | 1ᵉ             | 36.02 %      | 277    | Both opponents were identical strategy variants       |
| Sentinel7 + Prudent ×2                           | 1000      | 648    | 1ᵉ             | 37.96 %      | 246    | Beats duplicate Prudent                               |
| Sentinel7 + Calculative ×2                       | 1000      | 746    | 1ᵉ             | 36.06 %      | 269    | Very close between all strategies                     |
| Sentinel7 + Prudent + Calculative + Hybrid       | 1000      | 653    | 1ᵉ             | 28.02 %      | 183    | Balanced field                                        |
| Sentinel7 + Greedy + Random                      | 100000    | 72053  | 1ᵉ             | 35.47 %      | 25559  | Very strong showing                                   |

  
Our Strategy Sentinel7 finishes 1ᵉʳ or 2ᵉ in most cases and clearly beats the baseline strategies.

### Screenshots

* **test 1 :**  
![tournament test 1](./test_screenshots/tournament%20test%201.png)  
* **test 2 :**  
![tournament test 2](./test_screenshots/tournament%20test%202.png)  
* **test 3 :**  
![tournament test 3](./test_screenshots/tournament%20test%203.png)  
* **test 4 :**  
![tournament test 4](./test_screenshots/tournament%20test%204.png)  
* **test 5 :**  
![tournament test 5](./test_screenshots/tournament%20test%205.png)
---

## Limitations and Conclusions

* **No deep look-ahead** – purely myopic; Monte Carlo rollouts could improve late-game decision-making.  
* **Coarse opponent model** – tracks only remaining card counts; no probability inference of specific holdings.  
* **Stochastic tie-breaking** – helps with unpredictability, but may occasionally choose sub-optimal plays.

Despite these, **Sentinel7** consistently outperforms baseline bots and remains computationally efficient, making it suitable for fast tournament runs.

### Future Work

1. Bayesian tracking of unseen critical cards.  
2. Limited two-ply look-ahead for end-game scenarios.  
3. Windows compatibility (current dynamic loader targets Linux `dlopen`).

---

## Credits and References

* Base framework and source code supplied by **Janan Arslan** – **Sorbonne University – MU4RBI02**.  
* Strategy design inspired by:  
  * *“Optimal Play in Fan-Tan”*, Math. Games Bulletin 2012.  
  * <https://www.wikihow.com/Play-Sevens-(Card-Game)>  
  * Reddit discussions on Sevens tactics (`r/ClubhouseGames`)  
  * Personal experimentation and in-class matches  
* Special thanks to **Janan Arslan** (Q&A on Moodle) and classmates for testing.

---

*Report last updated: **19 May 2025** (Europe/Paris).*  
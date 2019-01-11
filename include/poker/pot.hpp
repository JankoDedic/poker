#pragma once

#include <vector>

#include <poker/player.hpp>

namespace poker {

class pot {
    std::vector<player*> _eligible_players;
    chips _pot_size{0};

public:
    pot() noexcept = default;

    enum class gather_outcome {
        all_bets_collected,
        some_bets_remaining,
        some_players_bankrupt
    };

    gather_outcome
    gather_bets_from(span<player*> players) noexcept
    {
        auto min_bet = chips(0);
        for (auto p : players) {
            if (p && p->bet_size() != 0 && p->bet_size() < min_bet) {
                min_bet = p->bet_size();
            }
        }
        if (min_bet == 0) {
            return gather_outcome::all_bets_collected;
            return false;
        }
        // Make a minimal example and ask on r/vim?
        const auto bet_per_player = min_bet;
        auto goutcome = gather_outcome::some_bets_remaining;
        _eligible_players.clear();
        for (auto p : players) {
            if (p && p->bet_size() != 0) {
                p->take_from_bet(bet_per_player);
                _pot_size += bet_per_player;
                _eligible_players.push_back(p);
                if (p->stack() == 0) {
                    goutcome = gather_outcome::some_players_bankrupt;
                }
            }
        }
        return goutcome;
    }
};

// When a player folds, can we just remove him from all pots?
// When player folds, his bet_size is immediately claimed. He is removed from all pots.
// When the bets are gathered again, the nulled player won't be a part of the pot anyway.

class pot_manager {
    std::vector<pot> _pots;
    // we're always accessing the last pot only

public:
    pot_manager() noexcept
        : _pots(1)
    {
    }

    void
    gather_bets(span<player*> players) noexcept
    {
        for (;;) {
            switch (_pots.back().gather_bets_from(players)) {
            case pot::outcome::all_bets_collected: return;
            case pot::outcome::some_bets_remaining: continue;
            case pot::outcome::some_players_bankrupt:
                for (auto p : players) {
                    if (p && p->stack() != 0) {
                        _pots.emplace_back();
                        break;
                    }
                }
            }
        }
    }
};

} // namespace poker

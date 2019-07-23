#pragma once

#include <vector>

#include <poker/player.hpp>
#include <poker/detail/seat_array.hpp>

namespace poker {

class pot {
    std::vector<player*> _eligible_players;
    chips _size;

public:
    pot() noexcept : _size{0} {}

    auto size() const noexcept -> chips { return _size; }

    auto eligible_players() const noexcept -> span<player* const> { return _eligible_players; }

    void add(chips amount) noexcept {
        assert(amount >= 0);
        _size += amount;
    }

    auto collect_bets_from(seat_array_view players) noexcept -> chips {
        auto it = std::find_if(players.begin(), players.end(), [] (const auto& p) { return p.bet_size() != 0; });
        if (it == players.end()) {
            for (auto& p : players) {
                _eligible_players.push_back(&p);
            }
            return 0;
        } else {
            auto min_bet = (*it).bet_size();
            std::for_each(it, players.end(), [&] (const auto& p) {
                if (p.bet_size() != 0 && p.bet_size() < min_bet) {
                    min_bet = p.bet_size();
                }
            });
            _eligible_players.clear();
            for (auto& p : players) {
                if (p.bet_size() != 0) {
                    p.take_from_bet(min_bet);
                    _size += min_bet;
                    _eligible_players.push_back(&p);
                }
            }
            return min_bet;
        }
    }
};

} // namespace poker

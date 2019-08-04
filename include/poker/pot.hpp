#pragma once

#include <vector>

#include <poker/player.hpp>
#include <poker/seat_array.hpp>

namespace poker {

class pot {
    std::vector<seat_index> _eligible_players;
    chips _size;

public:
    pot() noexcept : _size{0} {}

    auto size() const noexcept -> chips {
        return _size;
    }

    auto eligible_players() const noexcept -> span<const seat_index> {
        return _eligible_players;
    }

    void add(chips amount) POKER_NOEXCEPT {
        POKER_DETAIL_ASSERT(amount >= 0, "Cannot add a negative amount to the pot");
        _size += amount;
    }

    auto collect_bets_from(seat_array_view players) noexcept -> chips {
        auto it = std::find_if(players.begin(), players.end(), [] (const auto& p) { return p.bet_size() != 0; });
        if (it == players.end()) {
            for (auto iter = players.begin(); iter != players.end(); ++iter) {
                _eligible_players.push_back(iter.index());
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
            for (auto iter = players.begin(); iter != players.end(); ++iter) {
                if ((*iter).bet_size() != 0) {
                    (*iter).take_from_bet(min_bet);
                    _size += min_bet;
                    _eligible_players.push_back(iter.index());
                }
            }
            return min_bet;
        }
    }
};

} // namespace poker

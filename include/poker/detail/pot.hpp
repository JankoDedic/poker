#pragma once

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

    // Two distinct cases to test.
    TEST_CASE_CLASS("some bets remaining") {
        auto players = seat_array{};
        players.add_player(0, player{100});
        players.add_player(1, player{100});
        players.add_player(2, player{100});
        players[0].bet(0);
        players[1].bet(20);
        //players[2].bet(60);
        auto p = pot{};
        p.collect_bets_from(players);
        REQUIRE_EQ(p.size(), 20);
        REQUIRE_EQ(p.eligible_players().size(), 1);
        // REQUIRE_EQ(players[0].bet_size(), 0);
        REQUIRE_EQ(players[1].bet_size(), 0);
    }

    // Two distinct cases to test.
    TEST_CASE_CLASS("no bets remaining") {
        auto players = seat_array{};
        players.add_player(0, player{100});
        players.add_player(1, player{100});
        players.add_player(2, player{100});
        // no bets
        auto p = pot{};
        p.collect_bets_from(players);
        REQUIRE_EQ(p.size(), 0);
        REQUIRE_EQ(p.eligible_players().size(), 3);
    }
};

} // namespace poker

#pragma once

#include <poker/player.hpp>

namespace poker::detail {

class pot {
    std::vector<player *> _eligible_players;
    chips _size;

public:
    pot() noexcept : _size{0} {}

    auto size() const noexcept -> chips { return _size; }

    auto eligible_players() const noexcept -> span<player *const> { return _eligible_players; }

    void add(chips amount) noexcept {
        assert(amount >= 0);
        _size += amount;
    }

    auto collect_bets_from(span<player *const> players) noexcept -> chips {
        auto it = std::find_if(players.begin(), players.end(), [] (auto p) { return p && p->bet_size() != 0; });
        if (it == players.end()) {
            for (auto p : players) {
                if (p) _eligible_players.push_back(p);
            }
            return 0;
        } else {
            auto min_bet = (*it)->bet_size();
            std::for_each(it, players.end(), [&] (auto p) {
                if (p && p->bet_size() != 0 && p->bet_size() < min_bet) min_bet = p->bet_size();
            });
            _eligible_players.clear();
            for (auto p : players) {
                if (p && p->bet_size() != 0) {
                    p->take_from_bet(min_bet);
                    _size += min_bet;
                    _eligible_players.push_back(p);
                }
            }
            return min_bet;
        }
    }

    // Two distinct cases to test.
    TEST_CASE_CLASS("some bets remaining") {
        player players[] = {player{100}, player{100}, player{100}};
        players[0].bet(0);
        players[1].bet(20);
        //players[2].bet(60);
        auto player_pointers = std::vector<player *>{&players[0], &players[1], &players[2]};
        auto p = pot{};
        p.collect_bets_from(player_pointers);
        REQUIRE_EQ(p._size, 20);
        REQUIRE_EQ(p._eligible_players.size(), 1);
        // REQUIRE_EQ(players[0].bet_size(), 0);
        REQUIRE_EQ(players[1].bet_size(), 0);
    }

    TEST_CASE_CLASS("no bets remaining") {
        player players[] = {player{100}, player{100}, player{100}};
        // no bets
        auto player_pointers = std::vector<player *>{&players[0], &players[1], &players[2]};
        auto p = pot{};
        p.collect_bets_from(player_pointers);
        REQUIRE_EQ(p._size, 0);
        REQUIRE_EQ(p._eligible_players.size(), 3);
    }
};

} // namespace poker::detail

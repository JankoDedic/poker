#pragma once

#include "poker/detail/pot.hpp"

namespace poker::detail {

class pot_manager {
    std::vector<pot> _pots; // FIXME: static_vector with max_players-1 capacity
    chips _aggregate_folded_bets = {0};

public:
    pot_manager() noexcept : _pots{1} {}

    auto pots() const noexcept -> span<const pot> { return _pots; }

    void bet_folded(chips amount) noexcept {
        _aggregate_folded_bets += amount;
    }

    void collect_bets_from(span<player *const> players) noexcept {
        // TODO: Return a list of transactions.
        for (;;) {
            const auto min_bet = _pots.back().collect_bets_from(players);
            const auto num_eligible_players = static_cast<chips>(_pots.back().eligible_players().size());
            const auto aggregate_folded_bets_consumed_amount = std::min(_aggregate_folded_bets, num_eligible_players * min_bet);
            _pots.back().add(aggregate_folded_bets_consumed_amount);
            _aggregate_folded_bets -= aggregate_folded_bets_consumed_amount;
            auto it = std::find_if(players.begin(), players.end(), [] (auto p) { return p && p->bet_size() != 0; });
            if (it != players.end()) {
                _pots.emplace_back();
                continue;
            } else if (_aggregate_folded_bets != 0) {
                _pots.back().add(_aggregate_folded_bets);
                _aggregate_folded_bets = 0;
            }
            break;
        }
    }

    TEST_CASE_CLASS("whatever") {
        player players[] = {player{100}, player{100}, player{100}};
        players[0].bet(20);
        players[1].bet(40);
        players[2].bet(60);
        auto player_pointers = std::vector<player *>{&players[0], &players[1], &players[2]};
        auto pm = pot_manager{};
        pm.collect_bets_from(player_pointers);
        REQUIRE_EQ(pm._pots.size(), 3);
        REQUIRE_EQ(pm._pots[0].size(), 60);
        REQUIRE_EQ(pm._pots[1].size(), 40);
        REQUIRE_EQ(pm._pots[2].size(), 20);
    }

};

} // namespace poker::detail

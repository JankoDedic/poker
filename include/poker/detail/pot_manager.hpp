#pragma once

#include <poker/pot.hpp>

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

    void collect_bets_from(seat_array_view players) noexcept {
        // TODO: Return a list of transactions.
        for (;;) {
            const auto min_bet = _pots.back().collect_bets_from(players);

            // Calculate the right amount of folded bets to add to the pot.
            // Logic: If 'x' is chips which a player committed to the pot and 'n' is number of (eligible) players in that pot,
            // a player can win exactly x*n chips (from that particular pot).
            const auto num_eligible_players = static_cast<chips>(_pots.back().eligible_players().size());
            const auto aggregate_folded_bets_consumed_amount = std::min(_aggregate_folded_bets, num_eligible_players * min_bet);
            _pots.back().add(aggregate_folded_bets_consumed_amount);
            _aggregate_folded_bets -= aggregate_folded_bets_consumed_amount;

            auto it = std::find_if(players.begin(), players.end(), [] (const auto& p) { return p.bet_size() != 0; });
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
};

} // namespace poker::detail

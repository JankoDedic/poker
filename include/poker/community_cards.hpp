#pragma once

#include <array>

#include <poker/card.hpp>
#include <poker/deck.hpp>
#include "poker/detail/error.hpp"
#include "poker/detail/span.hpp"
#include "poker/detail/utility.hpp"

namespace poker {

enum class round_of_betting {
    preflop = 0,
    flop    = 3,
    turn    = 4,
    river   = 5
};

constexpr auto next(round_of_betting rob) noexcept -> round_of_betting {
    using poker::detail::to_underlying;
    if (rob == round_of_betting::preflop) {
        return round_of_betting::flop;
    } else {
        return static_cast<round_of_betting>(to_underlying(rob) + 1);
    }
}

class community_cards {
    std::array<card, 5> _cards;
    std::size_t _size = {0};

public:
    community_cards() noexcept = default;

    auto cards() const noexcept -> span<const card> {
        return span<const card>(_cards).first(_size);
    }

    void deal(span<const card> cards) POKER_NOEXCEPT {
        POKER_DETAIL_ASSERT(static_cast<std::size_t>(cards.size()) <= 5 - _size, "Cannot deal more than there is undealt cards");
        for (auto c : cards) _cards[_size++] = c;
    }
};

} // namespace poker

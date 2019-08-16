#pragma once

#include <poker/hole_cards.hpp>
#include "poker/detail/error.hpp"

namespace poker {

using chips = int;

class player {
    chips _total = {0};
    chips _bet_size = {0};

public:
    player() = default;

    constexpr explicit player(chips stack) noexcept
        : _total(stack)
    {
    }

    constexpr auto stack() const noexcept -> chips {
        return _total - _bet_size;
    }

    constexpr auto bet_size() const noexcept -> chips {
        return _bet_size;
    }

    constexpr auto total_chips() const noexcept -> chips {
        return _total;
    }

    constexpr void add_to_stack(chips amount) noexcept {
        _total += amount;
    }

    constexpr void take_from_stack(chips amount) noexcept {
        _total -= amount;
    }

    constexpr void bet(chips amount) POKER_NOEXCEPT {
        POKER_DETAIL_ASSERT(amount <= _total, "Player cannot bet more than he has");
        POKER_DETAIL_ASSERT(amount >= _bet_size, "Player must bet more than he has previously");
        _bet_size = amount;
    }

    constexpr void take_from_bet(chips amount) POKER_NOEXCEPT {
        POKER_DETAIL_ASSERT(amount <= _bet_size, "Cannot take from bet more than is there");
        _total -= amount;
        _bet_size -= amount;
    }
};

} // namespace poker

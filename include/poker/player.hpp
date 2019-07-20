#pragma once

#include <cassert>

#include <poker/hole_cards.hpp>

namespace poker {

using chips = int;

class player {
    chips _total = {0};
    chips _bet_size = {0};

public:
    poker::hole_cards hole_cards = {};

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

    constexpr void bet(chips amount) noexcept {
        assert(amount <= _total);
        assert(amount >= _bet_size);
        _bet_size = amount;
    }

    constexpr void take_from_bet(chips amount) noexcept {
        assert(amount <= _bet_size);
        _total -= amount;
        _bet_size -= amount;
    }
};

} // namespace poker

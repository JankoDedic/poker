#pragma once

#include <cassert>

#include <poker/hole_cards.hpp>

namespace poker {

using chips = int;

class player {
    chips _total;
    chips _bet_size;

public:
    hole_cards hole_cards;

    constexpr
    player(chips stack) noexcept
        : _total(stack)
        , _bet_size(0)
    {
    }

    constexpr
    chips
    stack() const noexcept
    {
        return _total - _bet_size;
    }

    constexpr
    chips
    bet_size() const noexcept
    {
        return _bet_size;
    }

    constexpr
    void
    add_to_stack(chips amount) noexcept
    {
        _total += amount;
    }

    constexpr
    void
    bet(chips amount) noexcept
    {
        assert(amount <= _total);
        assert(amount > _bet_size);
        _bet_size = amount;
    }

    constexpr
    void
    take_from_bet(chips amount) noexcept
    {
        assert(amount <= _bet_size);
        _total -= amount;
        _bet_size -= amount;
    }
};

} // namespace poker

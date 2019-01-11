#pragma once

#include <array>
#include <cassert>

#include <poker/card.hpp>
#include "poker/detail/span.hpp"

namespace poker {

class community_cards {
    std::array<card, 5> _cards;
    std::size_t _size{0};

public:
    community_cards() noexcept = default;

    span<const card>
    cards() const noexcept
    {
        return span<const card>(_cards).first(_size);
    }

    void
    deal_flop(card first, card second, card third) noexcept
    {
        assert(_size == 0);
        _cards[0] = first;
        _cards[1] = second;
        _cards[2] = third;
        _size = 3;
    }

    void
    deal_turn(card c) noexcept
    {
        assert(_size == 3);
        _cards[3] = c;
        ++_size;
    }

    void
    deal_river(card c) noexcept
    {
        assert(_size == 4);
        _cards[4] = c;
        ++_size;
    }
};

} // namespace poker

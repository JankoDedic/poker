#pragma once

#include <array>
#include <cassert>

#include <poker/card.hpp>
#include "poker/detail/utility.hpp"

namespace poker {

class deck {
    std::array<card, 52> _cards;
    std::size_t _size;

public:
    template<class URBG>
    deck(URBG&& g)
        : _size(52)
    {
        /* using poker::detail::to_underlying; */
        constexpr auto first_suit = to_underlying(card_suit::clubs);
        constexpr auto last_suit = to_underlying(card_suit::spades);
        constexpr auto first_rank = to_underlying(card_rank::_2);
        constexpr auto last_rank = to_underlying(card_rank::A);
        auto i = static_cast<std::size_t>(0);
        for (auto suit = first_suit; suit <= last_suit; ++suit) {
            for (auto rank = first_rank; rank <= last_rank; ++rank) {
                const auto arank = static_cast<card_rank>(rank);
                const auto asuit = static_cast<card_suit>(suit);
                _cards[i] = card{arank, asuit};
                ++i;
            }
        }
        std::shuffle(begin(_cards), end(_cards), std::forward<URBG>(g));
    }

    template<class URBG>
    void
    fill_and_shuffle(URBG&& g) noexcept
    {
        _size = 52;
        std::shuffle(begin(_cards), end(_cards), std::forward<URBG>(g));
    }

    [[nodiscard]]
    card
    draw() noexcept
    {
        assert(_size >= 0);
        return _cards[--_size];
    }

    std::size_t
    size() const noexcept
    {
        return _size;
    }
};

} // namespace poker
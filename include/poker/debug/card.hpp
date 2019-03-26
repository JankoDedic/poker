#pragma once

#include <cassert>
#include <iostream>
#include <string_view>

#include <poker/card.hpp>

namespace poker::debug {

inline auto make_card(std::string_view str) noexcept -> card {
    const auto rank = [&] {
        constexpr char rank_symbols[] = {
            '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'
        };
        const auto first = std::begin(rank_symbols);
        const auto last = std::end(rank_symbols);
        const auto it = std::find(first, last, str.front());
        assert(it != last);
        return static_cast<card_rank>(it - rank_symbols);
    }();
    const auto suit = [&] {
        constexpr char suit_symbols[] = { 'c', 'd', 'h', 's' };
        const auto first = std::begin(suit_symbols);
        const auto last = std::end(suit_symbols);
        const auto it = std::find(first, last, str.back());
        assert(it != last);
        return static_cast<card_suit>(it - suit_symbols);
    }();
    return card{rank, suit};
}

template<std::size_t N>
inline auto make_cards(std::string_view str) noexcept -> std::array<card, N> {
    auto cards = std::array<card, N>();
    for (auto i = 0; i < N; ++i) {
        cards[i] = make_card(str.substr(0, 2));
        if (i != N-1) {
            str.remove_prefix(3);
        }
    }
    return cards;
}

inline auto operator<<(std::ostream& os, card_rank rank) -> std::ostream& {
    constexpr char rank_symbols[] = {
        '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'
    };
    return os << rank_symbols[static_cast<std::size_t>(rank)];
}

inline auto operator<<(std::ostream& os, card_suit suit) -> std::ostream& {
    constexpr char suit_symbols[] = { 5, 4, 3, 6 };
    return os << suit_symbols[static_cast<std::size_t>(suit)];
}

inline auto operator<<(std::ostream& os, card c) -> std::ostream& {
    return os << c.rank << c.suit;
}

} // namespace poker::debug

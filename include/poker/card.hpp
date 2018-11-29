#pragma once

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>

#include "poker/detail/span.hpp"

namespace poker {

enum class card_rank { _2, _3, _4, _5, _6, _7, _8, _9, T, J, Q, K, A };
enum class card_suit { clubs, diamonds, hearts, spades };

struct card {
    card_rank rank;
    card_suit suit;
};

constexpr
bool
operator==(card lhs, card rhs) noexcept
{
    return lhs.rank == rhs.rank && lhs.suit == rhs.suit;
}

constexpr
bool
operator!=(card lhs, card rhs) noexcept
{
    return !(lhs == rhs);
}

constexpr
bool
operator<(card lhs, card rhs) noexcept
{
    return std::tie(lhs.suit, lhs.rank) < std::tie(rhs.suit, rhs.rank);
}

constexpr
bool
operator>(card lhs, card rhs) noexcept
{
    return rhs < lhs;
}

constexpr
bool
operator<=(card lhs, card rhs) noexcept
{
    return !(rhs < lhs);
}

constexpr
bool
operator>=(card lhs, card rhs) noexcept
{
    return !(lhs < rhs);
}

} // namespace poker

namespace poker::debug {

inline
card
make_card(std::string_view str) noexcept
{
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
inline
std::array<card, N>
make_cards(std::string_view str) noexcept
{
    auto cards = std::array<card, N>();
    for (auto i = 0; i < N; ++i) {
        cards[i] = make_card(str.substr(0, 2));
        if (i != N-1) {
            str.remove_prefix(3);
        }
    }
    return cards;
}

inline
std::ostream&
operator<<(std::ostream& os, card_rank rank)
{
    constexpr char rank_symbols[] = {
        '2', '3', '4', '5', '6', '7', '8', '9', 'T', 'J', 'Q', 'K', 'A'
    };
    return os << rank_symbols[static_cast<std::size_t>(rank)];
}

inline
std::ostream&
operator<<(std::ostream& os, card_suit suit)
{
    constexpr char suit_symbols[] = { 5, 4, 3, 6 };
    return os << suit_symbols[static_cast<std::size_t>(suit)];
}

inline
std::ostream&
operator<<(std::ostream& os, card c)
{
    return os << c.rank << c.suit;
}

} // namespace poker::debug

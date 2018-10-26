#pragma once

#include <iostream>

#include <poker/span.hpp>

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

} // namespace poker

namespace poker::debug {

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

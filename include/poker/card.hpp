#pragma once

#include <tuple>

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


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

constexpr auto operator== ( card lhs, card rhs ) noexcept -> bool { return lhs.rank == rhs.rank && lhs.suit == rhs.suit;                }
constexpr auto operator!= ( card lhs, card rhs ) noexcept -> bool { return !(lhs == rhs);                                               }
constexpr auto operator<  ( card lhs, card rhs ) noexcept -> bool { return std::tie(lhs.suit, lhs.rank) < std::tie(rhs.suit, rhs.rank); }
constexpr auto operator>  ( card lhs, card rhs ) noexcept -> bool { return rhs < lhs;                                                   }
constexpr auto operator<= ( card lhs, card rhs ) noexcept -> bool { return !(rhs < lhs);                                                }
constexpr auto operator>= ( card lhs, card rhs ) noexcept -> bool { return !(lhs < rhs);                                                }

} // namespace poker


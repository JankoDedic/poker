#pragma once

#include <iostream>

#include <poker/hand.hpp>

namespace poker::debug {

inline
std::ostream&
operator<<(std::ostream& os, hand_ranking hr)
{
    constexpr const char* hand_ranking_names[] = {
        "high card",
        "pair",
        "two pair",
        "three of a kind",
        "straight",
        "flush",
        "full house",
        "four of a kind",
        "straight flush",
        "royal flush"
    };
    return os << hand_ranking_names[static_cast<std::size_t>(hr)];
}

inline
hand
make_hand(std::string_view str) noexcept
{
    auto cards = make_cards<7>(str);
    return hand(cards);
}

} // namespace poker::debug

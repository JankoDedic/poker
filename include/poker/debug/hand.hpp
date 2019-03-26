#pragma once

#include <iostream>

#include <poker/hand.hpp>

namespace poker::debug {

inline auto operator<<(std::ostream& os, hand_ranking hr) -> std::ostream& {
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

inline auto make_hand(std::string_view str) noexcept -> hand {
    auto cards = make_cards<7>(str);
    return {cards};
}

} // namespace poker::debug

#pragma once

#include <iostream>

#include <poker/dealer.hpp>

namespace poker::debug {

std::ostream&
operator<<(std::ostream& os, round_of_betting rob)
{
    constexpr const char* rob_names[] = {"preflop", "flop", "turn", "river"};
    return os << rob_names[static_cast<std::size_t>(rob)];
}

} // namespace poker::debug

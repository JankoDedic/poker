#pragma once

#include <poker/card.hpp>

namespace poker {

struct hole_cards {
    card first;
    card second;
};

constexpr auto operator==(const hole_cards& x, const hole_cards& y) noexcept -> bool {
    return x.first == y.first && x.second == y.second;
}

constexpr auto operator!=(const hole_cards& x, const hole_cards& y) noexcept -> bool {
    return !(x == y);
}

} // namespace poker

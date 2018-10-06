#pragma once

namespace poker {

enum class hand_ranking {
    high_card,
    pair,
    two_pair,
    three_of_a_kind,
    straight,
    flush,
    full_house,
    four_of_a_kind,
    straight_flush,
    royal_flush
};

} // namespace poker

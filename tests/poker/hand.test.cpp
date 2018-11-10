#include <doctest/doctest.h>

#include <poker/hand.hpp>

TEST_CASE("high/low hand evaluation") {
    // TODO: test .cards() and .strength()
    const poker::hand hands[] = {
        poker::debug::make_hand("Ac Ac Ac Ac Kc 2c 2c"),
        poker::debug::make_hand("Ac Ac Ac Kc Kc 2c 2c"),
        poker::debug::make_hand("Ac Ac Ac Kc Kc Kc 2c"),
        poker::debug::make_hand("Ac Ac Kc Kc 3c 2c 2c"),
        poker::debug::make_hand("Ac Ac Kc Qc Jc Tc 2c"),
        poker::debug::make_hand("Ac Kc Qc Jc 9c 8c 7c"),
    };
    constexpr poker::hand_ranking hand_rankings[] = {
        poker::hand_ranking::four_of_a_kind,
        poker::hand_ranking::full_house,
        poker::hand_ranking::three_of_a_kind,
        poker::hand_ranking::two_pair,
        poker::hand_ranking::pair,
        poker::hand_ranking::high_card,
    };
    for (auto i = 0; i < std::size(hands); ++i) {
        REQUIRE_EQ(hands[i].ranking(), hand_rankings[i]);
    }
}

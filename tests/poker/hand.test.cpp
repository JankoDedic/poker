#include <doctest/doctest.h>

#include <iostream>
#include <poker/hand.hpp>
#include <poker/debug/card.hpp>
#include <poker/debug/hand.hpp>

TEST_CASE("high/low hand evaluation") {
    // TODO: test .cards() and .strength()
    std::array<poker::card, 7> all_cards[] = {
        poker::debug::make_cards<7>("Ac Ac Ac Ac Kc 2c 2c"),
        poker::debug::make_cards<7>("Ac Ac Ac Kc Kc 2c 2c"),
        poker::debug::make_cards<7>("Ac Ac Ac Kc Kc Kc 2c"),
        poker::debug::make_cards<7>("Ac Ac Kc Kc 3c 2c 2c"),
        poker::debug::make_cards<7>("Ac Ac Kc Qc Jc Tc 2c"),
        poker::debug::make_cards<7>("Ac Kc Qc Jc 9c 8c 7c"),
    };
    const poker::hand hands[] = {
        poker::hand::_high_low_hand_eval(all_cards[0]),
        poker::hand::_high_low_hand_eval(all_cards[1]),
        poker::hand::_high_low_hand_eval(all_cards[2]),
        poker::hand::_high_low_hand_eval(all_cards[3]),
        poker::hand::_high_low_hand_eval(all_cards[4]),
        poker::hand::_high_low_hand_eval(all_cards[5]),
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

TEST_CASE("straight/flush hand evaluation") {
    std::array<poker::card, 7> all_cards[] = {
        poker::debug::make_cards<7>("Ac Qc Tc 9c 7h 2c 3h"),
        poker::debug::make_cards<7>("Ts 9c 8d 7c 6h 4c 5h"),
        poker::debug::make_cards<7>("As 2c 3d 4c 5h Kc Qh"),
        poker::debug::make_cards<7>("Ks Qs Ts Js 9s 8s 7s"),
        poker::debug::make_cards<7>("As Ks Qs Js Ts 8s 7s"),
    };
    const poker::hand hands[] = {
        *poker::hand::_straight_flush_eval(all_cards[0]),
        *poker::hand::_straight_flush_eval(all_cards[1]),
        *poker::hand::_straight_flush_eval(all_cards[2]),
        *poker::hand::_straight_flush_eval(all_cards[3]),
        *poker::hand::_straight_flush_eval(all_cards[4]),
    };
    constexpr poker::hand_ranking hand_rankings[] = {
        poker::hand_ranking::flush,
        poker::hand_ranking::straight,
        poker::hand_ranking::straight,
        poker::hand_ranking::straight_flush,
        poker::hand_ranking::royal_flush,
    };
    for (auto i = 0; i < std::size(hands); ++i) {
        REQUIRE_EQ(hands[i].ranking(), hand_rankings[i]);
    }
}

#include <doctest/doctest.h>

#include <random>

#include <poker/dealer.hpp>

using namespace poker;

TEST_CASE("construction") {
    // auto players = std::vector<player>{player{1}, player{1}, player{1}};
    // auto rng = std::make_unique<std::mt19937>(std::random_device{}());
    // auto dck = deck{*rng};
    // auto cc = community_cards{};
    // auto dlr = dealer{players, players.begin()+1, {10, 20, 0}, dck, cc};
}

TEST_CASE("Starting the hand") {
    const auto b = forced_bets{blinds{25, 50}};
    auto dck = deck{std::default_random_engine{std::random_device{}()}};
    auto cc = community_cards{};

    GIVEN("A hand with two players who can cover their blinds") {
        auto players = seat_array{};
        players.add_player(0, player{100});
        players.add_player(1, player{100});
        auto d = dealer{players, 0, b, dck, cc};

        WHEN("The hand starts") {
            d.start_hand();

            THEN("The button has posted the small blind") {
                REQUIRE_EQ(players[0].bet_size(), 25);
            }

            THEN("The other player has posted the big blind") {
                REQUIRE_EQ(players[1].bet_size(), 50);
            }

            THEN("The action is on the button") {
                /* REQUIRE_EQ(*d.player_to_act(), &players[0]); */
                REQUIRE_EQ(d.player_to_act(), 0);
            }
        }
    }

    GIVEN("A hand with two players who can't cover their blinds") {
        auto players = seat_array{};
        players.add_player(0, player{20});
        players.add_player(1, player{20});
        auto d = dealer{players, 0, b, dck, cc};

        WHEN("The hand starts") {
            d.start_hand();

            THEN("The betting round is not in progress") {
                REQUIRE_FALSE(d.betting_round_in_progress());

                d.end_betting_round();

                REQUIRE_FALSE(d.betting_round_in_progress());
                REQUIRE(d.betting_rounds_completed());
                REQUIRE(d.round_of_betting() == round_of_betting::river);

                d.showdown();

                REQUIRE_FALSE(d.hand_in_progress());
            }
        }
    }

    GIVEN("A hand with more than two players") {
        auto players = seat_array{};
        players.add_player(0, player{100});
        players.add_player(1, player{100});
        players.add_player(2, player{100});
        players.add_player(3, player{100});
        auto d = dealer{players, 0, b, dck, cc};

        WHEN("The hand starts") {
            d.start_hand();

            THEN("The button+1 has posted the small blind") {
                REQUIRE_EQ(players[1].bet_size(), 25);
            }

            THEN("The button+2 has posted the big blind") {
                REQUIRE_EQ(players[2].bet_size(), 50);
            }

            THEN("The action is on the button+3") {
                REQUIRE_EQ(d.player_to_act(), 3);
            }
        }
    }
}

TEST_CASE("Ending the betting round") {
    const auto b = forced_bets{blinds{25, 50}};
    auto dck = deck{std::default_random_engine{std::random_device{}()}};
    auto cc = community_cards{};
    auto players = seat_array{};
    players.add_player(0, player{1000});
    players.add_player(1, player{1000});
    players.add_player(2, player{1000});
    auto d = dealer{players, 0, b, dck, cc};

    GIVEN("There is two or more active players at the end of any betting round except river") {
        d.start_hand();
        d.action_taken(dealer::action::call);
        d.action_taken(dealer::action::call);
        d.action_taken(dealer::action::check);

        REQUIRE_FALSE(d.betting_round_in_progress());
        REQUIRE_GE(d.num_active_players(), 2);
        REQUIRE_NE(d.round_of_betting(), poker::round_of_betting::river);
        REQUIRE_EQ(cc.cards().size(), 0);

        WHEN("The betting round is ended") {
            d.end_betting_round();

            THEN("The next betting round begins") {
                REQUIRE(d.betting_round_in_progress());
                REQUIRE_EQ(d.round_of_betting(), poker::round_of_betting::flop);
                REQUIRE_EQ(cc.cards().size(), 3);
            }
        }
    }

    GIVEN("There is two or more active players at the end of river") {
        d.start_hand();

        // preflop
        d.action_taken(dealer::action::call);
        d.action_taken(dealer::action::call);
        d.action_taken(dealer::action::check);
        d.end_betting_round();

        // flop
        d.action_taken(dealer::action::check);
        d.action_taken(dealer::action::check);
        d.action_taken(dealer::action::check);
        d.end_betting_round();

        // turn
        d.action_taken(dealer::action::check);
        d.action_taken(dealer::action::check);
        d.action_taken(dealer::action::check);
        d.end_betting_round();

        // river
        d.action_taken(dealer::action::check);
        d.action_taken(dealer::action::check);
        d.action_taken(dealer::action::check);
        // not ended yet

        REQUIRE_FALSE(d.betting_round_in_progress());
        REQUIRE_EQ(d.round_of_betting(), poker::round_of_betting::river);
        REQUIRE_EQ(cc.cards().size(), 5);

        WHEN("The betting round is ended") {
            d.end_betting_round();

            REQUIRE_FALSE(d.betting_round_in_progress());
            REQUIRE(d.betting_rounds_completed());
            REQUIRE(d.round_of_betting() == round_of_betting::river);

            d.showdown();

            THEN("The hand is over") {
                REQUIRE_FALSE(d.hand_in_progress());
            }
        }
    }

    GIVEN("There is one or less active players at the end of a betting round and more than one player in all pots") {
        d.start_hand();
        d.action_taken(dealer::action::raise, 1000);
        d.action_taken(dealer::action::call);
        d.action_taken(dealer::action::fold);

        REQUIRE_FALSE(d.betting_round_in_progress());
        REQUIRE_LE(d.num_active_players(), 1);
        REQUIRE_NE(d.round_of_betting(), poker::round_of_betting::river);
        REQUIRE_EQ(cc.cards().size(), 0);

        WHEN("The betting round is ended") {
            d.end_betting_round();
            d.showdown();

            THEN("The hand is over") {
                REQUIRE_FALSE(d.hand_in_progress());
            }
            THEN("The undealt community cards (if any) are dealt") {
                REQUIRE_EQ(cc.cards().size(), 5);
            }
        }
    }

    GIVEN("There is one or less active players at the end of a betting round and a single player in a single pot") {
        d.start_hand();
        d.action_taken(dealer::action::raise, 1000);
        d.action_taken(dealer::action::fold);
        d.action_taken(dealer::action::fold);

        REQUIRE_FALSE(d.betting_round_in_progress());
        REQUIRE_LE(d.num_active_players(), 1);
        REQUIRE_NE(d.round_of_betting(), poker::round_of_betting::river);
        REQUIRE_EQ(cc.cards().size(), 0);

        WHEN("The betting round is ended") {
            d.end_betting_round();
            d.showdown();

            THEN("The hand is over") {
                REQUIRE_FALSE(d.hand_in_progress());
            }
            THEN("The undealt community cards (if any) are not dealt") {
                REQUIRE_EQ(cc.cards().size(), 0);
            }
        }
    }
}

TEST_CASE("flop, someone folded preflop, now others fold, when 1 remains, the hand should be over") {
    // A bug where we pass a container of pointers where some are null to the betting_round => round.
    // round initializes _num_active_players to .size() of the container, instead of counting non-null pointers.
    //
    const auto b = forced_bets{blinds{25, 50}};
    auto dck = deck{std::default_random_engine{std::random_device{}()}};
    auto cc = community_cards{};
    auto players = seat_array{};
    players.add_player(0, player{1000});
    players.add_player(1, player{1000});
    players.add_player(2, player{1000});
    auto d = dealer{players, 0, b, dck, cc};

    d.start_hand();
    d.action_taken(dealer::action::fold);
    d.action_taken(dealer::action::call);
    d.action_taken(dealer::action::check);
    REQUIRE_FALSE(d.betting_round_in_progress());
    d.end_betting_round();

    d.action_taken(dealer::action::fold);
    REQUIRE_FALSE(d.betting_round_in_progress());
}

TEST_CASE("Showdown") {
    SUBCASE("single pot single player") {
        const auto b = forced_bets{blinds{25, 50}};
        auto dck = deck{std::default_random_engine{std::random_device{}()}};
        auto cc = community_cards{};
        auto players = seat_array{};
        players.add_player(0, player{1000});
        players.add_player(1, player{1000});
        players.add_player(2, player{1000});
        auto d = dealer{players, 0, b, dck, cc};

        d.start_hand();
        d.action_taken(dealer::action::raise, 1000);
        d.action_taken(dealer::action::fold);
        d.action_taken(dealer::action::fold);
        d.end_betting_round();
        d.showdown();

        REQUIRE_FALSE(d.hand_in_progress());

        REQUIRE_EQ(players[0].stack(), 1075);
    }

    SUBCASE("multiple pots, multiple winners") {
        const auto b = forced_bets{blinds{25, 50}};
        auto dck = deck{std::default_random_engine{std::random_device{}()}};
        auto cc = community_cards{};
        auto players = seat_array{};
        players.add_player(0, player{300});
        players.add_player(1, player{200});
        players.add_player(2, player{100});
        auto d = dealer{players, 0, b, dck, cc};

        d.start_hand();
        d.action_taken(dealer::action::raise, 300);
        d.action_taken(dealer::action::call);
        d.action_taken(dealer::action::call);
        d.end_betting_round();

        cc = {};
        cc.deal(std::array{
            card{card_rank::A, card_suit::spades},
            card{card_rank::K, card_suit::spades},
            card{card_rank::Q, card_suit::spades},
            card{card_rank::J, card_suit::spades},
            card{card_rank::T, card_suit::spades}
        });
        /* for (auto &p : players) { */
        /*     p.hole_cards = { */
        /*         {card_rank::_2, card_suit::clubs}, */
        /*         {card_rank::_3, card_suit::clubs} */
        /*     }; */
        /* } */

        /* d.showdown(); */

        /* REQUIRE_FALSE(d.hand_in_progress()); */

        /* REQUIRE_EQ(players[0].stack(), 300); */
        /* REQUIRE_EQ(players[1].stack(), 200); */
        /* REQUIRE_EQ(players[2].stack(), 100); */
    }
}

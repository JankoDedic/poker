#include <doctest/doctest.h>

#include <algorithm>
#include <random>

#include <poker/table.hpp>

TEST_CASE("table construction") {
    auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};

    REQUIRE(std::find(t.seats().occupancy().begin(), t.seats().occupancy().end(), true) == t.seats().occupancy().end());
    REQUIRE_EQ(t.forced_bets(), poker::forced_bets{poker::blinds{25, 50}});
    REQUIRE_FALSE(t.hand_in_progress());
}

TEST_CASE("setting forced bets") {
    auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};

    t.set_forced_bets(poker::forced_bets{poker::blinds{100, 200}});

    REQUIRE_EQ(t.forced_bets(), poker::forced_bets{poker::blinds{100, 200}});
}

TEST_CASE("moving the button between hands") {
    auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};

    t.sit_down(2, 2000);
    t.sit_down(3, 2000);
    t.sit_down(4, 2000);
    t.start_hand(std::default_random_engine{std::random_device{}()});
    REQUIRE_EQ(t.button(), 2);
    t.action_taken(poker::action::fold);
    t.action_taken(poker::action::fold);
    t.end_betting_round();
    t.showdown();
    REQUIRE_FALSE(t.hand_in_progress());

    // Start a new hand
    t.start_hand(std::default_random_engine{std::random_device{}()});

    // Button jumped to the next present player
    REQUIRE_EQ(t.button(), 3);
}

TEST_CASE("adding/removing players") {
    GIVEN("a table with no hand in play") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};

        WHEN("a player takes a seat") {
            t.sit_down(7, 1000);

            THEN("that seat is taken") {
                REQUIRE(t.seats().occupancy()[7]);
            }
        }
    }

    GIVEN("a table with one player seated and no hand in play") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};
        t.sit_down(7, 1000);

        WHEN("that player stands up") {
            t.stand_up(7);

            THEN("the seat opens up") {
                REQUIRE_FALSE(t.seats().occupancy()[7]);
            }
        }
    }

    GIVEN("a table with three players active in the hand which is in progress") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};
        t.sit_down(4, 2000);
        t.sit_down(5, 2000);
        t.sit_down(6, 2000);
        t.start_hand(std::default_random_engine{std::random_device{}()});
        REQUIRE(t.betting_round_in_progress());
        REQUIRE_EQ(t.player_to_act(), 4);

        // More than one player remain sitting
        WHEN("one of them stands up") {
            t.stand_up(5);

            THEN("the betting round is still in progress") {
                REQUIRE(t.betting_round_in_progress());
            }
        }

        // One player remains sitting
        WHEN("two of them stand up") {
            t.stand_up(4);
            REQUIRE_EQ(t.player_to_act(), 5);
            t.stand_up(6);

            THEN("the betting round is over") {
                REQUIRE_FALSE(t.betting_round_in_progress());
            }
        }
    }

    GIVEN("a table with a few active players in a hand which is in progress") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};
        t.sit_down(4, 2000);
        t.sit_down(5, 2000);
        t.sit_down(6, 2000);
        t.start_hand(std::default_random_engine{std::random_device{}()});

        // Normal set to fold
        WHEN("a player stands up") {
            t.stand_up(6);

            THEN("his automatic action is set to fold") {
                REQUIRE_EQ(t.automatic_actions()[6], poker::table::automatic_action::fold);
            }
        }

        // Immediate fold
        WHEN("the player to act stands up") {
            REQUIRE_EQ(t.player_to_act(), 4);
            REQUIRE_EQ(t.num_active_players(), 3);
            t.stand_up(4);

            THEN("his action counts as a fold") {
                REQUIRE_EQ(t.player_to_act(), 5);
                REQUIRE_EQ(t.num_active_players(), 2);
            }
        }
    }
}

TEST_CASE("automatic actions") {
    GIVEN("a table") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};

        WHEN("three players sit down and the hand begins") {
            t.sit_down(1, 2000);
            t.sit_down(2, 2000);
            t.sit_down(3, 2000);
            t.start_hand(std::default_random_engine{std::random_device{}()});

            THEN("the legal actions for each player are appropriate") {
                REQUIRE_EQ(t.seats()[1].bet_size(), 0);
                REQUIRE_EQ(t.seats()[2].bet_size(), 25);
                REQUIRE_EQ(t.seats()[3].bet_size(), 50);

                auto legal_automatic_actions = poker::table::automatic_action{};

                legal_automatic_actions = t.legal_automatic_actions(1);
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::fold));
                REQUIRE_FALSE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::check_fold));
                REQUIRE_FALSE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::check));
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::call));
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::call_any));
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::all_in));

                legal_automatic_actions = t.legal_automatic_actions(2);
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::fold));
                REQUIRE_FALSE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::check_fold));
                REQUIRE_FALSE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::check));
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::call));
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::call_any));
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::all_in));

                legal_automatic_actions = t.legal_automatic_actions(3);
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::fold));
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::check_fold));
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::check));
                REQUIRE_FALSE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::call));
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::call_any));
                REQUIRE(static_cast<bool>(legal_automatic_actions & poker::table::automatic_action::all_in));
            }
        }
    }

    GIVEN("a table with a game that has just begun") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};
        t.sit_down(1, 2000);
        t.sit_down(2, 2000);
        t.sit_down(3, 2000);
        t.start_hand(std::default_random_engine{std::random_device{}()});

        WHEN("SB and BB set their automatic actions") {
            t.set_automatic_action(2, poker::table::automatic_action::call);
            t.set_automatic_action(3, poker::table::automatic_action::all_in);

            THEN("the table state reflects that") {
                REQUIRE_EQ(*t.automatic_actions()[2], poker::table::automatic_action::call);
                REQUIRE_EQ(*t.automatic_actions()[3], poker::table::automatic_action::all_in);
            }
        }
    }

    GIVEN("a table with a game that has just begun where SB and BB have set their automatic actions to call/check") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};
        t.sit_down(1, 2000);
        t.sit_down(2, 2000);
        t.sit_down(3, 2000);
        t.start_hand(std::default_random_engine{std::random_device{}()});
        t.set_automatic_action(2, poker::table::automatic_action::call);
        t.set_automatic_action(3, poker::table::automatic_action::check);

        WHEN("the player to act calls") {
            t.action_taken(poker::action::call);

            THEN("the automatic actions play out") {
                REQUIRE_EQ(t.seats()[1].bet_size(), 50);
                REQUIRE_EQ(t.seats()[2].bet_size(), 50);
                REQUIRE_EQ(t.seats()[3].bet_size(), 50);
            }
            THEN("the betting round ends") {
                REQUIRE_FALSE(t.betting_round_in_progress());
            }
        }
    }

    // Checking to see if the automatic action gets cleared.
    GIVEN("a table where a player's automatic action has been taken") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};
        t.sit_down(1, 2000);
        t.sit_down(2, 2000);
        t.sit_down(3, 2000);
        t.start_hand(std::default_random_engine{std::random_device{}()});
        t.set_automatic_action(2, poker::table::automatic_action::call);
        t.action_taken(poker::action::call); // player 1 calls
        REQUIRE_EQ(t.player_to_act(), 3);

        WHEN("action gets back to him") {
            t.action_taken(poker::action::raise, 200);
            t.action_taken(poker::action::call);

            THEN("he is the player to act") {
                REQUIRE(t.betting_round_in_progress());
                REQUIRE_EQ(t.player_to_act(), 2);
            }
        }
    }

    GIVEN("a table where a player's automatic action is set to check_fold") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};
        t.sit_down(1, 2000);
        t.sit_down(2, 2000);
        t.sit_down(3, 2000);
        t.start_hand(std::default_random_engine{std::random_device{}()});
        t.set_automatic_action(3, poker::table::automatic_action::check_fold);

        WHEN("some other player raises") {
            t.action_taken(poker::action::raise, 200);

            THEN("his automatic action falls back to fold") {
                REQUIRE_EQ(*t.automatic_actions()[3], poker::table::automatic_action::fold);
            }
        }

        WHEN("that doesn't happen") {
            t.action_taken(poker::action::call);

            THEN("his automatic action remains the same") {
                REQUIRE_EQ(*t.automatic_actions()[3], poker::table::automatic_action::check_fold);
            }
        }
    }

    GIVEN("a table where a player's automatic action is set to check") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};
        t.sit_down(1, 2000);
        t.sit_down(2, 2000);
        t.sit_down(3, 2000);
        t.start_hand(std::default_random_engine{std::random_device{}()});
        t.set_automatic_action(3, poker::table::automatic_action::check);

        WHEN("some other player raises") {
            t.action_taken(poker::action::raise, 200);

            THEN("his automatic action gets removed") {
                REQUIRE_EQ(t.automatic_actions()[3], std::nullopt);
            }
        }

        WHEN("that doesn't happen") {
            t.action_taken(poker::action::call);

            THEN("his automatic action remains the same") {
                REQUIRE_EQ(*t.automatic_actions()[3], poker::table::automatic_action::check);
            }
        }
    }

    GIVEN("a table where a player's automatic action is set to call_any") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};
        t.sit_down(1, 2000);
        t.sit_down(2, 2000);
        t.sit_down(3, 2000);
        t.start_hand(std::default_random_engine{std::random_device{}()});
        t.set_automatic_action(3, poker::table::automatic_action::call_any);

        WHEN("some other player goes all-in") {
            t.action_taken(poker::action::raise, 2000);

            // All doubt has been cleared, it's not "call any", it's "call this exact amount".
            THEN("his automatic action falls back to call") {
                REQUIRE_EQ(*t.automatic_actions()[3], poker::table::automatic_action::call);
            }
        }

        WHEN("that doesn't happen") {
            t.action_taken(poker::action::call);

            THEN("his automatic action remains the same") {
                REQUIRE_EQ(*t.automatic_actions()[3], poker::table::automatic_action::call_any);
            }
        }
    }

    GIVEN("a table where a hand has just begun") {
        auto t = poker::table{poker::forced_bets{poker::blinds{25, 50}}};
        t.sit_down(1, 2000);
        t.sit_down(2, 2000);
        t.sit_down(3, 2000);
        t.start_hand(std::default_random_engine{std::random_device{}()});

        WHEN("a player sets his automatic action to fold and it gets triggered") {
            t.set_automatic_action(2, poker::table::automatic_action::fold);
            t.action_taken(poker::action::call);

            THEN("he folded") {
                REQUIRE_FALSE(t.hand_players().filter()[2]);
            }
        }

        WHEN("a player sets his automatic action to check_fold and it gets triggered") {
            /* REQUIRE_EQ(t.seats()[3]->bet_size(), 50); */
            REQUIRE_EQ(t.seats()[3].bet_size(), 50);
            t.set_automatic_action(3, poker::table::automatic_action::check_fold);
            t.action_taken(poker::action::call);
            t.action_taken(poker::action::call);

            THEN("he checked") {
                REQUIRE_FALSE(t.betting_round_in_progress());
                REQUIRE_EQ(t.seats()[3].bet_size(), 50);
            }
        }

        WHEN("a player sets his automatic action to check and it gets triggered") {
            REQUIRE_EQ(t.seats()[3].bet_size(), 50);
            t.set_automatic_action(3, poker::table::automatic_action::check);
            t.action_taken(poker::action::call);
            t.action_taken(poker::action::call);

            THEN("he checked") {
                REQUIRE_FALSE(t.betting_round_in_progress());
                REQUIRE_EQ(t.seats()[3].bet_size(), 50);
            }
        }

        WHEN("a player sets his automatic action to call and it gets triggered") {
            REQUIRE_EQ(t.seats()[2].bet_size(), 25);
            t.set_automatic_action(2, poker::table::automatic_action::call);
            t.action_taken(poker::action::call);

            THEN("he called") {
                REQUIRE_EQ(t.player_to_act(), 3);
                REQUIRE_EQ(t.seats()[2].bet_size(), 50);
            }
        }

        WHEN("a player sets his automatic action to call_any and it gets triggered") {
            REQUIRE_EQ(t.seats()[2].bet_size(), 25);
            t.set_automatic_action(2, poker::table::automatic_action::call_any);
            t.action_taken(poker::action::call);

            THEN("he called (any)") {
                REQUIRE_EQ(t.player_to_act(), 3);
                REQUIRE_EQ(t.seats()[2].bet_size(), 50);
            }
        }

        WHEN("a player sets his automatic action to all_in and it gets triggered") {
            REQUIRE_EQ(t.player_to_act(), 1);
            REQUIRE_EQ(t.seats()[2].bet_size(), 25);
            t.set_automatic_action(2, poker::table::automatic_action::all_in);
            t.action_taken(poker::action::call);

            THEN("he called (any)") {
                REQUIRE_EQ(t.player_to_act(), 3);
                REQUIRE_EQ(t.seats()[2].bet_size(), 2000);
            }
        }
    }
}

// OLD TESTS

TEST_CASE("When second to last player stands up, the hand ends") {
    using namespace poker;

    auto t = table{forced_bets{blinds{25, 50}}};
    t.sit_down(0, 1000);
    t.sit_down(1, 1000);
    t.sit_down(2, 1000);

    t.start_hand(std::default_random_engine{std::random_device{}()});
    REQUIRE_EQ(t.player_to_act(), 0);

    REQUIRE_EQ(t.seats()[0].bet_size(), 0);
    REQUIRE_EQ(t.seats()[1].bet_size(), 25);
    REQUIRE_EQ(t.seats()[2].bet_size(), 50);
    REQUIRE_EQ(0, t.button());

    t.stand_up(1);
    t.stand_up(2);
    t.sit_down(1, 1000);
    t.sit_down(2, 1000);
    REQUIRE_FALSE(t.betting_round_in_progress());
    t.end_betting_round();

    REQUIRE_EQ(t.seats()[0].stack(), 950);

    t.showdown();
    REQUIRE_FALSE(t.hand_in_progress());

    REQUIRE_EQ(t.seats()[0].stack(), 1075);

    t.start_hand(std::default_random_engine{std::random_device{}()});
    REQUIRE_EQ(1, t.button());
    t.stand_up(2);
    t.stand_up(0);
    REQUIRE_FALSE(t.betting_round_in_progress());
    REQUIRE(t.hand_in_progress());
    t.end_betting_round();
    REQUIRE(t.hand_in_progress());
    t.showdown();
    REQUIRE_FALSE(t.hand_in_progress());
}

TEST_CASE("testing the special case") {
    using namespace poker;

    auto t = table{forced_bets{blinds{25}}};
    t.sit_down(0, 1000);
    t.sit_down(1, 1000);
    t.sit_down(2, 1000);
    t.stand_up(2);
    t.sit_down(2, 1000);
    t.start_hand(std::default_random_engine{std::random_device{}()});
    t.set_automatic_action(1, table::automatic_action::call_any);
    t.set_automatic_action(2, table::automatic_action::call_any);
    t.action_taken(action::call);
    REQUIRE_FALSE(t.betting_round_in_progress());
}

TEST_CASE("Community cards get reset when a new hand begins") {
    auto t = poker::table{poker::forced_bets{poker::blinds{25}}};
    t.sit_down(0, 1000);
    t.sit_down(1, 1000);
    t.start_hand(std::default_random_engine{std::random_device{}()});
    t.action_taken(poker::dealer::action::call);
    t.action_taken(poker::dealer::action::check);
    t.end_betting_round();
    t.action_taken(poker::dealer::action::fold);
    t.end_betting_round();
    REQUIRE(t.betting_rounds_completed());
    t.showdown();
    t.start_hand(std::default_random_engine{std::random_device{}()});
    REQUIRE_EQ(t.community_cards().cards().size(), 0);
}

TEST_CASE("Setting the button manually works on the first, as well as the subsequent hands") {
    auto t = poker::table{poker::forced_bets{poker::blinds{25}}};
    t.sit_down(0, 1000);
    t.sit_down(3, 1000);
    t.sit_down(5, 1000);
    t.sit_down(8, 1000);

    // First hand
    t.start_hand(std::default_random_engine{std::random_device{}()}, 8);
    REQUIRE_EQ(t.button(), 8);
    t.action_taken(poker::dealer::action::fold);
    t.action_taken(poker::dealer::action::fold);
    t.action_taken(poker::dealer::action::fold);
    t.end_betting_round();
    t.showdown();

    // Second hand
    t.start_hand(std::default_random_engine{std::random_device{}()}, 5);
    REQUIRE_EQ(t.button(), 5);
}

TEST_CASE("Buttons wraps around correctly when moved from the last position") {
    auto t = poker::table{poker::forced_bets{poker::blinds{25}}};
    t.sit_down(0, 1000);
    t.sit_down(3, 1000);
    t.sit_down(5, 1000);
    t.sit_down(8, 1000);

    // First hand
    t.start_hand(std::default_random_engine{std::random_device{}()}, 8);
    REQUIRE_EQ(t.button(), 8);
    t.action_taken(poker::dealer::action::fold);
    t.action_taken(poker::dealer::action::fold);
    t.action_taken(poker::dealer::action::fold);
    t.end_betting_round();
    t.showdown();

    // Second hand
    t.start_hand(std::default_random_engine{std::random_device{}()});
    REQUIRE_EQ(t.button(), 0);
}

TEST_CASE("No crash when the player to act stands up with one player remaining") {
    // This was caused by act_passively() being called in the case
    // of player_to_act standing up. Since the action is taken directly,
    // the hand ends naturally by itself and no further actions can be taken.
    auto t = poker::table{poker::forced_bets{poker::blinds{25}}};
    t.sit_down(1, 1000);
    t.sit_down(8, 1000);
    t.start_hand(std::default_random_engine{std::random_device{}()});
    t.stand_up(1);
}

TEST_CASE("Betting round ends when only a single active player remains") {
    // Correct behavior after action_taken.
    {
        auto t = poker::table{poker::forced_bets{poker::blinds{25}}};
        t.sit_down(1, 1000);
        t.sit_down(5, 1000);
        t.sit_down(8, 1000);
        t.start_hand(std::default_random_engine{std::random_device{}()});
        REQUIRE_EQ(t.player_to_act(), 1);
        t.stand_up(8);
        t.action_taken(poker::dealer::action::fold);
        REQUIRE_FALSE(t.betting_round_in_progress());
    }

    // Correct behavior after stand_up.
    {
        auto t = poker::table{poker::forced_bets{poker::blinds{25}}};
        t.sit_down(1, 1000);
        t.sit_down(5, 1000);
        t.sit_down(8, 1000);
        t.start_hand(std::default_random_engine{std::random_device{}()});
        REQUIRE_EQ(t.player_to_act(), 1);
        t.action_taken(poker::dealer::action::fold);
        t.stand_up(8);
        REQUIRE_FALSE(t.betting_round_in_progress());
    }
}

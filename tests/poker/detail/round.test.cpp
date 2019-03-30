#include <doctest/doctest.h>

#include "poker/detail/round.hpp"

using namespace poker;
using namespace poker::detail;
using player_container = round::player_container;

TEST_CASE("two leave, one of which is contesting do not result in round being over") {
    constexpr auto some_constant = 42;
    player players[] = {player{some_constant}, player{some_constant}, player{some_constant}};
    auto player_pointers = player_container{&players[0], &players[1], &players[2]};
    auto r = poker::detail::round{player_pointers, player_pointers.begin()}; // round is ambigous for some reason here

    r.action_taken(round::action::aggressive | round::action::leave);
    r.action_taken(round::action::passive | round::action::leave);
    REQUIRE(r.in_progress());
}

TEST_CASE("round construction") {
    constexpr auto some_constant = 42;
    player players[] = {player{some_constant}, player{some_constant}, player{some_constant}};
    auto player_pointers = player_container{&players[0], &players[1], &players[2]};
    auto r = poker::detail::round{player_pointers, player_pointers.begin()};

    REQUIRE(r.in_progress());
    REQUIRE_EQ(r.player_to_act(), r.last_aggressive_actor());
    REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
    REQUIRE_EQ(r.num_active_players(), 3);
}

SCENARIO("there are only 2 players in the round") {
    constexpr auto some_constant = 42;
    player players[] = {player{some_constant}, player{some_constant}};
    auto player_pointers = player_container{&players[0], &players[1]};
    auto r = poker::detail::round{player_pointers, player_pointers.begin()};

    GIVEN("there was no action in the round yet") {
        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
        REQUIRE(r.in_progress());
        REQUIRE_EQ(r.num_active_players(), 2);

        WHEN("the first player acts aggressively") {
            r.action_taken(round::action::aggressive);

            THEN("the last aggressive actor remains unchanged") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
            }

            THEN("the second player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there are still 2 active players") {
                REQUIRE_EQ(r.num_active_players(), 2);
            }
        }

        WHEN("the first player acts aggressively and leaves") {
            r.action_taken(round::action::aggressive | round::action::leave);

            THEN("the last aggressive actor remains unchanged") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
            }

            THEN("the second player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there is 1 active player") {
                REQUIRE_EQ(r.num_active_players(), 1);
            }
        }

        WHEN("the first player acts passively") {
            r.action_taken(round::action::passive);

            THEN("the last aggressive actor remains unchanged") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
            }

            THEN("the second player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there are still 2 active players") {
                REQUIRE_EQ(r.num_active_players(), 2);
            }
        }

        WHEN("the first player acts passively and leaves") {
            r.action_taken(round::action::passive | round::action::leave);

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }
        }

        WHEN("the first player leaves") {
            r.action_taken(round::action::leave);

            THEN("the round is over") {
                REQUIRE(!r.in_progress());
            }
        }
    }

    GIVEN("the next player is the last aggressive actor") {
        r.action_taken(round::action::aggressive);

        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
        REQUIRE(r.in_progress());
        REQUIRE_EQ(r.num_active_players(), 2);

        WHEN("the player to act acts aggressively") {
            r.action_taken(round::action::aggressive);

            THEN("the player to act becomes the last aggressive actor") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 1);
            }

            THEN("the last aggressive actor becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there are still 2 active players") {
                REQUIRE_EQ(r.num_active_players(), 2);
            }
        }

        WHEN("the player to act acts aggressively and leaves") {
            r.action_taken(round::action::aggressive | round::action::leave);

            THEN("the player to act becomes the last aggressive actor") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 1);
            }

            THEN("the last aggressive actor becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there is 1 active player") {
                REQUIRE_EQ(r.num_active_players(), 1);
            }
        }

        WHEN("the current player acts passively") {
            r.action_taken(round::action::passive);

            THEN("the round is over") {
                REQUIRE(!r.in_progress());
            }
        }

        WHEN("the player to act acts passively and leaves") {
            r.action_taken(round::action::passive | round::action::leave);

            THEN("the round is over") {
                REQUIRE(!r.in_progress());
            }
        }

        WHEN("the player to act leaves") {
            r.action_taken(round::action::leave);

            THEN("the round is over") {
                REQUIRE(!r.in_progress());
            }
        }
    }
}

SCENARIO("there are more than 2 players in the round") {
    constexpr auto some_constant = 42;
    player players[] = {player{some_constant}, player{some_constant}, player{some_constant}};
    auto player_pointers = player_container{&players[0], &players[1], &players[2]};
    auto r = poker::detail::round{player_pointers, player_pointers.begin()};

    const auto initial_num_active_players = r.num_active_players();

    GIVEN("there was no action in the round yet") {
        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
        REQUIRE(r.in_progress());
        REQUIRE_EQ(r.num_active_players(), 3);

        WHEN("the player to act acts aggressively") {
            r.action_taken(round::action::aggressive);

            THEN("the last aggressive actor remains unchanged") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("the number of active players remains unchanged") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players);
            }
        }

        WHEN("the player to act acts aggressively and leaves") {
            r.action_taken(round::action::aggressive | round::action::leave);

            THEN("the last aggressive actor remains unchanged") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there is one less active player") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
            }
        }

        WHEN("the player to act acts passively") {
            r.action_taken(round::action::passive);

            THEN("the last aggressive actor remains unchanged") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("the number of active players remains unchanged") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players);
            }
        }

        WHEN("the player to act acts passively and leaves") {
            r.action_taken(round::action::passive | round::action::leave);

            THEN("the last aggressive actor remains unchanged") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there is one less active player") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
            }
        }

        WHEN("the player to act leaves") {
            r.action_taken(round::action::leave);

            THEN("the last aggressive actor remains unchanged") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there is one less active player") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
            }
        }
    }

    GIVEN("the next player is the last aggressive actor") {
        r.action_taken(round::action::aggressive);
        r.action_taken(round::action::passive);

        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
        REQUIRE(r.in_progress());
        REQUIRE_EQ(r.num_active_players(), 3);

        WHEN("the player to act acts aggressively") {
            r.action_taken(round::action::aggressive);

            THEN("the player to act becomes the last aggressive actor") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 2);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("the number of active players remains unchanged") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players);
            }
        }

        WHEN("the player to act acts aggressively and leaves") {
            r.action_taken(round::action::aggressive | round::action::leave);

            THEN("the player to act becomes the last aggressive actor") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 2);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there is one less active player") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
            }
        }

        WHEN("the player to act acts passively") {
            r.action_taken(round::action::passive);

            THEN("the round is over") {
                REQUIRE(!r.in_progress());
            }
        }

        WHEN("the player to act acts passively and leaves") {
            r.action_taken(round::action::passive | round::action::leave);

            THEN("the round is over") {
                REQUIRE(!r.in_progress());
            }
        }

        WHEN("the player to act leaves") {
            r.action_taken(round::action::leave);

            THEN("the round is over") {
                REQUIRE(!r.in_progress());
            }
        }
    }

    GIVEN("the next player is not the last aggressive actor") {
        r.action_taken(round::action::aggressive);

        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
        REQUIRE(r.in_progress());
        REQUIRE_EQ(r.num_active_players(), 3);

        WHEN("the player to act acts aggressively") {
            r.action_taken(round::action::aggressive);

            THEN("the player to act becomes the last aggressive actor") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 1);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("the number of active players remains unchanged") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players);
            }
        }

        WHEN("the player to act acts aggressively and leaves") {
            r.action_taken(round::action::aggressive | round::action::leave);

            THEN("the player to act becomes the last aggressive actor") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 1);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there is one less active player") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
            }
        }

        WHEN("the player to act acts passively") {
            r.action_taken(round::action::passive);

            THEN("the last aggressive actor remains unchanged") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("the number of active players remains unchanged") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players);
            }
        }

        WHEN("the player to act acts passively and leaves") {
            r.action_taken(round::action::passive | round::action::leave);

            THEN("the last aggressive actor remains unchanged") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there is one less active player") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
            }
        }

        WHEN("the player to act leaves") {
            r.action_taken(round::action::leave);

            THEN("the last aggressive actor remains unchanged") {
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
            }

            THEN("the next player becomes the player to act") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
            }

            THEN("the round is not over") {
                REQUIRE(r.in_progress());
            }

            THEN("there is one less active player") {
                REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
            }
        }
    }
}

SCENARIO("there are 3 players and the first one acts first") {
    player ps[] = {player{300}, player{400}, player{500}};
    auto players = player_container{&ps[0], &ps[1], &ps[2]};
    auto current = players.begin();

    // Test for first action (_is_next_player_contested) exception.
    GIVEN("a round") {
        auto r = poker::detail::round{players, current};
        REQUIRE(r.in_progress());
    }

    // Test for round end by _num_active_players.
    GIVEN("a round") {
        auto r = poker::detail::round{players, current};

        WHEN("the first two players leave") {
            r.action_taken(round::action::leave);
            r.action_taken(round::action::leave);

            THEN("the round is over") {
                REQUIRE(!r.in_progress());
                // Round should end because of _num_active_players being 1
                REQUIRE_EQ(r.num_active_players(), 1);
            }
        }
    }

    // Test for round end by action reaching _last_aggressive_actor.
    GIVEN("a round") {
        auto r = poker::detail::round{players, current};

        WHEN("the first player leaves, and the other two act passively") {
            r.action_taken(round::action::leave);
            r.action_taken(round::action::passive);
            r.action_taken(round::action::passive);

            THEN("the round is over") {
                REQUIRE(!r.in_progress());
                // Round should end because the _player_to_act is _last_aggressive_actor
                // NOTE: We cannot do this because access to player_to_act() is disallowed.
                // We could allow it and make it "UB". We could also not test it (because it's internals).
                /* REQUIRE_EQ(r.player_to_act(), r.last_aggressive_actor()); */
            }
        }
    }
}

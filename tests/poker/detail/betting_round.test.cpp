#include <doctest/doctest.h>

#include "poker/detail/betting_round.hpp"

using namespace poker;
using namespace poker::detail;

TEST_CASE("testing valid actions") {
    GIVEN("a betting round") {
        auto players = seat_array{};
        players.add_player(0, player{1});
        players.add_player(1, player{1});
        players.add_player(2, player{1});
        auto r = betting_round{players, 0, 50};
        REQUIRE_EQ(r.player_to_act(), 0);
        REQUIRE_EQ(r.biggest_bet(), 50);
        REQUIRE_EQ(r.min_raise(), 50);

        WHEN("the player has less chips than the biggest bet") {
            players[0] = player{25};
            REQUIRE_LT(players[0].total_chips(), r.biggest_bet());

            THEN("he cannot raise") {
                const auto actions = r.legal_actions();
                REQUIRE_FALSE(actions.can_raise);
            }
        }

        WHEN("the player has amount of chips equal to the biggest bet") {
            players[0] = player{50};
            REQUIRE_EQ(players[0].total_chips(), r.biggest_bet());

            THEN("he cannot raise") {
                const auto actions = r.legal_actions();
                REQUIRE_FALSE(actions.can_raise);
            }
        }

        WHEN("the player has more chips than the biggest bet and less than minimum re-raise bet") {
            players[0] = player{75};
            REQUIRE_GT(players[0].total_chips(), r.biggest_bet());
            REQUIRE_LT(players[0].total_chips(), r.biggest_bet() + r.min_raise());

            THEN("he can raise, but only his entire stack") {
                const auto actions = r.legal_actions();
                REQUIRE(actions.can_raise);
                REQUIRE_EQ(actions.chip_range.min, players[0].total_chips());
                REQUIRE_EQ(actions.chip_range.max, players[0].total_chips());
            }
        }

        WHEN("the player has amount of chips equal to the minimum re-raise bet") {
            players[0] = player{100};
            REQUIRE_EQ(players[0].total_chips(), r.biggest_bet() + r.min_raise());

            THEN("he can raise, but only his entire stack") {
                const auto actions = r.legal_actions();
                REQUIRE(actions.can_raise);
                REQUIRE_EQ(actions.chip_range.min, players[0].total_chips());
                REQUIRE_EQ(actions.chip_range.max, players[0].total_chips());
            }
        }

        WHEN("the player has more chips than the minimum re-raise bet") {
            players[0] = player{150};
            REQUIRE_GT(players[0].total_chips(), r.biggest_bet() + r.min_raise());

            THEN("he can raise any amount ranging from min re-raise to his entire stack") {
                const auto actions = r.legal_actions();
                REQUIRE(actions.can_raise);
                REQUIRE_EQ(actions.chip_range.min, r.biggest_bet() + r.min_raise());
                REQUIRE_EQ(actions.chip_range.max, players[0].total_chips());
            }
        }
    }
}

TEST_CASE("betting round actions map to round actions properly") {
    GIVEN("a betting round") {
        auto players = seat_array{};
        players.add_player(0, player{1000});
        players.add_player(1, player{1000});
        players.add_player(2, player{1000});
        auto r = poker::detail::round{players.occupancy(), 0};
        auto br = betting_round{players, 0, 50};
        REQUIRE_EQ(r, br._round);
        REQUIRE_EQ(br.player_to_act(), 0);

        WHEN("a player raises for less than his entire stack") {
            br.action_taken(betting_round::action::raise, 200);
            REQUIRE_GT(players[0].stack(), 0);

            THEN("he made an aggressive action") {
                r.action_taken(round::action::aggressive);
                REQUIRE_EQ(r, br._round);
            }
        }

        WHEN("a player raises his entire stack") {
            br.action_taken(betting_round::action::raise, 1000);
            REQUIRE_EQ(players[0].stack(), 0);

            THEN("he made an aggressive action and left the round") {
                r.action_taken(round::action::aggressive | round::action::leave);
                REQUIRE_EQ(r, br._round);
            }
        }

        WHEN("a player matches for less than his entire stack") {
            br.action_taken(betting_round::action::match, 500);
            REQUIRE_GT(players[0].stack(), 0);

            THEN("he made a passive action") {
                r.action_taken(round::action::passive);
                REQUIRE_EQ(r, br._round);
            }
        }

        WHEN("a player matches for his entire stack") {
            players[0] = player{50};
            br.action_taken(betting_round::action::match);
            REQUIRE_EQ(players[0].stack(), 0);

            THEN("he made a passive action and left the round") {
                r.action_taken(round::action::passive | round::action::leave);
                REQUIRE_EQ(r, br._round);
            }
        }

        WHEN("a player leaves") {
            br.action_taken(betting_round::action::leave);

            THEN("he left the round") {
                r.action_taken(round::action::leave);
                REQUIRE_EQ(r, br._round);
            }
        }
    }
}

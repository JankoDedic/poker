#pragma once

#include "poker/detail/round.hpp"

namespace poker {

struct chip_range {
    chips min;
    chips max;

    auto contains(chips amount) const noexcept -> bool {
        return min <= amount && amount <= max;
    }
};

} // namespace poker

namespace poker::detail {

class betting_round {
    static constexpr auto max_players = 9; // for the time being
    using player_container = std::array<player *, max_players>;

    round _round;
    chips _biggest_bet = 0;
    chips _min_raise = 0;

    auto is_raise_valid(chips bet) const noexcept -> bool {
        const auto &player = **_round.player_to_act();
        const auto player_chips = player.stack() + player.bet_size();
        const auto min_bet = _biggest_bet + _min_raise;
        if (player_chips > _biggest_bet && player_chips < min_bet) return bet == player_chips;
        else return bet >= min_bet && bet <= player_chips;
    }

public:
    betting_round() = default;

    betting_round(const player_container &players, player_container::const_iterator current, chips min_raise) noexcept
        : _round(players, current)
        , _biggest_bet(min_raise)
        , _min_raise(min_raise)
    {
    }

    auto over()               const noexcept -> bool                             { return _round.over();               }
    auto player_to_act()      const noexcept -> player_container::const_iterator { return _round.player_to_act();      }
    auto biggest_bet()        const noexcept -> chips                            { return _biggest_bet;                }
    auto min_raise()          const noexcept -> chips                            { return _min_raise;                  }
    auto players()            const noexcept -> const player_container &         { return _round.players();            }
    auto num_active_players() const noexcept -> std::size_t                      { return _round.num_active_players(); }

    enum class action { leave, match, raise };

    struct action_range {
        bool can_raise;
        poker::chip_range chip_range = {0, 0};
    };

    auto legal_actions() const noexcept -> action_range {
        // A player can raise if his stack+bet_size is greater than _biggest_bet
        const auto &player = **_round.player_to_act();
        const auto player_chips = player.total_chips();
        const auto can_raise = player_chips > _biggest_bet;
        if (can_raise) {
            const auto min_bet = _biggest_bet + _min_raise;
            const auto raise_range = chip_range{std::min(min_bet, player_chips), player_chips};
            return action_range{can_raise, raise_range};
        } else {
            return action_range{can_raise};
        }
    }

    TEST_CASE_CLASS("testing valid actions") {
        GIVEN("a betting round") {
            player players[] = {player{1}, player{1}, player{1}};
            auto player_pointers = player_container{&players[0], &players[1], &players[2]};
            auto r = betting_round{player_pointers, player_pointers.begin(), 50};
            REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
            REQUIRE_EQ(r.biggest_bet(), 50);
            REQUIRE_EQ(r.min_raise(), 50);

            WHEN("the player has less chips than the biggest bet") {
                players[0] = player{25};
                REQUIRE_LT(players[0].total_chips(), r.biggest_bet());

                THEN("he cannot raise") {
                    const auto actions = r.legal_actions();
                    REQUIRE(!actions.can_raise);
                }
            }

            WHEN("the player has amount of chips equal to the biggest bet") {
                players[0] = player{50};
                REQUIRE_EQ(players[0].total_chips(), r.biggest_bet());

                THEN("he cannot raise") {
                    const auto actions = r.legal_actions();
                    REQUIRE(!actions.can_raise);
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

    void action_taken(action a, chips bet = 0) noexcept {
        // chips bet is ignored when not needed
        auto &player = **_round.player_to_act();
        if (a == action::raise) {
            assert(is_raise_valid(bet));
            player.bet(bet);
            _min_raise = bet - _biggest_bet;
            _biggest_bet = bet;
            auto action_flag = round::action::aggressive;
            if (player.stack() == 0) action_flag |= round::action::leave;
            _round.action_taken(action_flag);
        } else if (a == action::match) {
            player.bet(std::min(_biggest_bet, player.total_chips()));
            auto action_flag = round::action::passive;
            if (player.stack() == 0) action_flag |= round::action::leave;
            _round.action_taken(action_flag);
        } else {
            assert(a == action::leave);
            _round.action_taken(round::action::leave);
        }
    }

    TEST_CASE_CLASS("betting round actions map to round actions properly") {
        GIVEN("a betting round") {
            player players[] = {player{1000}, player{1000}, player{1000}};
            auto player_pointers = player_container{&players[0], &players[1], &players[2]};
            auto r = round{player_pointers, player_pointers.begin()};
            auto br = betting_round{player_pointers, player_pointers.begin(), 50};
            REQUIRE_EQ(r, br._round);
            REQUIRE_EQ(*br.player_to_act(), &players[0]);

            WHEN("a player raises for less than his entire stack") {
                br.action_taken(action::raise, 200);
                REQUIRE_GT(players[0].stack(), 0);

                THEN("he made an aggressive action") {
                    r.action_taken(round::action::aggressive);
                    REQUIRE_EQ(r, br._round);
                }
            }

            WHEN("a player raises his entire stack") {
                br.action_taken(action::raise, 1000);
                REQUIRE_EQ(players[0].stack(), 0);

                THEN("he made an aggressive action and left the round") {
                    r.action_taken(round::action::aggressive | round::action::leave);
                    REQUIRE_EQ(r, br._round);
                }
            }

            WHEN("a player matches for less than his entire stack") {
                br.action_taken(action::match, 500);
                REQUIRE_GT(players[0].stack(), 0);

                THEN("he made a passive action") {
                    r.action_taken(round::action::passive);
                    REQUIRE_EQ(r, br._round);
                }
            }

            WHEN("a player matches for his entire stack") {
                players[0] = player{50};
                br.action_taken(action::match);
                REQUIRE_EQ(players[0].stack(), 0);

                THEN("he made a passive action and left the round") {
                    r.action_taken(round::action::passive | round::action::leave);
                    REQUIRE_EQ(r, br._round);
                }
            }

            WHEN("a player leaves") {
                br.action_taken(action::leave);

                THEN("he left the round") {
                    r.action_taken(round::action::leave);
                    REQUIRE_EQ(r, br._round);
                }
            }
        }
    }
};

} // namespace poker::detail

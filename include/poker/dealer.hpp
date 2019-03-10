#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <new>
#include <random>

#include <doctest/doctest.h>

#include <poker/deck.hpp>
#include <poker/player.hpp>
#include <poker/hand.hpp>
#include <poker/community_cards.hpp>
#include "poker/detail/utility.hpp"

namespace poker {

#define DEFINE_FRIEND_CONSTEXPR_FLAG_OPERATIONS(Type)                         \
    friend constexpr auto operator|(Type x, Type y) noexcept -> Type {        \
        return static_cast<Type>(to_underlying(x) | to_underlying(y));        \
    }                                                                         \
                                                                              \
    friend constexpr auto operator&(Type x, Type y) noexcept -> Type {        \
        return static_cast<Type>(to_underlying(x) & to_underlying(y));        \
    }                                                                         \
                                                                              \
    friend constexpr auto operator|=(Type &x, Type y) noexcept -> Type & {    \
        return x = x | y;                                                     \
    }                                                                         \
                                                                              \
    friend constexpr auto operator&=(Type &x, Type y) noexcept -> Type & {    \
        return x = x & y;                                                     \
    }

struct blinds {
    chips small;
    chips big = 2*small;
    chips ante = 0;
};

class dealer {
    static constexpr auto max_players = 9; // for the time being
    using player_container = std::array<player *, max_players>;

    class round {
        player_container                 _players = {};
        player_container::iterator       _player_to_act;
        player_container::const_iterator _last_aggressive_actor;
        bool                             _contested = false;      // passive or aggressive action was taken this round
        bool                             _first_action = true;
        std::size_t                      _num_active_players = 0;

        void increment_player() noexcept {
            do {
                ++_player_to_act;
                if (_player_to_act == _players.end()) _player_to_act = _players.begin();
                if (_player_to_act == _last_aggressive_actor) break;
            } while (*_player_to_act == nullptr);
        }

    public:
        round() /*noexcept*/ = default;

        round(const player_container &players, player_container::const_iterator current) noexcept
            : _players(players)
            , _player_to_act(_players.begin() + std::distance(players.begin(), current))
            , _last_aggressive_actor(_player_to_act)
            , _num_active_players(std::count_if(begin(_players), end(_players), [] (auto *p) { return p != nullptr; }))
        {
        }

        round(const round &) = delete;
        auto operator=(const round &) -> round & = delete;

        round(round &&) = delete;
        auto operator=(round &&) -> round & = delete;

        auto players() const noexcept -> const player_container & {
            return _players;
        }

        auto player_to_act() const noexcept -> player_container::const_iterator {
            assert(!over());
            return _player_to_act;
        }

        auto last_aggressive_actor() const noexcept -> player_container::const_iterator {
            return _last_aggressive_actor;
        }

        auto num_active_players() const noexcept -> std::size_t {
            return _num_active_players;
        }

        auto over() const -> bool {
            return (!_contested && _num_active_players <= 1) || (!_first_action && _player_to_act == _last_aggressive_actor);
        }

        enum class action {
            leave      = 01,
            passive    = 02,
            aggressive = 04
        };
        DEFINE_FRIEND_CONSTEXPR_FLAG_OPERATIONS(action)

        void action_taken(action a) noexcept {
            assert(!over());
            if (_first_action) _first_action = false;
            assert(!(bool(a & action::passive) && bool(a & action::aggressive)));
            // Implication: if there is aggressive action => the next player is contested
            if (bool(a & action::aggressive)) {
                _last_aggressive_actor = _player_to_act;
                _contested = true;
            } else if (bool(a & action::passive)) {
                _contested = true;
            }
            if (bool(a & action::leave)) {
                *_player_to_act = nullptr;
                --_num_active_players;
            }
            increment_player();
        }

        TEST_CASE_CLASS("two leave, one of which is contesting do not result in round being over") {
            constexpr auto some_constant = 42;
            player players[] = {player{some_constant}, player{some_constant}, player{some_constant}};
            auto player_pointers = player_container{&players[0], &players[1], &players[2]};
            auto r = round{player_pointers, player_pointers.begin()};

            r.action_taken(action::aggressive | action::leave);
            r.action_taken(action::passive | action::leave);
            REQUIRE_FALSE(r.over());
        }

        // FIXME: c'mon
        friend auto operator==(const round &x, const round &y) noexcept -> bool {
            return x._players == y._players
                && (x._player_to_act - x._players.begin()) == (y._player_to_act - y._players.begin())
                && (x._last_aggressive_actor - x._players.begin()) == (y._last_aggressive_actor - y._players.begin())
                && x._contested == y._contested
                && x._num_active_players == y._num_active_players;
        }

        TEST_CASE_CLASS("round construction") {
            constexpr auto some_constant = 42;
            player players[] = {player{some_constant}, player{some_constant}, player{some_constant}};
            auto player_pointers = player_container{&players[0], &players[1], &players[2]};
            auto r = round{player_pointers, player_pointers.begin()};

            REQUIRE(!r.over());
            REQUIRE_EQ(r.player_to_act(), r.last_aggressive_actor());
            REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
            REQUIRE_EQ(r.num_active_players(), 3);
        }

        SCENARIO_CLASS("there are only 2 players in the round") {
            constexpr auto some_constant = 42;
            player players[] = {player{some_constant}, player{some_constant}};
            auto player_pointers = player_container{&players[0], &players[1]};
            auto r = round{player_pointers, player_pointers.begin()};

            GIVEN("there was no action in the round yet") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                REQUIRE(!r.over());
                REQUIRE_EQ(r.num_active_players(), 2);

                WHEN("the first player acts aggressively") {
                    r.action_taken(action::aggressive);

                    THEN("the last aggressive actor remains unchanged") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                    }

                    THEN("the second player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there are still 2 active players") {
                        REQUIRE_EQ(r.num_active_players(), 2);
                    }
                }

                WHEN("the first player acts aggressively and leaves") {
                    r.action_taken(action::aggressive | action::leave);

                    THEN("the last aggressive actor remains unchanged") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                    }

                    THEN("the second player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there is 1 active player") {
                        REQUIRE_EQ(r.num_active_players(), 1);
                    }
                }

                WHEN("the first player acts passively") {
                    r.action_taken(action::passive);

                    THEN("the last aggressive actor remains unchanged") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                    }

                    THEN("the second player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there are still 2 active players") {
                        REQUIRE_EQ(r.num_active_players(), 2);
                    }
                }

                WHEN("the first player acts passively and leaves") {
                    r.action_taken(action::passive | action::leave);

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }
                }

                WHEN("the first player leaves") {
                    r.action_taken(action::leave);

                    THEN("the round is over") {
                        REQUIRE(r.over());
                    }
                }
            }

            GIVEN("the next player is the last aggressive actor") {
                r.action_taken(action::aggressive);

                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                REQUIRE(!r.over());
                REQUIRE_EQ(r.num_active_players(), 2);

                WHEN("the player to act acts aggressively") {
                    r.action_taken(action::aggressive);

                    THEN("the player to act becomes the last aggressive actor") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 1);
                    }

                    THEN("the last aggressive actor becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there are still 2 active players") {
                        REQUIRE_EQ(r.num_active_players(), 2);
                    }
                }

                WHEN("the player to act acts aggressively and leaves") {
                    r.action_taken(action::aggressive | action::leave);

                    THEN("the player to act becomes the last aggressive actor") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 1);
                    }

                    THEN("the last aggressive actor becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there is 1 active player") {
                        REQUIRE_EQ(r.num_active_players(), 1);
                    }
                }

                WHEN("the current player acts passively") {
                    r.action_taken(action::passive);

                    THEN("the round is over") {
                        REQUIRE(r.over());
                    }
                }

                WHEN("the player to act acts passively and leaves") {
                    r.action_taken(action::passive | action::leave);

                    THEN("the round is over") {
                        REQUIRE(r.over());
                    }
                }

                WHEN("the player to act leaves") {
                    r.action_taken(action::leave);

                    THEN("the round is over") {
                        REQUIRE(r.over());
                    }
                }
            }
        }

        SCENARIO_CLASS("there are more than 2 players in the round") {
            constexpr auto some_constant = 42;
            player players[] = {player{some_constant}, player{some_constant}, player{some_constant}};
            auto player_pointers = player_container{&players[0], &players[1], &players[2]};
            auto r = round{player_pointers, player_pointers.begin()};

            const auto initial_num_active_players = r.num_active_players();

            GIVEN("there was no action in the round yet") {
                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                REQUIRE(!r.over());
                REQUIRE_EQ(r.num_active_players(), 3);

                WHEN("the player to act acts aggressively") {
                    r.action_taken(action::aggressive);

                    THEN("the last aggressive actor remains unchanged") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("the number of active players remains unchanged") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players);
                    }
                }

                WHEN("the player to act acts aggressively and leaves") {
                    r.action_taken(action::aggressive | action::leave);

                    THEN("the last aggressive actor remains unchanged") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there is one less active player") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
                    }
                }

                WHEN("the player to act acts passively") {
                    r.action_taken(action::passive);

                    THEN("the last aggressive actor remains unchanged") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("the number of active players remains unchanged") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players);
                    }
                }

                WHEN("the player to act acts passively and leaves") {
                    r.action_taken(action::passive | action::leave);

                    THEN("the last aggressive actor remains unchanged") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there is one less active player") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
                    }
                }

                WHEN("the player to act leaves") {
                    r.action_taken(action::leave);

                    THEN("the last aggressive actor remains unchanged") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there is one less active player") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
                    }
                }
            }

            GIVEN("the next player is the last aggressive actor") {
                r.action_taken(action::aggressive);
                r.action_taken(action::passive);

                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                REQUIRE(!r.over());
                REQUIRE_EQ(r.num_active_players(), 3);

                WHEN("the player to act acts aggressively") {
                    r.action_taken(action::aggressive);

                    THEN("the player to act becomes the last aggressive actor") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 2);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("the number of active players remains unchanged") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players);
                    }
                }

                WHEN("the player to act acts aggressively and leaves") {
                    r.action_taken(action::aggressive | action::leave);

                    THEN("the player to act becomes the last aggressive actor") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 2);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 0);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there is one less active player") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
                    }
                }

                WHEN("the player to act acts passively") {
                    r.action_taken(action::passive);

                    THEN("the round is over") {
                        REQUIRE(r.over());
                    }
                }

                WHEN("the player to act acts passively and leaves") {
                    r.action_taken(action::passive | action::leave);

                    THEN("the round is over") {
                        REQUIRE(r.over());
                    }
                }

                WHEN("the player to act leaves") {
                    r.action_taken(action::leave);

                    THEN("the round is over") {
                        REQUIRE(r.over());
                    }
                }
            }

            GIVEN("the next player is not the last aggressive actor") {
                r.action_taken(action::aggressive);

                REQUIRE_EQ(r.player_to_act(), r.players().begin() + 1);
                REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                REQUIRE(!r.over());
                REQUIRE_EQ(r.num_active_players(), 3);

                WHEN("the player to act acts aggressively") {
                    r.action_taken(action::aggressive);

                    THEN("the player to act becomes the last aggressive actor") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 1);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("the number of active players remains unchanged") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players);
                    }
                }

                WHEN("the player to act acts aggressively and leaves") {
                    r.action_taken(action::aggressive | action::leave);

                    THEN("the player to act becomes the last aggressive actor") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 1);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there is one less active player") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
                    }
                }

                WHEN("the player to act acts passively") {
                    r.action_taken(action::passive);

                    THEN("the last aggressive actor remains unchanged") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("the number of active players remains unchanged") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players);
                    }
                }

                WHEN("the player to act acts passively and leaves") {
                    r.action_taken(action::passive | action::leave);

                    THEN("the last aggressive actor remains unchanged") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there is one less active player") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
                    }
                }

                WHEN("the player to act leaves") {
                    r.action_taken(action::leave);

                    THEN("the last aggressive actor remains unchanged") {
                        REQUIRE_EQ(r.last_aggressive_actor(), r.players().begin() + 0);
                    }

                    THEN("the next player becomes the player to act") {
                        REQUIRE_EQ(r.player_to_act(), r.players().begin() + 2);
                    }

                    THEN("the round is not over") {
                        REQUIRE(!r.over());
                    }

                    THEN("there is one less active player") {
                        REQUIRE_EQ(r.num_active_players(), initial_num_active_players - 1);
                    }
                }
            }
        }

        SCENARIO_CLASS("there are 3 players and the first one acts first") {
            player ps[] = {player{300}, player{400}, player{500}};
            auto players = player_container{&ps[0], &ps[1], &ps[2]};
            auto current = players.begin();

            // Test for first action (_is_next_player_contested) exception.
            GIVEN("a round") {
                auto r = round{players, current};
                REQUIRE(!r.over());
            }

            // Test for round end by _num_active_players.
            GIVEN("a round") {
                auto r = round{players, current};

                WHEN("the first two players leave") {
                    r.action_taken(action::leave);
                    r.action_taken(action::leave);

                    THEN("the round is over") {
                        REQUIRE(r.over());
                        // Round should end because of _num_active_players being 1
                        REQUIRE_EQ(r._num_active_players, 1);
                    }
                }
            }

            // Test for round end by action reaching _last_aggressive_actor.
            GIVEN("a round") {
                auto r = round{players, current};

                WHEN("the first player leaves, and the other two act passively") {
                    r.action_taken(action::leave);
                    r.action_taken(action::passive);
                    r.action_taken(action::passive);

                    THEN("the round is over") {
                        REQUIRE(r.over());
                        // Round should end because the _player_to_act is _last_aggressive_actor
                        REQUIRE_EQ(r._player_to_act, r._last_aggressive_actor);
                    }
                }
            }
        }
    };

    struct chip_range {
        chips min;
        chips max;

        auto contains(chips amount) const noexcept -> bool {
            return min <= amount && amount <= max;
        }
    };

    class betting_round {
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
            chip_range chips = {0, 0};
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
                        REQUIRE_EQ(actions.chips.min, players[0].total_chips());
                        REQUIRE_EQ(actions.chips.max, players[0].total_chips());
                    }
                }

                WHEN("the player has amount of chips equal to the minimum re-raise bet") {
                    players[0] = player{100};
                    REQUIRE_EQ(players[0].total_chips(), r.biggest_bet() + r.min_raise());

                    THEN("he can raise, but only his entire stack") {
                        const auto actions = r.legal_actions();
                        REQUIRE(actions.can_raise);
                        REQUIRE_EQ(actions.chips.min, players[0].total_chips());
                        REQUIRE_EQ(actions.chips.max, players[0].total_chips());
                    }
                }

                WHEN("the player has more chips than the minimum re-raise bet") {
                    players[0] = player{150};
                    REQUIRE_GT(players[0].total_chips(), r.biggest_bet() + r.min_raise());

                    THEN("he can raise any amount ranging from min re-raise to his entire stack") {
                        const auto actions = r.legal_actions();
                        REQUIRE(actions.can_raise);
                        REQUIRE_EQ(actions.chips.min, r.biggest_bet() + r.min_raise());
                        REQUIRE_EQ(actions.chips.max, players[0].total_chips());
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

    class pot {
        std::vector<player *> _eligible_players;
        chips _size;

    public:
        pot() noexcept : _size{0} {}

        auto size() const noexcept -> chips { return _size; }

        auto eligible_players() const noexcept -> span<player *const> { return _eligible_players; }

        void add(chips amount) noexcept {
            assert(amount >= 0);
            _size += amount;
        }

        auto collect_bets_from(span<player *const> players) noexcept -> chips {
            auto it = std::find_if(players.begin(), players.end(), [] (auto p) { return p && p->bet_size() != 0; });
            if (it == players.end()) {
                for (auto p : players) {
                    if (p) _eligible_players.push_back(p);
                }
                return 0;
            } else {
                auto min_bet = (*it)->bet_size();
                std::for_each(it, players.end(), [&] (auto p) {
                    if (p && p->bet_size() != 0 && p->bet_size() < min_bet) min_bet = p->bet_size();
                });
                _eligible_players.clear();
                for (auto p : players) {
                    if (p && p->bet_size() != 0) {
                        p->take_from_bet(min_bet);
                        _size += min_bet;
                        _eligible_players.push_back(p);
                    }
                }
                return min_bet;
            }
        }

        // Two distinct cases to test.
        TEST_CASE_CLASS("some bets remaining") {
            player players[] = {player{100}, player{100}, player{100}};
            players[0].bet(0);
            players[1].bet(20);
            //players[2].bet(60);
            auto player_pointers = std::vector<player *>{&players[0], &players[1], &players[2]};
            auto p = pot{};
            p.collect_bets_from(player_pointers);
            REQUIRE_EQ(p._size, 20);
            REQUIRE_EQ(p._eligible_players.size(), 1);
            // REQUIRE_EQ(players[0].bet_size(), 0);
            REQUIRE_EQ(players[1].bet_size(), 0);
        }

        TEST_CASE_CLASS("no bets remaining") {
            player players[] = {player{100}, player{100}, player{100}};
            // no bets
            auto player_pointers = std::vector<player *>{&players[0], &players[1], &players[2]};
            auto p = pot{};
            p.collect_bets_from(player_pointers);
            REQUIRE_EQ(p._size, 0);
            REQUIRE_EQ(p._eligible_players.size(), 3);
        }
    };

    class pot_manager {
        std::vector<pot> _pots; // FIXME: static_vector with max_players-1 capacity
        chips _aggregate_folded_bets = 0;

    public:
        pot_manager() noexcept : _pots(1) {}

        auto pots() const noexcept -> span<const pot> { return _pots; }

        void bet_folded(chips amount) noexcept {
            _aggregate_folded_bets += amount;
        }

        void collect_bets_from(span<player *const> players) noexcept {
            // TODO: Return a list of transactions.
            for (;;) {
                const auto min_bet = _pots.back().collect_bets_from(players);
                _pots.back().add(std::min(_aggregate_folded_bets, (chips)_pots.back().eligible_players().size() * min_bet));
                auto it = std::find_if(players.begin(), players.end(), [] (auto p) { return p && p->bet_size() != 0; });
                if (it != players.end()) {
                    _pots.emplace_back();
                    continue;
                }
                break;
            }
            _aggregate_folded_bets = 0;
        }

        TEST_CASE_CLASS("whatever") {
            player players[] = {player{100}, player{100}, player{100}};
            players[0].bet(20);
            players[1].bet(40);
            players[2].bet(60);
            auto player_pointers = std::vector<player *>{&players[0], &players[1], &players[2]};
            auto pm = pot_manager{};
            pm.collect_bets_from(player_pointers);
            REQUIRE_EQ(pm._pots.size(), 3);
            REQUIRE_EQ(pm._pots[0].size(), 60);
            REQUIRE_EQ(pm._pots[1].size(), 40);
            REQUIRE_EQ(pm._pots[2].size(), 20);
        }

    };

    // Data members
    player_container _players{};
    player_container::iterator _button;

    betting_round _betting_round;
    blinds _blinds;

    deck *_deck;
    community_cards *_community_cards;

    poker::round_of_betting _round_of_betting;
    bool _betting_round_ended = false;
    pot_manager _pot_manager{};
    // store legal action range?

    auto next_or_wrap(player_container::iterator it) noexcept -> player_container::iterator {
        do {
            ++it;
            if (it == _players.end()) it = _players.begin();
        } while (!*it);
        return it;
    }

    void collect_ante() noexcept {
        for (auto p : _players) {
            if (p) {
                p->take_from_stack(std::min(_blinds.ante, p->total_chips()));
            }
        }
    }

    auto post_blinds() noexcept -> player_container::iterator {
        auto it = _button;
        const auto num_players = std::count_if(std::begin(_players), std::end(_players), [] (player *p) { return p != nullptr; });
        if (num_players != 2) it = next_or_wrap(it);
        (**it).bet(std::min(_blinds.small, (**it).total_chips()));
        it = next_or_wrap(it);
        (**it).bet(std::min(_blinds.big, (**it).total_chips()));
        return it;
    }

    void deal_hole_cards() noexcept {
        for (auto p : _players) {
            if (p) p->hole_cards = {_deck->draw(), _deck->draw()};
        }
    }

    // Deals community cards up until the current round of betting.
    void deal_community_cards() noexcept {
        auto cards = std::vector<card>();
        const auto num_cards_to_deal = to_underlying(_round_of_betting) - _community_cards->cards().size();
        std::generate_n(std::back_inserter(cards), num_cards_to_deal, [&] { return _deck->draw(); });
        _community_cards->deal(cards);
    }

public:
    template<typename PlayerRange>
    dealer(PlayerRange &players, decltype(std::begin(players)) button, blinds b, deck &d, community_cards &cc) noexcept
        : _blinds(b)
        , _deck(&d)
        , _community_cards(&cc)
        , _round_of_betting(round_of_betting::preflop)
    {
        constexpr auto addressof = [] (player &p) { return &p; };
        std::transform(std::begin(players), std::end(players), std::begin(_players), addressof);
        _button = _players.begin() + std::distance(std::begin(players), button);
    }

    dealer(const dealer &) = delete;
    auto operator=(const dealer &) -> dealer & = delete;

    dealer(dealer &&) = delete;
    auto operator=(dealer &&) -> dealer & = delete;

    auto player_to_act() const noexcept -> player_container::const_iterator {
        return _betting_round.player_to_act();
    }

    auto players() const noexcept -> const player_container & { return _betting_round.players(); }

    auto done() const noexcept -> bool {
        return _betting_round.over() && _betting_round_ended && _round_of_betting == round_of_betting::river;
    }

    auto round_of_betting() const noexcept -> poker::round_of_betting {
        return _round_of_betting;
    }

    // TODO: What happens when d.done() ? Do we assert or return a special value? Special value sounds bad.
    auto num_active_players() const noexcept -> std::size_t {
        return _betting_round.num_active_players();
    }

    TEST_CASE_CLASS("construction") {
        // auto players = std::vector<player>{player{1}, player{1}, player{1}};
        // auto rng = std::make_unique<std::mt19937>(std::random_device{}());
        // auto dck = deck{*rng};
        // auto cc = community_cards{};
        // auto dlr = dealer{players, players.begin()+1, {10, 20, 0}, dck, cc};
    }

    auto start_hand() noexcept -> void {
        collect_ante();
        const auto first_action = next_or_wrap(post_blinds());
        deal_hole_cards();
        if (std::count_if(_players.begin(), _players.end(), [] (auto p) { return p && p->stack() != 0; }) > 1) {
            new (&_betting_round) betting_round(_players, first_action, _blinds.big);
        }
    }

    TEST_CASE_CLASS("Starting the hand") {
        const auto b = blinds{25, 50};
        auto dck = deck{std::default_random_engine{std::random_device{}()}};
        auto cc = community_cards{};

        GIVEN("A hand with two players who can cover their blinds") {
            player players[] = {player{100}, player{100}};
            auto d = dealer{players, &players[0], b, dck, cc};

            WHEN("The hand starts") {
                d.start_hand();

                THEN("The button has posted the small blind") {
                    REQUIRE_EQ(players[0].bet_size(), 25);
                }

                THEN("The other player has posted the big blind") {
                    REQUIRE_EQ(players[1].bet_size(), 50);
                }

                THEN("The action is on the button") {
                    REQUIRE_EQ(*d.player_to_act(), &players[0]);
                }
            }
        }

        GIVEN("A hand with two players who can't cover their blinds") {
            player players[] = {player{20}, player{20}};
            auto d = dealer{players, &players[0], b, dck, cc};

            WHEN("The hand starts") {
                d.start_hand();

                THEN("The hand has ended") {
                    REQUIRE(d.betting_round_over());
                    d.end_betting_round();
                    REQUIRE(d.done());
                }
            }
        }

        GIVEN("A hand with more than two players") {
            player players[] = {player{100}, player{100}, player{100}, player{100}};
            auto d = dealer{players, &players[0], b, dck, cc};

            WHEN("The hand starts") {
                d.start_hand();

                THEN("The button+1 has posted the small blind") {
                    REQUIRE_EQ(players[1].bet_size(), 25);
                }

                THEN("The button+2 has posted the big blind") {
                    REQUIRE_EQ(players[2].bet_size(), 50);
                }

                THEN("The action is on the button+3") {
                    REQUIRE_EQ(*d.player_to_act(), &players[3]);
                }
            }
        }
    }

    enum class action : unsigned char {
        fold  = 1 << 0,
        check = 1 << 1,
        call  = 1 << 2,
        bet   = 1 << 3,
        raise = 1 << 4
    };
    DEFINE_FRIEND_CONSTEXPR_FLAG_OPERATIONS(action)

    static auto is_valid(action a) noexcept -> bool {
        return std::bitset<CHAR_BIT>(static_cast<unsigned char>(a)).count() == 1;
    }

    static constexpr auto is_aggressive(action a) noexcept -> bool {
        return bool(a & action::bet) || bool(a & action::raise);
    }

    struct action_range {
        dealer::action action = dealer::action::fold; // you can always fold
        chip_range chips;

        auto contains(dealer::action a, poker::chips bet = 0) const noexcept -> bool {
            assert(is_valid(a));
            return static_cast<bool>(a & action) && (is_aggressive(a) ? chips.contains(bet) : true);
        }
    };

    // TODO: legal_actions() ?
    auto legal_action_range() const noexcept -> action_range {
        const auto &player = **_betting_round.player_to_act();
        const auto actions = _betting_round.legal_actions();
        auto ar = action_range{};
        ar.chips = actions.chips;
        // Below we take care of differentiating between check/call and bet/raise,
        // which the betting_round treats as just "match" and "raise".
        if (_betting_round.biggest_bet() - player.bet_size() == 0) {
            ar.action |= action::check;
            assert(actions.can_raise); // If you can check, you can always bet or raise.
            // If this guy can check, with his existing bet_size, he is the big blind.
            if (player.bet_size() > 0) ar.action |= action::raise;
            else ar.action |= action::bet;
        } else {
            ar.action |= action::call;
            // If you can call, you may or may not be able to raise.
            if (actions.can_raise) ar.action |= action::raise;
        }
        return ar;
    }

    auto betting_round_over() const noexcept -> bool {
        return _betting_round.over();
    }

    void action_taken(action a, chips bet = 0) noexcept {
        assert(!betting_round_over());
        assert(legal_action_range().contains(a, bet));

        if (bool(a & action::check) || bool(a & action::call)) {
            _betting_round.action_taken(betting_round::action::match);
        } else if (bool(a & action::bet) || bool(a & action::raise)) {
            _betting_round.action_taken(betting_round::action::raise, bet);
        } else {
            assert(bool(a & action::fold));
            _pot_manager.bet_folded((*player_to_act())->bet_size());
            /* const auto folded_player_index = std::distance(players().begin(), player_to_act()); */
            const auto folded_player_index = player_to_act() - players().begin();
            _players[folded_player_index] = nullptr;
            _betting_round.action_taken(betting_round::action::leave);
        }
    }

    void end_betting_round() noexcept {
        assert(!_betting_round_ended);
        assert(betting_round_over());
        _pot_manager.collect_bets_from(_players);
        if (_betting_round.num_active_players() <= 1) {
            _round_of_betting = round_of_betting::river;
            // If there is only one pot, and there is only one player in it...
            if (_pot_manager.pots().size() == 1 && _pot_manager.pots()[0].eligible_players().size() == 1) {
                // ...there is no need to deal the undealt community cards.
            } else {
                deal_community_cards();
            }
            _betting_round_ended = true;
            // Now you call showdown()
        } else if (_round_of_betting < round_of_betting::river) {
            // Start the next betting round.
            _round_of_betting = next(_round_of_betting);
            const auto button_index = std::distance(_players.begin(), _button);
            _players = _betting_round.players();
            _button = _players.begin() + button_index;
            new (&_betting_round) betting_round(_players, next_or_wrap(_button), 0);
            deal_community_cards();
            // _betting_round_ended = false;
        } else {
            assert(_round_of_betting == round_of_betting::river);
            _betting_round_ended = true;
            // Now you call showdown()
        }
    }

    TEST_CASE_CLASS("Ending the betting round") {
        const auto b = blinds{25, 50};
        auto dck = deck{std::default_random_engine{std::random_device{}()}};
        auto cc = community_cards{};
        player players[] = {player{1000}, player{1000}, player{1000}};
        auto d = dealer{players, &players[0], b, dck, cc};

        GIVEN("There is two or more active players at the end of any betting round except river") {
            d.start_hand();
            d.action_taken(dealer::action::call);
            d.action_taken(dealer::action::call);
            d.action_taken(dealer::action::check);

            REQUIRE(d.betting_round_over());
            REQUIRE_GE(d.num_active_players(), 2);
            REQUIRE_NE(d.round_of_betting(), poker::round_of_betting::river);
            REQUIRE_EQ(cc.cards().size(), 0);

            WHEN("The betting round is ended") {
                d.end_betting_round();

                THEN("The next betting round begins") {
                    REQUIRE(!d.betting_round_over());
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

            REQUIRE(d.betting_round_over());
            REQUIRE_EQ(d.round_of_betting(), poker::round_of_betting::river);
            REQUIRE_EQ(cc.cards().size(), 5);

            WHEN("The betting round is ended") {
                d.end_betting_round();

                THEN("The hand is over") {
                    REQUIRE(d.done());
                }
            }
        }

        GIVEN("There is one or less active players at the end of a betting round and more than one player in all pots") {
            d.start_hand();
            d.action_taken(dealer::action::raise, 1000);
            d.action_taken(dealer::action::call);
            d.action_taken(dealer::action::fold);

            REQUIRE(d.betting_round_over());
            REQUIRE_LE(d.num_active_players(), 1);
            REQUIRE_NE(d.round_of_betting(), poker::round_of_betting::river);
            REQUIRE_EQ(cc.cards().size(), 0);

            WHEN("The betting round is ended") {
                d.end_betting_round();

                THEN("The hand is over") {
                    REQUIRE(d.done());
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

            REQUIRE(d.betting_round_over());
            REQUIRE_LE(d.num_active_players(), 1);
            REQUIRE_NE(d.round_of_betting(), poker::round_of_betting::river);
            REQUIRE_EQ(cc.cards().size(), 0);

            WHEN("The betting round is ended") {
                d.end_betting_round();

                THEN("The hand is over") {
                    REQUIRE(d.done());
                }
                THEN("The undealt community cards (if any) are not dealt") {
                    REQUIRE_EQ(cc.cards().size(), 0);
                }
            }
        }
    }

    TEST_CASE_CLASS("flop, someone folded preflop, now others fold, when 1 remains, the hand should be over") {
        // A bug where we pass a container of pointers where some are null to the betting_round => round.
        // round initializes _num_active_players to .size() of the container, instead of counting non-null pointers.
        //
        const auto b = blinds{25, 50};
        auto dck = deck{std::default_random_engine{std::random_device{}()}};
        auto cc = community_cards{};
        player players[] = {player{1000}, player{1000}, player{1000}};
        auto d = dealer{players, &players[0], b, dck, cc};

        d.start_hand();
        d.action_taken(dealer::action::fold);
        d.action_taken(dealer::action::call);
        d.action_taken(dealer::action::check);
        REQUIRE(d.betting_round_over());
        d.end_betting_round();

        d.action_taken(dealer::action::fold);
        REQUIRE(d.betting_round_over());
    }

    void showdown() noexcept {
        if (_pot_manager.pots().size() == 1 && _pot_manager.pots()[0].eligible_players().size() == 1) {
            // No need to evaluate the hand. There is only one player.
            _pot_manager.pots()[0].eligible_players()[0]->add_to_stack(_pot_manager.pots()[0].size());
            return;

            // TODO: Also, no reveals in this case. Reveals are only necessary when there is >=2 players.
        }
        for (auto p : _pot_manager.pots()) {
            auto player_results = std::vector<std::pair<player *, hand>>();
            player_results.reserve(p.eligible_players().size());
            std::transform(p.eligible_players().begin(), p.eligible_players().end(), std::back_inserter(player_results), [&] (player *p) {
                return std::make_pair(p, hand{p->hole_cards, *_community_cards});
            });
            std::sort(player_results.begin(), player_results.end(), [] (auto &&lhs, auto &&rhs) {
                return lhs.second < rhs.second;
            });
            auto first_winner = player_results.begin();
            auto last_winner = std::adjacent_find(player_results.begin(), player_results.end(), [] (auto &&lhs, auto &&rhs) {
                return lhs.second != rhs.second;
            });
            if (last_winner != player_results.end()) ++last_winner;
            const auto payout = static_cast<chips>(p.size() / std::distance(first_winner, last_winner));
            std::for_each(first_winner, last_winner, [&] (auto &&winner) {
                winner.first->add_to_stack(payout);
            });
        }
    }

    TEST_CASE_CLASS("Showdown") {
        SUBCASE("single pot single player") {
            const auto b = blinds{25, 50};
            auto dck = deck{std::default_random_engine{std::random_device{}()}};
            auto cc = community_cards{};
            player players[] = {player{1000}, player{1000}, player{1000}};
            auto d = dealer{players, &players[0], b, dck, cc};

            d.start_hand();
            d.action_taken(dealer::action::raise, 1000);
            d.action_taken(dealer::action::fold);
            d.action_taken(dealer::action::fold);
            d.end_betting_round();

            REQUIRE(d.done());

            d.showdown();
            REQUIRE_EQ(players[0].stack(), 1075);
        }

        SUBCASE("multiple pots, multiple winners") {
            const auto b = blinds{25, 50};
            auto dck = deck{std::default_random_engine{std::random_device{}()}};
            auto cc = community_cards{};
            player players[] = {player{300}, player{200}, player{100}};
            auto d = dealer{players, &players[0], b, dck, cc};

            d.start_hand();
            d.action_taken(dealer::action::raise, 300);
            d.action_taken(dealer::action::call);
            d.action_taken(dealer::action::call);
            d.end_betting_round();

            REQUIRE(d.done());

            cc = {};
            cc.deal(std::array{
                card{card_rank::A, card_suit::spades},
                card{card_rank::K, card_suit::spades},
                card{card_rank::Q, card_suit::spades},
                card{card_rank::J, card_suit::spades},
                card{card_rank::T, card_suit::spades}
            });
            for (auto &p : players) {
                p.hole_cards = {
                    {card_rank::_2, card_suit::clubs},
                    {card_rank::_3, card_suit::clubs}
                };
            }

            d.showdown();

            REQUIRE_EQ(players[0].stack(), 300);
            REQUIRE_EQ(players[1].stack(), 200);
            REQUIRE_EQ(players[2].stack(), 100);
        }
    }
};

} // namespace poker

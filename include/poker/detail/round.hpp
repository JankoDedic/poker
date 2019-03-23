#pragma once

#include <poker/player.hpp>

#include "poker/detail/utility.hpp"

namespace poker::detail {

class round {
    static constexpr auto max_players = 9;
    using player_container = std::array<player *, max_players>;

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
    POKER_DETAIL_DEFINE_FRIEND_FLAG_OPERATIONS(action)

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

} // namespace poker::detail

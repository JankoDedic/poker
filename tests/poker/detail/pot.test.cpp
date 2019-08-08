#include <doctest/doctest.h>

#include <poker/pot.hpp>

using namespace poker;

// Two distinct cases to test.
TEST_CASE("some bets remaining") {
    auto players = seat_array{};
    players.add_player(0, player{100});
    players.add_player(1, player{100});
    players.add_player(2, player{100});
    players[0].bet(0);
    players[1].bet(20);
    //players[2].bet(60);
    auto p = pot{};
    p.collect_bets_from(players);
    REQUIRE_EQ(p.size(), 20);
    REQUIRE_EQ(p.eligible_players().size(), 1);
    // REQUIRE_EQ(players[0].bet_size(), 0);
    REQUIRE_EQ(players[1].bet_size(), 0);
}

// Two distinct cases to test.
TEST_CASE("no bets remaining") {
    auto players = seat_array{};
    players.add_player(0, player{100});
    players.add_player(1, player{100});
    players.add_player(2, player{100});
    // no bets
    auto p = pot{};
    p.collect_bets_from(players);
    REQUIRE_EQ(p.size(), 0);
    REQUIRE_EQ(p.eligible_players().size(), 3);
}

TEST_CASE("Players who folded are not kept as eligible after a betting round with no bets") {
    auto players = seat_array{};
    players.add_player(0, player{100});
    players.add_player(1, player{100});
    players[0].bet(10);
    players[1].bet(10);
    auto p = pot{};
    p.collect_bets_from(players);
    players.remove_player(1);
    p.collect_bets_from(players);
    REQUIRE_EQ(p.eligible_players().size(), 1);
}

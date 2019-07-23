#include <doctest/doctest.h>

#include "poker/detail/pot_manager.hpp"

using namespace poker;
using namespace poker::detail;

TEST_CASE("whatever") {
    auto players = seat_array{};
    players.add_player(0, player{100});
    players.add_player(1, player{100});
    players.add_player(2, player{100});
    players[0].bet(20);
    players[1].bet(40);
    players[2].bet(60);
    auto pm = pot_manager{};
    pm.collect_bets_from(players);
    REQUIRE_EQ(pm.pots().size(), 3);
    REQUIRE_EQ(pm.pots()[0].size(), 60);
    REQUIRE_EQ(pm.pots()[1].size(), 40);
    REQUIRE_EQ(pm.pots()[2].size(), 20);
}

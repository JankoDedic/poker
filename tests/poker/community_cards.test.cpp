#include <doctest/doctest.h>

#include <poker/community_cards.hpp>

using namespace poker;

TEST_CASE("Cards are being dealt up to and including the given round of betting") {
    auto cc = community_cards{};

    GIVEN("A pre-flop situation") {
        REQUIRE_EQ(cc.cards().size(), 0);

        WHEN("A flop deal is requested") {
            cc.deal(std::array<card, 3>{});

            THEN("First three cards are dealt") {
                REQUIRE_EQ(cc.cards().size(), 3);
            }
        }

        WHEN("A turn deal is requested") {
            cc.deal(std::array<card, 4>{});

            THEN("First four cards are dealt") {
                REQUIRE_EQ(cc.cards().size(), 4);
            }
        }

        WHEN("A river deal is requested") {
            cc.deal(std::array<card, 5>{});

            THEN("All five cards are dealt") {
                REQUIRE_EQ(cc.cards().size(), 5);
            }
        }
    }

    GIVEN("A flop situation") {
        cc.deal(std::array<card, 3>{});
        REQUIRE_EQ(cc.cards().size(), 3);

        WHEN("A turn deal is requested") {
            cc.deal(std::array<card, 1>{});

            THEN("The fourth card is dealt") {
                REQUIRE_EQ(cc.cards().size(), 4);
            }
        }

        WHEN("A river deal is requested") {
            cc.deal(std::array<card, 2>{});
            
            THEN("The fifth card is dealt") {
                REQUIRE_EQ(cc.cards().size(), 5);
            }
        }
    }

    GIVEN("A turn situation") {
        cc.deal(std::array<card, 4>{});
        REQUIRE_EQ(cc.cards().size(), 4);

        WHEN("A river deal is requested") {
            cc.deal(std::array<card, 1>{});

            THEN("The fifth card is dealt") {
                REQUIRE_EQ(cc.cards().size(), 5);
            }
        }
    }
}

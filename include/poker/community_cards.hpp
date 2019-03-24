#pragma once

#include <array>
#include <cassert>

#include <doctest/doctest.h>

#include <poker/card.hpp>
#include <poker/deck.hpp>
#include "poker/detail/span.hpp"
#include "poker/detail/utility.hpp"

namespace poker {

enum class round_of_betting {
    preflop = 0,
    flop    = 3,
    turn    = 4,
    river   = 5
};

constexpr auto next(round_of_betting rob) noexcept -> round_of_betting {
    using poker::detail::to_underlying;
    return rob == round_of_betting::preflop ? round_of_betting::flop : static_cast<round_of_betting>(to_underlying(rob) + 1);
}

class community_cards {
    std::array<card, 5> _cards;
    std::size_t _size{0};

public:
    community_cards() noexcept = default;

    auto cards() const noexcept -> span<const card> {
        return span<const card>(_cards).first(_size);
    }

    void deal(span<const card> cards) {
        assert(static_cast<std::size_t>(cards.size()) <= 5 - _size);
        for (auto c : cards) _cards[_size++] = c;
    }
};

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

} // namespace poker

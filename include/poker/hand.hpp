#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <tuple>

#include <poker/card.hpp>
#include <poker/community_cards.hpp>
#include <poker/hole_cards.hpp>
#include "poker/detail/error.hpp"
#include "poker/detail/span.hpp"

namespace poker {

enum class hand_ranking {
    high_card,
    pair,
    two_pair,
    three_of_a_kind,
    straight,
    flush,
    full_house,
    four_of_a_kind,
    straight_flush,
    royal_flush
};

class hand {
    hand_ranking _ranking;
    int _strength;
    std::array<card, 5> _cards;

    hand(hand_ranking ranking, int strength, span<const card, 5> cards)
        : _ranking{ranking}
        , _strength{strength}
    {
        std::copy(cards.cbegin(), cards.cend(), _cards.begin());
    }

public:
    static auto _high_low_hand_eval(span<card, 7> cards) noexcept -> hand;
    static auto _straight_flush_eval(span<card, 7> cards) noexcept -> std::optional<hand>;

public:
    hand() = default;

    hand(const hole_cards& hc, const community_cards& cc) POKER_NOEXCEPT;

    hand(span<card, 7> cards) noexcept;

    auto ranking()  const noexcept -> hand_ranking        { return _ranking;  }
    auto strength() const noexcept -> int                 { return _strength; }
    auto cards()    const noexcept -> span<const card, 5> { return _cards;    }
};

inline auto operator==(const hand& lhs, const hand& rhs) noexcept -> bool {
    return lhs.ranking() == rhs.ranking() && lhs.strength() == rhs.strength();
}

inline auto operator!=(const hand& lhs, const hand& rhs) noexcept -> bool {
    return !(lhs == rhs);
}

inline auto operator<(const hand& lhs, const hand& rhs) noexcept -> bool {
    const auto r1 = lhs.ranking();
    const auto s1 = lhs.strength();
    const auto r2 = rhs.ranking();
    const auto s2 = rhs.strength();
    return std::tie(r1, s1) < std::tie(r2, s2);
}

inline auto operator>(const hand& lhs, const hand& rhs) noexcept -> bool {
    return rhs < lhs;
}

inline auto operator<=(const hand& lhs, const hand& rhs) noexcept -> bool {
    return !(rhs < lhs);
}

inline auto operator>=(const hand& lhs, const hand& rhs) noexcept -> bool {
    return !(lhs < rhs);
}

} // namespace poker

namespace poker::detail {

struct rank_info {
    card_rank rank;
    int count;
};

constexpr auto next_rank(span<const card> cards) noexcept -> rank_info {
    assert(!cards.empty());
    const auto first_rank = cards[0].rank;
    const auto second_rank_index = [&] {
        for (auto i = 0; i < cards.size(); ++i) {
            if (cards[i].rank != cards[0].rank) {
                return i;
            }
        }
        return static_cast<int>(cards.size());
    }();
    return {first_rank, second_rank_index};
}

inline auto get_strength(span<const card, 5> arg_cards) noexcept -> int {
    auto cards = span<const card>(arg_cards);
    auto sum = 0;
    auto multiplier = static_cast<int>(std::pow(13, 4));
    for (;;) {
        /* const auto [rank, count] = next_rank(cards); */
        const auto tmp = next_rank(cards);
        const auto rank = tmp.rank;
        const auto count = tmp.count;
        sum += multiplier * static_cast<int>(rank);
        cards = cards.subspan(count);
        if (!cards.empty()) {
            multiplier /= 13;
        } else {
            break;
        }
    }
    return sum;
}

} // namespace poker::detail

namespace poker {

inline auto hand::_high_low_hand_eval(span<card, 7> cards) noexcept -> hand {
    using poker::detail::get_strength, poker::detail::next_rank;

    auto rank_occurrences = std::array<int, 13>{};
    const auto get_rank_occurrences = [&] (card c) -> int& {
        return rank_occurrences[static_cast<std::size_t>(c.rank)];
    };
    for (card c : cards) {
        ++get_rank_occurrences(c);
    }
    const auto cmp = [&] (card c1, card c2) -> bool {
        if (get_rank_occurrences(c1) == get_rank_occurrences(c2)) {
            return c1.rank > c2.rank;
        } else {
            return get_rank_occurrences(c1) > get_rank_occurrences(c2);
        }
    };
    std::sort(cards.begin(), cards.end(), cmp);

    auto ranking = hand_ranking{};
    /* const auto [rank, count] = next_rank(cards); */
    const auto tmp = next_rank(cards);
    /* const auto rank = tmp.rank; // UNUSED VARIABLE */
    const auto count = tmp.count;
    if (count == 4) {
        const auto greater_rank = [] (card x, card y) -> bool {
            return x.rank > y.rank;
        };
        std::sort(cards.begin() + 4, cards.end(), greater_rank);
        ranking = hand_ranking::four_of_a_kind;
    } else if (count == 3) {
        /* const auto [rank, count] = next_rank(cards.last<4>()); */
        const auto tmp = next_rank(cards.last<4>());
        /* const auto rank = tmp.rank; // UNUSED VARIABLE */
        const auto count = tmp.count;
        if (count == 2) {
            ranking = hand_ranking::full_house;
        } else {
            ranking = hand_ranking::three_of_a_kind;
        }
    } else if (count == 2) {
        /* const auto [rank, count] = next_rank(cards.last<5>()); */
        const auto tmp = next_rank(cards.last<5>());
        /* const auto rank = tmp.rank; // UNUSED VARIABLE */
        const auto count = tmp.count;
        if (count == 2) {
            ranking = hand_ranking::two_pair;
        } else {
            ranking = hand_ranking::pair;
        }
    } else {
        ranking = hand_ranking::high_card;
    }

    const auto hand_cards = cards.first<5>();
    const auto strength = get_strength(hand_cards);
    return {ranking, strength, hand_cards};
}

} // namespace poker

namespace poker::detail {

// If there are >=5 cards with the same suit, return a span containing all of
// them.
inline auto get_suited_cards(span<card, 7> cards) noexcept -> std::optional<span<card>> {
    std::sort(cards.begin(), cards.end(), std::greater<card>{});
    auto first = cards.begin();
    for (;;) {
        auto last = std::find_if_not(first+1, cards.end(), [&] (auto card) {
            return card.suit == (*first).suit;
        });
        if (last-first >= 5) {
            return span<card>{first, last};
        } else if (last == cards.end()) {
            return std::nullopt;
        }
        first = last;
    }
}

// EXPECTS: 'cards' is a descending range of cards with unique ranks.
// Returns the subrange which contains the cards forming a straight. Ranks of
// cards in the resulting range are r, r-1, r-2... except for the wheel.
inline auto get_straight_cards(span<card> cards) noexcept -> std::optional<span<card, 5>> {
    assert(cards.size() >= 5);

    auto first = cards.begin();
    auto last = cards.end();

    for (;;) {
        auto it = std::adjacent_find(first, last, [] (auto c1, auto c2) {
            return static_cast<int>(c1.rank) != static_cast<int>(c2.rank) + 1;
        });
        if (it != last) {
            ++it;
        }
        if (it-first >= 5) {
            return span<card, 5>{first, 5};
        } else if (it-first == 4) {
            if (first->rank == card_rank::_5 && cards[0].rank == card_rank::A) {
                std::rotate(cards.begin(), first, cards.end());
                return span<card, 5>{cards.begin(), 5};
            }
        } else if (last-it < 4) {
            return std::nullopt;
        }
        first = it;
    }
}

} // namespace poker::detail

namespace poker {

inline auto hand::_straight_flush_eval(span<card, 7> cards) noexcept -> std::optional<hand> {
    using detail::get_suited_cards, detail::get_straight_cards;
    if (auto suited_cards = get_suited_cards(cards)) {
        if (auto straight_cards = get_straight_cards(*suited_cards)) {
            auto ranking = hand_ranking{};
            auto strength = int{};
            if ((*straight_cards)[0].rank == card_rank::A) {
                ranking = hand_ranking::royal_flush;
                strength = 0;
            } else {
                ranking = hand_ranking::straight_flush;
                strength = static_cast<int>((*straight_cards)[0].rank);
            }
            const auto cards = straight_cards->first<5>();
            return hand{ranking, strength, cards};
        } else {
            const auto ranking = hand_ranking::flush;
            const auto cards = suited_cards->first<5>();
            const auto strength = detail::get_strength(cards);
            return hand{ranking, strength, cards};
        }
    } else {
        const auto first = cards.begin();
        const auto last = cards.end();
        std::sort(first, last, [] (card c1, card c2) -> bool {
            return c1.rank > c2.rank;
        });
        constexpr auto equal_ranks = [] (card c1, card c2) -> bool {
            return c1.rank == c2.rank;
        };
        const auto u = std::unique(first, last, equal_ranks);
        const auto cards = span<card>{first, u};

        if (cards.size() < 5) {
            return std::nullopt;
        } else if (auto straight_cards = get_straight_cards(cards)) {
            const auto ranking = hand_ranking::straight;
            const auto strength = static_cast<int>((*straight_cards)[0].rank);
            return hand{ranking, strength, *straight_cards};
        }
    }
    return std::nullopt;
}

inline hand::hand(const hole_cards& hc, const community_cards& cc) POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(cc.cards().size() == 5, "All community cards must be dealt");
    auto cards = std::array<card, 7>{};
    cards[0] = hc.first;
    cards[1] = hc.second;
    std::copy(cc.cards().cbegin(), cc.cards().cend(), cards.begin() + 2);
    *this = hand{cards};
}

inline hand::hand(span<card, 7> cards) noexcept {
    auto h1 = _high_low_hand_eval(cards);
    if (auto h2 = _straight_flush_eval(cards)) {
        *this = std::max(h1, *h2);
    } else {
        *this = h1;
    }
}

} // namespace poker


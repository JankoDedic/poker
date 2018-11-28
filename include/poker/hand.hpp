#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <optional>
#include <tuple>

#include <poker/card.hpp>
#include <poker/span.hpp>

namespace poker {

using hole_cards = std::pair<card, card>;

class community_cards {
    std::array<card, 5> _cards;
    std::size_t _size{0};

public:
    community_cards() noexcept = default;

    span<const card>
    cards() const noexcept
    {
        return _cards;
    }

    void
    deal_flop(card first, card second, card third) noexcept
    {
        assert(_size == 0);
        _cards[0] = first;
        _cards[1] = second;
        _cards[2] = third;
        _size = 3;
    }

    void
    deal_turn(card c) noexcept
    {
        assert(_size == 3);
        _cards[3] = c;
        ++_size;
    }

    void
    deal_river(card c) noexcept
    {
        assert(_size == 4);
        _cards[4] = c;
        ++_size;
    }
};

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

} // namespace poker

namespace poker::debug {

inline
std::ostream&
operator<<(std::ostream& os, hand_ranking hr)
{
    constexpr const char* hand_ranking_names[] = {
        "high card",
        "pair",
        "two pair",
        "three of a kind",
        "straight",
        "flush",
        "full house",
        "four of a kind",
        "straight flush",
        "royal flush"
    };
    return os << hand_ranking_names[static_cast<std::size_t>(hr)];
}

} // namespace poker::debug

namespace poker {

class hand {
    hand_ranking _ranking;
    int _strength;
    std::array<card, 5> _cards;

    hand(
        hand_ranking ranking,
        int strength,
        span<const card, 5> cards) noexcept
        : _ranking(ranking)
        , _strength(strength)
    {
        std::copy(cards.cbegin(), cards.cend(), _cards.begin());
    }

public:
    static
    hand
    _high_low_hand_eval(span<card, 7> cards) noexcept;

    static
    std::optional<hand>
    _straight_flush_eval(span<card, 7> cards) noexcept;

public:
    hand() noexcept = default;

    hand(const hole_cards& hc, const community_cards& cc) noexcept;

    hand(span<card, 7> cards) noexcept;

    hand_ranking
    ranking() const noexcept
    {
        return _ranking;
    }

    int
    strength() const noexcept
    {
        return _strength;
    }

    span<const card, 5>
    cards() const noexcept
    {
        return _cards;
    }
};

inline
bool
operator==(const hand& lhs, const hand& rhs) noexcept
{
    return lhs.ranking() == rhs.ranking() && lhs.strength() == rhs.strength();
}

inline
bool
operator!=(const hand& lhs, const hand& rhs) noexcept
{
    return !(lhs == rhs);
}

inline
bool
operator<(const hand& lhs, const hand& rhs) noexcept
{
    const auto r1 = lhs.ranking();
    const auto s1 = lhs.strength();
    const auto r2 = rhs.ranking();
    const auto s2 = rhs.strength();
    return std::tie(r1, s1) < std::tie(r2, s2);
}

inline
bool
operator>(const hand& lhs, const hand& rhs) noexcept
{
    return rhs < lhs;
}

inline
bool
operator<=(const hand& lhs, const hand& rhs) noexcept
{
    return !(rhs < lhs);
}

inline
bool
operator>=(const hand& lhs, const hand& rhs) noexcept
{
    return !(lhs < rhs);
}

} // namespace poker

namespace poker::detail {

struct rank_info {
    card_rank rank;
    int count;
};

constexpr
inline
rank_info
next_rank(span<const card> cards) noexcept
{
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
    return rank_info{first_rank, second_rank_index};
}

inline
int
get_strength(span<const card, 5> arg_cards) noexcept
{
    auto cards = span<const card>(arg_cards);
    auto sum = 0;
    auto multiplier = static_cast<int>(std::pow(13, 4));
    for (;;) {
        const auto [rank, count] = next_rank(cards);
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

inline
hand
hand::_high_low_hand_eval(span<card, 7> cards) noexcept
{
    using poker::detail::get_strength, poker::detail::next_rank;

    auto rank_occurrences = std::array<int, 13>();
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

    auto ranking = hand_ranking();
    const auto [rank, count] = next_rank(cards);
    if (count == 4) {
        const auto greater_rank = [] (card x, card y) -> bool {
            return x.rank > y.rank;
        };
        std::sort(cards.begin() + 4, cards.end(), greater_rank);
        ranking = hand_ranking::four_of_a_kind;
    } else if (count == 3) {
        const auto [rank, count] = next_rank(cards.last<4>());
        if (count == 2) {
            ranking = hand_ranking::full_house;
        } else {
            ranking = hand_ranking::three_of_a_kind;
        }
    } else if (count == 2) {
        const auto [rank, count] = next_rank(cards.last<5>());
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
    return hand(ranking, strength, hand_cards);
}

} // namespace poker

namespace poker::detail {

// If there are >=5 cards with the same suit, return a span containing all of
// them.
inline
std::optional<span<card>>
get_suited_cards(span<card, 7> cards) noexcept
{
    std::sort(cards.begin(), cards.end(), std::greater<card>());
    auto first = cards.begin();
    for (;;) {
        auto last = std::find_if_not(first+1, cards.end(), [&] (auto card) {
            return card.suit == (*first).suit;
        });
        if (last-first >= 5) {
            return span<card>(first, last);
        } else if (last == cards.end()) {
            return std::nullopt;
        }
        first = last;
    }
}

// EXPECTS: 'cards' is a descending range of cards with unique ranks.
// Returns the subrange which contains the cards forming a straight. Ranks of
// cards in the resulting range are r, r-1, r-2... except for the wheel.
inline
std::optional<span<card, 5>>
get_straight_cards(span<card> cards) noexcept
{
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
            return span<card, 5>(first, 5);
        } else if (it-first == 4) {
            if (first->rank == card_rank::_5 && cards[0].rank == card_rank::A)
            {
                std::rotate(cards.begin(), first, cards.end());
                return span<card, 5>(cards.begin(), 5);
            }
        } else if (last-it < 4) {
            return std::nullopt;
        }
        first = it;
    }
}

} // namespace poker::detail

namespace poker {

inline
std::optional<hand>
hand::_straight_flush_eval(span<card, 7> cards) noexcept
{
    using detail::get_suited_cards, detail::get_straight_cards;
    if (auto suited_cards = get_suited_cards(cards)) {
        if (auto straight_cards = get_straight_cards(*suited_cards)) {
            auto ranking = hand_ranking();
            auto strength = int();
            if ((*straight_cards)[0].rank == card_rank::A) {
                ranking = hand_ranking::royal_flush;
                strength = 0;
            } else {
                ranking = hand_ranking::straight_flush;
                strength = static_cast<int>((*straight_cards)[0].rank);
            }
            const auto cards = straight_cards->first<5>();
            return hand(ranking, strength, cards);
        } else {
            const auto ranking = hand_ranking::flush;
            const auto cards = suited_cards->first<5>();
            const auto strength = detail::get_strength(cards);
            return hand(ranking, strength, cards);
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
        const auto cards = span<card>(first, u);

        if (auto straight_cards = get_straight_cards(cards)) {
            const auto ranking = hand_ranking::straight;
            const auto strength = static_cast<int>((*straight_cards)[0].rank);
            return hand(ranking, strength, *straight_cards);
        }
    }
    return std::nullopt;
}

inline
hand::hand(const hole_cards& hc, const community_cards& cc) noexcept
{
    assert(cc.cards().size() == 5);
    auto cards = std::array<card, 7>();
    cards[0] = hc.first;
    cards[1] = hc.second;
    std::copy(cc.cards().cbegin(), cc.cards().cend(), cards.begin() + 2);
    *this = hand(cards);
}

inline
hand::hand(span<card, 7> cards) noexcept
{
    auto h1 = _high_low_hand_eval(cards);
    if (auto h2 = _straight_flush_eval(cards)) {
        *this = std::max(h1, *h2);
    } else {
        *this = h1;
    }
}

} // namespace poker

namespace poker::debug {

inline
hand
make_hand(std::string_view str) noexcept
{
    auto cards = make_cards<7>(str);
    return hand(cards);
}


} // namespace poker::debug

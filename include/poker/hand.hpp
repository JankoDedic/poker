#pragma once

#include <algorithm>
#include <array>
#include <cassert>

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

public:
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

struct rank_info {
    card_rank rank;
    int count;
};

inline
rank_info
next_rank(span<const card> cards) noexcept
{
    const auto first_rank = cards[0].rank;
    const auto pred = [&] (card c) -> bool { return c.rank != first_rank; };
    const auto it = std::find_if(cards.cbegin(), cards.cend(), pred);
    return rank_info{first_rank, static_cast<int>(it - cards.cbegin())};
}

inline
int
get_strength(span<const card, 5> arg_cards) noexcept
{
    auto cards = span<const card>(arg_cards);
    auto sum = 0;
    auto multiplier = static_cast<int>(std::pow(13, 4));
    auto cards_remaining = 5;
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

inline
hand
hand::_high_low_hand_eval(span<card, 7> cards) noexcept
{
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

inline
hand::hand(const hole_cards& hc, const community_cards& cc) noexcept
{
    assert(cc.cards().size() == 5);
    auto cards = std::array<card, 7>();
    cards[0] = hc.first;
    cards[1] = hc.second;
    std::copy(cc.cards().cbegin(), cc.cards().cend(), cards.begin() + 2);
    *this = _high_low_hand_eval(cards);
}

inline
hand::hand(span<card, 7> cards) noexcept
{
    *this = _high_low_hand_eval(cards);
}

} // namespace poker

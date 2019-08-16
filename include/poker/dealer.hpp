#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <climits>
#include <new>

#include <poker/community_cards.hpp>
#include <poker/deck.hpp>
#include <poker/hand.hpp>
#include <poker/player.hpp>
#include <poker/pot.hpp>
#include <poker/slot_array.hpp>

#include "poker/detail/betting_round.hpp"
#include "poker/detail/error.hpp"
#include "poker/detail/pot_manager.hpp"
#include "poker/detail/utility.hpp"

namespace poker {

struct blinds {
    chips small = 0;
    chips big = 2*small;
};

constexpr auto operator==(const blinds& x, const blinds& y) noexcept -> bool {
    return x.small == y.small && x.big == y.big;
}

constexpr auto operator!=(const blinds& x, const blinds& y) noexcept -> bool {
    return !(x == y);
}

struct forced_bets {
    poker::blinds blinds = {};
    chips ante = 0;
};

constexpr auto operator==(const forced_bets& x, const forced_bets& y) noexcept -> bool {
    return x.blinds == y.blinds && x.ante == y.ante;
}

constexpr auto operator!=(const forced_bets& x, const forced_bets& y) noexcept -> bool {
    return !(x == y);
}

class dealer {
public:
    //
    // Constants
    //
    static constexpr auto max_players = 9;

    //
    // Types
    //
    enum class action : unsigned char {
        fold  = 1 << 0,
        check = 1 << 1,
        call  = 1 << 2,
        bet   = 1 << 3,
        raise = 1 << 4
    };
    POKER_DETAIL_DEFINE_FRIEND_FLAG_OPERATIONS(action)

    struct action_range {
        dealer::action action = dealer::action::fold; // you can always fold
        poker::chip_range chip_range;

        auto contains(dealer::action, poker::chips bet = 0) const POKER_NOEXCEPT -> bool;
    };

    //
    // Static functions
    //
    static           auto is_valid(action)      noexcept -> bool;
    static constexpr auto is_aggressive(action) noexcept -> bool;

    //
    // Special functions
    //
    dealer()              = default;
    dealer(const dealer&) = delete;
    dealer(dealer&&)      = delete;
    auto operator=(const dealer&) -> dealer& = delete;
    auto operator=(dealer&&)      -> dealer& = delete;

    //
    // Construction
    //
    dealer(seat_array_view players, seat_index button, forced_bets, deck&, community_cards&) POKER_NOEXCEPT;

    //
    // Observers
    //
    auto hand_in_progress()          const noexcept       -> bool;
    auto betting_rounds_completed()  const POKER_NOEXCEPT -> bool;
    auto player_to_act()             const POKER_NOEXCEPT -> seat_index;
    auto players()                   const noexcept       -> seat_array_view;
    auto round_of_betting()          const POKER_NOEXCEPT -> poker::round_of_betting;
    auto num_active_players()        const noexcept       -> std::size_t;
    auto biggest_bet()               const noexcept       -> chips;
    auto betting_round_in_progress() const noexcept       -> bool;
    auto legal_actions()             const POKER_NOEXCEPT -> action_range;
    auto pots()                      const POKER_NOEXCEPT -> span<const pot>;
    auto button()                    const noexcept       -> seat_index;
    auto hole_cards()                const POKER_NOEXCEPT -> slot_view<const poker::hole_cards, max_players>;

    //
    // Modifiers
    //
    void start_hand()                          POKER_NOEXCEPT;
    void action_taken(action, chips bet = 0)   POKER_NOEXCEPT;
    void end_betting_round()                   POKER_NOEXCEPT;
    void showdown()                            POKER_NOEXCEPT;

private:
    auto next_or_wrap(seat_index) noexcept -> seat_index;
    void collect_ante() noexcept;
    auto post_blinds() noexcept -> seat_index;
    void deal_hole_cards() noexcept;
    void deal_community_cards() noexcept; // Deals community cards up until the current round of betting.

private:
    seat_array_view                     _players;
    seat_index                          _button                   = 0;

    detail::betting_round               _betting_round;
    forced_bets                         _forced_bets;

    deck*                               _deck                     = nullptr;
    community_cards*                    _community_cards          = nullptr;
    std::array<poker::hole_cards, max_players> _hole_cards               = {};

    bool                                _hand_in_progress         = false;
    poker::round_of_betting             _round_of_betting         = poker::round_of_betting::preflop;
    bool                                _betting_rounds_completed = false;
    detail::pot_manager                 _pot_manager              = {};
};

inline auto dealer::action_range::contains(dealer::action a, poker::chips bet/* = 0*/) const POKER_NOEXCEPT -> bool {
    POKER_DETAIL_ASSERT(is_valid(a), "The dealer::action representation must be valid");
    return static_cast<bool>(a & action) && (is_aggressive(a) ? chip_range.contains(bet) : true);
}

inline auto dealer::is_valid(action a) noexcept -> bool {
    return std::bitset<CHAR_BIT>(static_cast<unsigned char>(a)).count() == 1;
}

inline constexpr auto dealer::is_aggressive(action a) noexcept -> bool {
    return static_cast<bool>(a & action::bet) || static_cast<bool>(a & action::raise);
}

inline dealer::dealer(seat_array_view players, seat_index button, forced_bets fb, deck& d, community_cards& cc) POKER_NOEXCEPT
    : _players{players}
    , _button{button}
    , _forced_bets{fb}
    , _deck{&d}
    , _community_cards{&cc}
{
    POKER_DETAIL_ASSERT(d.size() == 52, "Deck must be whole");
    POKER_DETAIL_ASSERT(cc.cards().size() == 0, "No community cards should have been dealt");
}

inline auto dealer::hand_in_progress() const noexcept -> bool {
    return _hand_in_progress;
}

inline auto dealer::betting_rounds_completed() const POKER_NOEXCEPT -> bool {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _betting_rounds_completed;
}

inline auto dealer::player_to_act() const POKER_NOEXCEPT -> seat_index {
    POKER_DETAIL_ASSERT(betting_round_in_progress(), "Betting round must be in progress");

    return _betting_round.player_to_act();
}

inline auto dealer::players() const noexcept -> seat_array_view {
    return _betting_round.players();
}

inline auto dealer::round_of_betting() const POKER_NOEXCEPT -> poker::round_of_betting {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _round_of_betting;
}

inline auto dealer::num_active_players() const noexcept -> std::size_t {
    return _betting_round.num_active_players();
}

inline auto dealer::biggest_bet() const noexcept -> chips {
    return _betting_round.biggest_bet();
}

inline auto dealer::betting_round_in_progress() const noexcept -> bool {
    return _betting_round.in_progress();
}

inline auto dealer::legal_actions() const POKER_NOEXCEPT -> action_range {
    POKER_DETAIL_ASSERT(betting_round_in_progress(), "Betting round must be in progress");

    const auto& player = _players[_betting_round.player_to_act()];
    const auto actions = _betting_round.legal_actions();
    auto ar = action_range{};
    ar.chip_range = actions.chip_range;
    // Below we take care of differentiating between check/call and bet/raise,
    // which the betting_round treats as just "match" and "raise".
    if (_betting_round.biggest_bet() - player.bet_size() == 0) {
        ar.action |= action::check;
        assert(actions.can_raise); // If you can check, you can always bet or raise.
        // If this guy can check, with his existing bet_size, he is the big blind.
        if (player.bet_size() > 0)
            ar.action |= action::raise;
        else
            ar.action |= action::bet;
    } else {
        ar.action |= action::call;
        // If you can call, you may or may not be able to raise.
        if (actions.can_raise) ar.action |= action::raise;
    }
    return ar;
}

inline auto dealer::pots() const POKER_NOEXCEPT -> span<const pot> {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _pot_manager.pots();
}


inline auto dealer::button() const noexcept -> seat_index {
    return _button;
}

inline auto dealer::hole_cards() const POKER_NOEXCEPT -> slot_view<const poker::hole_cards, max_players> {
    POKER_DETAIL_ASSERT(hand_in_progress() || betting_rounds_completed(), "Hand must be in progress or showdown must have ended");

    return {_hole_cards, _players.filter()};
}

inline void dealer::start_hand() POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(!hand_in_progress(), "Hand must not be in progress");

    _betting_rounds_completed = false;
    _round_of_betting = round_of_betting::preflop;
    collect_ante();
    const auto first_action = next_or_wrap(post_blinds());
    deal_hole_cards();
    if (std::count_if(_players.begin(), _players.end(), [] (const auto& p) { return p.stack() != 0; }) > 1) {
        new (&_betting_round) detail::betting_round{_players, first_action, _forced_bets.blinds.big};
    }
    _hand_in_progress = true;
}

inline void dealer::action_taken(action a, chips bet/* = 0*/) POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(betting_round_in_progress(), "Betting round must be in progress");
    POKER_DETAIL_ASSERT(legal_actions().contains(a, bet), "Action must be legal");

    if (static_cast<bool>(a & action::check) || static_cast<bool>(a & action::call)) {
        _betting_round.action_taken(detail::betting_round::action::match);
    } else if (static_cast<bool>(a & action::bet) || static_cast<bool>(a & action::raise)) {
        _betting_round.action_taken(detail::betting_round::action::raise, bet);
    } else {
        assert(static_cast<bool>(a & action::fold));
        _pot_manager.bet_folded(_players[player_to_act()].bet_size());
        _players.exclude_player(player_to_act());
        _betting_round.action_taken(detail::betting_round::action::leave);
    }
}

inline void dealer::end_betting_round() POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(!_betting_rounds_completed, "Betting rounds must not be completed");
    POKER_DETAIL_ASSERT(!betting_round_in_progress(), "Betting round must not be in progress");

    _pot_manager.collect_bets_from(_players);
    if (_betting_round.num_active_players() <= 1) {
        _round_of_betting = round_of_betting::river;
        // If there is only one pot, and there is only one player in it...
        if (_pot_manager.pots().size() == 1 && _pot_manager.pots()[0].eligible_players().size() == 1) {
            // ...there is no need to deal the undealt community cards.
        } else {
            deal_community_cards();
        }
        _betting_rounds_completed = true;
        // Now you call showdown()
    } else if (_round_of_betting < round_of_betting::river) {
        // Start the next betting round.
        _round_of_betting = next(_round_of_betting);
        _players = _betting_round.players();
        new (&_betting_round) detail::betting_round{_players, next_or_wrap(_button), 0};
        deal_community_cards();
        assert(_betting_rounds_completed == false);
    } else {
        assert(_round_of_betting == round_of_betting::river);
        _betting_rounds_completed = true;
        // Now you call showdown()
    }
}

inline void dealer::showdown() POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(_round_of_betting == round_of_betting::river, "Round of betting must be river");
    POKER_DETAIL_ASSERT(!betting_round_in_progress(), "Betting round must not be in progress");
    POKER_DETAIL_ASSERT(betting_rounds_completed(), "Betting rounds must be completed");

    _hand_in_progress = false;
    if (_pot_manager.pots().size() == 1 && _pot_manager.pots()[0].eligible_players().size() == 1) {
        // No need to evaluate the hand. There is only one player.
        const auto index = _pot_manager.pots().front().eligible_players().front();
        _players[index].add_to_stack(_pot_manager.pots().front().size());
        return;

        // TODO: Also, no reveals in this case. Reveals are only necessary when there is >=2 players.
    }
    for (auto& p : _pot_manager.pots()) {
        auto player_results = std::vector<std::pair<seat_index, hand>>{};
        player_results.reserve(p.eligible_players().size());
        std::transform(p.eligible_players().begin(), p.eligible_players().end(), std::back_inserter(player_results), [&] (seat_index i) {
            /* return std::pair{i, hand{_players[i].hole_cards, *_community_cards}}; */
            return std::pair{i, hand{_hole_cards[i], *_community_cards}};
        });
        std::sort(player_results.begin(), player_results.end(), [] (auto&& lhs, auto&& rhs) {
            return lhs.second > rhs.second;
        });
        auto first_winner = player_results.begin();
        auto last_winner = std::adjacent_find(player_results.begin(), player_results.end(), [] (auto&& lhs, auto&& rhs) {
            return lhs.second != rhs.second;
        });
        if (last_winner != player_results.end()) ++last_winner;
        const auto payout = p.size() / static_cast<chips>(std::distance(first_winner, last_winner));
        std::for_each(first_winner, last_winner, [&] (auto&& winner) {
            _players[winner.first].add_to_stack(payout);
        });
    }
}

inline auto dealer::next_or_wrap(seat_index seat) noexcept -> seat_index {
    do {
        ++seat;
        if (seat == max_players) seat = 0;
    } while (!_players.filter()[seat]);
    return seat;
}

inline void dealer::collect_ante() noexcept {
    for (auto& p : _players) {
        p.take_from_stack(std::min(_forced_bets.ante, p.total_chips()));
    }
}

inline auto dealer::post_blinds() noexcept -> seat_index {
    auto seat = _button;
    const auto num_players = std::count(_players.filter().begin(), _players.filter().end(), true);
    if (num_players != 2) seat = next_or_wrap(seat);
    _players[seat].bet(std::min(_forced_bets.blinds.small, _players[seat].total_chips()));
    seat = next_or_wrap(seat);
    _players[seat].bet(std::min(_forced_bets.blinds.big, _players[seat].total_chips()));
    return seat;
}

inline void dealer::deal_hole_cards() noexcept {
    for (auto i = 0; i < max_players; ++i) {
        if (_players.filter()[i]) {
            _hole_cards[i] = {_deck->draw(), _deck->draw()};
        }
    }
}

// Deals community cards up until the current round of betting.
inline void dealer::deal_community_cards() noexcept {
    using poker::detail::to_underlying;
    auto cards = std::vector<card>{};
    const auto num_cards_to_deal = to_underlying(_round_of_betting) - _community_cards->cards().size();
    std::generate_n(std::back_inserter(cards), num_cards_to_deal, [&] { return _deck->draw(); });
    _community_cards->deal(cards);
}

} // namespace poker

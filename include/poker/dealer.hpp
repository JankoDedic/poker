#pragma once

#include <algorithm>
#include <array>
#include <bitset>
#include <new>

#include <doctest/doctest.h>

#include <poker/community_cards.hpp>
#include <poker/deck.hpp>
#include <poker/hand.hpp>
#include <poker/player.hpp>

#include "poker/detail/betting_round.hpp"
#include "poker/detail/pot.hpp"
#include "poker/detail/pot_manager.hpp"
#include "poker/detail/utility.hpp"

namespace poker {

struct blinds {
    chips small = 0;
    chips big = 2*small;
};

struct forced_bets {
    poker::blinds blinds = {};
    chips ante = 0;
};

class dealer {
public:
    //
    // Constants
    //
    static constexpr auto max_players = 9;

    //
    // Types
    //
    using player_container = std::array<player*, max_players>;

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

        auto contains(dealer::action, poker::chips bet = 0) const noexcept -> bool;
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
    template<typename PlayerRange, typename = std::enable_if_t<std::is_same_v<poker::detail::range_value_t<PlayerRange>, player>>>
    dealer(PlayerRange& players, decltype(std::begin(players)) button, forced_bets, deck&, community_cards&) noexcept;

    dealer(const player_container& players, player_container::const_iterator button, forced_bets, deck&, community_cards&) noexcept;

    //
    // Observers
    //
    auto done()               const noexcept -> bool;
    auto player_to_act()      const noexcept -> player_container::const_iterator;
    auto players()            const noexcept -> const player_container&;
    auto round_of_betting()   const noexcept -> poker::round_of_betting;
    auto num_active_players() const noexcept -> std::size_t;
    auto biggest_bet()        const noexcept -> chips;
    auto betting_round_over() const noexcept -> bool;
    auto legal_actions()      const noexcept -> action_range;
    auto pots()               const noexcept -> span<const pot>;
    auto button()             const noexcept -> player_container::const_iterator;

    //
    // Modifiers
    //
    void start_hand()                          noexcept;
    void action_taken(action, chips bet = {0}) noexcept;
    void end_betting_round()                   noexcept;
    void showdown()                            noexcept;

private:
    auto next_or_wrap(player_container::iterator)    noexcept -> player_container::iterator;
    void collect_ante()                              noexcept;
    auto post_blinds()                               noexcept -> player_container::iterator;
    void deal_hole_cards()                           noexcept;
    void deal_community_cards()                      noexcept; // Deals community cards up until the current round of betting.

private:
    // Data members
    player_container           _players = {};
    player_container::iterator _button;

    detail::betting_round      _betting_round;
    forced_bets                _forced_bets;

    deck*                      _deck = nullptr;
    community_cards*           _community_cards = nullptr;

    poker::round_of_betting    _round_of_betting;
    bool                       _betting_round_ended = false;
    detail::pot_manager        _pot_manager = {};
    // store legal action range?
};

inline auto dealer::action_range::contains(dealer::action a, poker::chips bet/* = 0*/) const noexcept -> bool {
    assert(is_valid(a));
    return static_cast<bool>(a & action) && (is_aggressive(a) ? chip_range.contains(bet) : true);
}

inline auto dealer::is_valid(action a) noexcept -> bool {
    return std::bitset<CHAR_BIT>(static_cast<unsigned char>(a)).count() == 1;
}

inline constexpr auto dealer::is_aggressive(action a) noexcept -> bool {
    return static_cast<bool>(a & action::bet) || static_cast<bool>(a & action::raise);
}

template<typename PlayerRange, typename>
dealer::dealer(PlayerRange& players, decltype(std::begin(players)) button, forced_bets fb, deck& d, community_cards& cc) noexcept
    : _forced_bets{fb}
    , _deck{&d}
    , _community_cards{&cc}
    , _round_of_betting{round_of_betting::preflop}
{
    constexpr auto addressof = [] (player& p) { return &p; };
    std::transform(std::begin(players), std::end(players), std::begin(_players), addressof);
    _button = _players.begin() + std::distance(std::begin(players), button);
}

inline dealer::dealer(
    const player_container& players, player_container::const_iterator button, forced_bets fb, deck& d, community_cards& cc) noexcept
    : _forced_bets{fb}
    , _deck{&d}
    , _community_cards{&cc}
    , _round_of_betting{round_of_betting::preflop}
    , _players{players}
    , _button{_players.begin() + std::distance(players.begin(), button)}
{
}

inline auto dealer::done() const noexcept -> bool {
    return _betting_round.over() && _betting_round_ended && _round_of_betting == round_of_betting::river;
}

inline auto dealer::player_to_act()      const noexcept -> player_container::const_iterator { return _betting_round.player_to_act();      }
inline auto dealer::players()            const noexcept -> const player_container&          { return _betting_round.players();            }
inline auto dealer::round_of_betting()   const noexcept -> poker::round_of_betting          { return _round_of_betting;                   }
// TODO: What happens when d.done() ? Do we assert or return a special value? Special value sounds bad.
inline auto dealer::num_active_players() const noexcept -> std::size_t                      { return _betting_round.num_active_players(); }
inline auto dealer::biggest_bet()        const noexcept -> chips                            { return _betting_round.biggest_bet();        }
inline auto dealer::betting_round_over() const noexcept -> bool                             { return _betting_round.over();               }

inline auto dealer::legal_actions() const noexcept -> action_range {
    const auto& player = **_betting_round.player_to_act();
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

inline auto dealer::pots() const noexcept -> span<const pot> {
    return _pot_manager.pots();
}

inline auto dealer::button() const noexcept -> player_container::const_iterator {
    return _button;
}

inline void dealer::start_hand() noexcept {
    collect_ante();
    const auto first_action = next_or_wrap(post_blinds());
    deal_hole_cards();
    if (std::count_if(_players.begin(), _players.end(), [] (auto p) { return p && p->stack() != 0; }) > 1) {
        new (&_betting_round) detail::betting_round{_players, first_action, _forced_bets.blinds.big};
    }
}

inline void dealer::action_taken(action a, chips bet/* = 0*/) noexcept {
    assert(!betting_round_over());
    assert(legal_actions().contains(a, bet));

    if (static_cast<bool>(a & action::check) || static_cast<bool>(a & action::call)) {
        _betting_round.action_taken(detail::betting_round::action::match);
    } else if (static_cast<bool>(a & action::bet) || static_cast<bool>(a & action::raise)) {
        _betting_round.action_taken(detail::betting_round::action::raise, bet);
    } else {
        assert(static_cast<bool>(a & action::fold));
        _pot_manager.bet_folded((*player_to_act())->bet_size());
        const auto folded_player_index = std::distance(std::begin(players()), player_to_act());
        _players[folded_player_index] = nullptr;
        _betting_round.action_taken(detail::betting_round::action::leave);
    }
}

inline void dealer::end_betting_round() noexcept {
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
        new (&_betting_round) detail::betting_round{_players, next_or_wrap(_button), 0};
        deal_community_cards();
        // _betting_round_ended = false;
    } else {
        assert(_round_of_betting == round_of_betting::river);
        _betting_round_ended = true;
        // Now you call showdown()
    }
}

inline void dealer::showdown() noexcept {
    if (_pot_manager.pots().size() == 1 && _pot_manager.pots()[0].eligible_players().size() == 1) {
        // No need to evaluate the hand. There is only one player.
        _pot_manager.pots()[0].eligible_players()[0]->add_to_stack(_pot_manager.pots()[0].size());
        return;

        // TODO: Also, no reveals in this case. Reveals are only necessary when there is >=2 players.
    }
    for (auto p : _pot_manager.pots()) {
        auto player_results = std::vector<std::pair<player*, hand>>{};
        player_results.reserve(p.eligible_players().size());
        std::transform(p.eligible_players().begin(), p.eligible_players().end(), std::back_inserter(player_results), [&] (player* p) {
            return std::pair{p, hand{p->hole_cards, *_community_cards}};
        });
        std::sort(player_results.begin(), player_results.end(), [] (auto&& lhs, auto&& rhs) {
            return lhs.second < rhs.second;
        });
        auto first_winner = player_results.begin();
        auto last_winner = std::adjacent_find(player_results.begin(), player_results.end(), [] (auto&& lhs, auto&& rhs) {
            return lhs.second != rhs.second;
        });
        if (last_winner != player_results.end()) ++last_winner;
        const auto payout = p.size() / static_cast<chips>(std::distance(first_winner, last_winner));
        std::for_each(first_winner, last_winner, [&] (auto&& winner) {
            winner.first->add_to_stack(payout);
        });
    }
}

inline auto dealer::next_or_wrap(player_container::iterator it) noexcept -> player_container::iterator {
    do {
        ++it;
        if (it == _players.end()) it = _players.begin();
    } while (!*it);
    return it;
}

inline void dealer::collect_ante() noexcept {
    for (auto p : _players) {
        if (p) p->take_from_stack(std::min(_forced_bets.ante, p->total_chips()));
    }
}

inline auto dealer::post_blinds() noexcept -> player_container::iterator {
    auto it = _button;
    const auto num_players = std::count_if(std::begin(_players), std::end(_players), [] (player* p) { return p != nullptr; });
    if (num_players != 2) it = next_or_wrap(it);
    (**it).bet(std::min(_forced_bets.blinds.small, (**it).total_chips()));
    it = next_or_wrap(it);
    (**it).bet(std::min(_forced_bets.blinds.big, (**it).total_chips()));
    return it;
}

inline void dealer::deal_hole_cards() noexcept {
    for (auto p : _players) {
        if (p) p->hole_cards = {_deck->draw(), _deck->draw()};
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

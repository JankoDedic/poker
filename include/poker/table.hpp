#pragma once

#include <poker/dealer.hpp>

#include "poker/detail/error.hpp"
#include "poker/detail/utility.hpp"

namespace poker {

using action = dealer::action;

/* template<std::size_t num_seats> */
class table {
public:
    //
    // Constants
    //
    static constexpr auto num_seats = std::size_t{9};

    //
    // Types
    //
    enum class automatic_action {
        fold       = 1 << 0,
        check_fold = 1 << 1,
        check      = 1 << 2,
        call       = 1 << 3,
        call_any   = 1 << 4,
        all_in     = 1 << 5
    };
    POKER_DETAIL_DEFINE_FRIEND_FLAG_OPERATIONS(automatic_action)

    //
    // Special functions
    //
    table() = default;
    table(const table&) = delete;
    table(table&&)      = delete;
    auto operator=(const table&) -> table& = delete;
    auto operator=(table&&)      -> table& = delete;

    //
    // Constructors
    //
    explicit table(poker::forced_bets) noexcept;

    //
    // Observers
    //
    auto seats() const noexcept -> const seat_array&;
    auto forced_bets() const noexcept -> poker::forced_bets;

    // Dealer
    auto hand_in_progress()          const noexcept       -> bool;
    auto betting_round_in_progress() const POKER_NOEXCEPT -> bool;
    auto betting_rounds_completed()  const POKER_NOEXCEPT -> bool;
    auto hand_players()              const POKER_NOEXCEPT -> seat_array_view;
    auto button()                    const POKER_NOEXCEPT -> seat_index;
    auto player_to_act()             const POKER_NOEXCEPT -> seat_index;
    auto num_active_players()        const POKER_NOEXCEPT -> std::size_t;
    auto pots()                      const POKER_NOEXCEPT -> span<const pot>;
    auto round_of_betting()          const POKER_NOEXCEPT -> poker::round_of_betting;
    auto community_cards()           const POKER_NOEXCEPT -> const poker::community_cards&;
    auto legal_actions()             const POKER_NOEXCEPT -> dealer::action_range;
    auto hole_cards()                const POKER_NOEXCEPT -> slot_view<const poker::hole_cards, num_seats>;

    // Automatic actions
    auto automatic_actions()                  const POKER_NOEXCEPT -> span<const std::optional<automatic_action>, num_seats>;
    auto can_set_automatic_action(seat_index) const POKER_NOEXCEPT -> bool;
    auto legal_automatic_actions(seat_index)  const POKER_NOEXCEPT -> automatic_action;

    //
    // Modifiers
    //
    void set_forced_bets(poker::forced_bets) POKER_NOEXCEPT;

    // Adding/removing players
    void sit_down(seat_index, chips buy_in) POKER_NOEXCEPT;
    void stand_up(seat_index) POKER_NOEXCEPT;

    // Dealer
    template<class URBG> void start_hand(URBG&&) POKER_NOEXCEPT;
    template<class URBG> void start_hand(URBG&&, seat_index) POKER_NOEXCEPT;
    void action_taken(action, chips bet = 0) POKER_NOEXCEPT;
    void end_betting_round() POKER_NOEXCEPT;
    void showdown() POKER_NOEXCEPT;

    // Automatic actions
    void set_automatic_action(seat_index, automatic_action);

private:
    void take_automatic_action(automatic_action) noexcept;
    void amend_automatic_actions() noexcept;
    void act_passively() noexcept;
    void increment_button() noexcept;
    void update_table_players() noexcept;
    auto single_active_player_remaining() const noexcept -> bool;

private:
    seat_array _hand_players;
    bool                                                  _first_time_button = true;
    bool                                                  _button_set_manually = false; // has the button been set manually
    seat_index _button = 0;
    poker::forced_bets                                    _forced_bets       = {};
    deck                                                  _deck;
    poker::community_cards                                _community_cards;
    dealer                                                _dealer;

    // All the players physically present at the table
    seat_array _table_players;
    // All players who took a seat before the .start_hand()
    std::array<bool,num_seats>                            _staged = {};
    //std::array<bool,num_seats>                            _sitting_out = {}; // NOT USED
    std::array<std::optional<automatic_action>,num_seats> _automatic_actions;
};

inline table::table(poker::forced_bets fb) noexcept
    : _forced_bets{fb}
{
}

inline void table::take_automatic_action(automatic_action a) noexcept {
    const auto& player = _hand_players[_dealer.player_to_act()];
    const auto biggest_bet = _dealer.biggest_bet();
    const auto bet_gap = biggest_bet - player.bet_size();
    const auto total_chips = player.total_chips();
    switch (a) {
    case automatic_action::fold:       return _dealer.action_taken(action::fold);
    case automatic_action::check_fold: return _dealer.action_taken(bet_gap == 0 ? action::check : action::fold);
    case automatic_action::check:      return _dealer.action_taken(action::check);
    case automatic_action::call:       return _dealer.action_taken(action::call);
    case automatic_action::call_any:   return _dealer.action_taken(bet_gap == 0 ? action::check : action::call);
    case automatic_action::all_in:
        if (total_chips < biggest_bet) {
            return _dealer.action_taken(action::call);
        } else {
            return _dealer.action_taken(action::raise, total_chips);
        }
        break;
    default:
        assert(false);
    }
}

inline void table::amend_automatic_actions() noexcept {
    // fold, all_in -- no need to fallback, always legal
    // check_fold, check -- (if the bet_gap becomes >0 then check is no longer legal)
    // call -- you cannot lose your ability to call if you were able to do it in the first place
    // call_any -- you can lose your ability to call_any, which only leaves the normal call (doubt cleared)
    //          condition: biggest_bet >= total_chips
    const auto biggest_bet = _dealer.biggest_bet();
    for (auto s = seat_index{0}; s < num_seats; ++s) {
        if (auto& aa = _automatic_actions[s]) {
            const auto& player = _hand_players[s];
            const auto bet_gap = biggest_bet - player.bet_size();
            const auto total_chips = player.total_chips();
            if (static_cast<bool>(*aa & automatic_action::check_fold) && bet_gap > 0) {
                *aa = automatic_action::fold;
            } else if (static_cast<bool>(*aa & automatic_action::check) && bet_gap > 0) {
                aa = std::nullopt;
            } else if (static_cast<bool>(*aa & automatic_action::call_any) && biggest_bet >= total_chips) {
                *aa = automatic_action::call;
            }
        }
    }
}

inline auto table::player_to_act() const POKER_NOEXCEPT -> seat_index {
    POKER_DETAIL_ASSERT(betting_round_in_progress(), "Betting round must be in progress");

    return _dealer.player_to_act();
}

inline auto table::button() const POKER_NOEXCEPT -> seat_index {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _button;
}

inline auto table::seats() const noexcept -> const seat_array& {
    return _table_players;
}

inline auto table::hand_players() const POKER_NOEXCEPT -> seat_array_view {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _dealer.players();
}

inline auto table::num_active_players() const POKER_NOEXCEPT -> std::size_t {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _dealer.num_active_players();
}

inline auto table::pots() const POKER_NOEXCEPT -> span<const pot> {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _dealer.pots();
}

inline auto table::forced_bets() const noexcept -> poker::forced_bets {
    return _forced_bets;
}

inline void table::set_forced_bets(poker::forced_bets fb) POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(!hand_in_progress(), "Hand must not be in progress");

    _forced_bets = fb;
}

inline void table::increment_button() noexcept {
    if (_button_set_manually) {
        _button_set_manually = false;
        _first_time_button = false;
    } else if (_first_time_button) {
        auto seat = seat_index{_hand_players.begin().index()};
        assert(seat != num_seats);
        _button = seat;
        _first_time_button = false;
    } else {
        auto it = seat_array::iterator{_hand_players, _button};
        ++it;
        if (it.index() == num_seats) {
            _button = _hand_players.begin().index();
        } else {
            _button = it.index();
        }
    }
}

inline void table::update_table_players() noexcept {
    for (auto s = seat_index{0}; s < num_seats; ++s) {
        if (!_staged[s] && _hand_players.occupancy()[s]) {
            assert(_table_players.occupancy()[s]);
            _table_players[s] = _hand_players[s];
        }
    }
}

// A player is considered active (in class table context) when
// he is still within betting_round::players and did not stand up in this hand (!_staged[i]).
// We need the second condition for the players who stood up, but are still technically
// in the betting_round because betting_round does not handle players disappearing.
inline auto table::single_active_player_remaining() const noexcept -> bool {
    assert(betting_round_in_progress());

    const auto& occupancy = _dealer.players().filter();
    auto active_player_count = 0;
    for (auto i = seat_index{0}; i < num_seats; ++i) {
        active_player_count += (occupancy[i] && !_staged[i]);
    }
    return active_player_count == 1;
}

template<class URBG>
inline void table::start_hand(URBG&& g) POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(!hand_in_progress(), "Hand must not be in progress");
    POKER_DETAIL_ASSERT(
        std::count(_table_players.occupancy().begin(), _table_players.occupancy().end(), true) >= 2,
        "There must be at least 2 players at the table"
        );

    _staged = {};
    _automatic_actions = {};
    _hand_players = _table_players;
    increment_button();
    _deck = {std::forward<URBG>(g)};
    _community_cards = {};
    new (&_dealer) dealer{_hand_players, _button, _forced_bets, _deck, _community_cards};
    _dealer.start_hand();
    update_table_players();
}

template<class URBG>
inline void table::start_hand(URBG&& g, seat_index s) POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(s <= num_seats, "Given seat index must be valid");
    POKER_DETAIL_ASSERT(_table_players.occupancy()[s], "Given seat must be occupied");
    // other overload will assert the rest

    _button = s;
    _button_set_manually = true;
    start_hand(std::forward<URBG>(g));
}

inline auto table::hand_in_progress() const noexcept -> bool {
    return _dealer.hand_in_progress();
}

inline auto table::betting_round_in_progress() const POKER_NOEXCEPT -> bool {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _dealer.betting_round_in_progress();
}

inline auto table::betting_rounds_completed() const POKER_NOEXCEPT -> bool {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _dealer.betting_rounds_completed();
}

inline auto table::round_of_betting() const POKER_NOEXCEPT -> poker::round_of_betting {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _dealer.round_of_betting();
}

inline auto table::community_cards() const POKER_NOEXCEPT -> const poker::community_cards& {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _community_cards;
}

inline auto table::legal_actions() const POKER_NOEXCEPT -> dealer::action_range {
    POKER_DETAIL_ASSERT(betting_round_in_progress(), "Betting round must be in progress");

    return _dealer.legal_actions();
}

inline auto table::hole_cards() const POKER_NOEXCEPT -> slot_view<const poker::hole_cards, num_seats> {
    POKER_DETAIL_ASSERT(hand_in_progress() || betting_rounds_completed(), "Hand must be in progress or showdown must have ended");

    return _dealer.hole_cards();
}

inline void table::action_taken(action a, chips bet) POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(betting_round_in_progress(), "Betting round must be in progress");

    _dealer.action_taken(a, bet);
    while (_dealer.betting_round_in_progress()) {
        amend_automatic_actions();
        if (auto& ac = _automatic_actions[player_to_act()]) {
            take_automatic_action(*ac);
            ac = std::nullopt; // Clear the automatic action out, once it has been used.
        } else {
            break;
        }
    }

    if (betting_round_in_progress() && single_active_player_remaining()) {
        // We only need to take action for this one player, and the other automatic actions will unfold automatically.
        act_passively();
    }

    update_table_players();
}

inline void table::end_betting_round() POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(!betting_round_in_progress(), "Betting round must not be in progress");
    POKER_DETAIL_ASSERT(!betting_rounds_completed(), "Betting rounds must not be completed");

    _dealer.end_betting_round();
    amend_automatic_actions();
    update_table_players();
}

inline void table::showdown() POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(!betting_round_in_progress(), "Betting round must not be in progress");
    POKER_DETAIL_ASSERT(betting_rounds_completed(), "Betting rounds must be completed");

    _dealer.showdown();
    update_table_players();
}

inline auto table::automatic_actions() const POKER_NOEXCEPT -> span<const std::optional<automatic_action>, num_seats> {
    POKER_DETAIL_ASSERT(hand_in_progress(), "Hand must be in progress");

    return _automatic_actions;
}

inline auto table::can_set_automatic_action(seat_index s) const POKER_NOEXCEPT -> bool {
    POKER_DETAIL_ASSERT(betting_round_in_progress(), "Betting round must be in progress");

    // (1) This is only ever true for players that have been in the hand since the start.
    // Every following sit-down is accompanied by a _staged[s] = true
    // (2) If a player is not seated at the table, he obviously cannot set his automatic actions.
    /* return !_staged[s] && _table_players[s]; */
    return !_staged[s] && _table_players.occupancy()[s];
}

inline auto table::legal_automatic_actions(seat_index s) const POKER_NOEXCEPT -> automatic_action {
    POKER_DETAIL_ASSERT(can_set_automatic_action(s), "Player must be allowed to set automatic actions");

    // fold, all_in -- always viable
    // check, check_fold -- viable when biggest_bet - bet_size == 0
    // call -- when biggest_bet - bet_size > 0 ("else" of the previous case)
    // call_only -- available always except when biggest_bet >= total_chips (no choice/"any" then)
    //
    // fallbacks:
    // check_fold -> fold
    // check -> nullopt
    // call_any -> check
    const auto biggest_bet = _dealer.biggest_bet();
    const auto& player = _table_players[s];
    const auto bet_size = player.bet_size();
    const auto total_chips = player.total_chips();
    auto legal_actions = automatic_action::fold | automatic_action::all_in;
    const auto can_check = biggest_bet - bet_size == 0;
    if (can_check) {
        legal_actions |= automatic_action::check_fold | automatic_action::check;
    } else {
        legal_actions |= automatic_action::call;
    }
    if (biggest_bet < total_chips) {
        legal_actions |= automatic_action::call_any;
    }
    return legal_actions;
}

inline void table::set_automatic_action(seat_index s, automatic_action a) {
    POKER_DETAIL_ASSERT(can_set_automatic_action(s), "Player must be allowed to set automatic actions");
    POKER_DETAIL_ASSERT(s != player_to_act(), "Player must not be the player to act");
    POKER_DETAIL_ASSERT(std::bitset<CHAR_BIT>(static_cast<unsigned char>(a)).count() == 1, "Player must pick one automatic action");
    POKER_DETAIL_ASSERT(static_cast<bool>(a & legal_automatic_actions(s)), "Given automatic action must be legal");

    _automatic_actions[s] = a;
}

inline void table::sit_down(seat_index s, chips buy_in) POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(s < table::num_seats, "Given seat index must be valid");
    POKER_DETAIL_ASSERT(!_table_players.occupancy()[s], "Given seat must not be occupied");

    _table_players.add_player(s, player{buy_in});
    _staged[s] = true;
}

// Make the current player act passively:
// - check if possible or;
// - call if possible.
inline void table::act_passively() noexcept {
    const auto legal_actions = _dealer.legal_actions();
    if (static_cast<bool>(legal_actions.action & action::check)) {
        action_taken(action::check);
    } else {
        assert(static_cast<bool>(legal_actions.action & action::call));
        action_taken(action::call);
    }
}

// TODO: return chips?
inline void table::stand_up(seat_index s) POKER_NOEXCEPT {
    POKER_DETAIL_ASSERT(s < table::num_seats, "Given seat index must be valid");
    POKER_DETAIL_ASSERT(_table_players.occupancy()[s], "Given seat must be occupied");

    if (hand_in_progress()) {
        assert(betting_round_in_progress());
        if (s == player_to_act()) {
            action_taken(action::fold);

            _table_players.remove_player(s);
            _staged[s] = true;
        } else if (_hand_players.occupancy()[s]) {
            set_automatic_action(s, automatic_action::fold);

            _table_players.remove_player(s);
            _staged[s] = true;

            if (single_active_player_remaining()) {
                // We only need to take action for this one player, and the other automatic actions will unfold automatically.
                act_passively();
            }
        }
    } else {
        _table_players.remove_player(s);
    }
}

} // namespace poker

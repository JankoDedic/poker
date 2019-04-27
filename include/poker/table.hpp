#pragma once

#include <poker/dealer.hpp>

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
    using seat = std::size_t;

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
    auto seats()       const noexcept -> span<const std::optional<player>, num_seats>;
    auto forced_bets() const noexcept -> poker::forced_bets;

    // Dealer
    auto hand_in_progress()          const noexcept -> bool;
    auto betting_round_in_progress() const noexcept -> bool;
    auto betting_rounds_completed()  const noexcept -> bool;
    auto hand_players()              const noexcept -> span<player* const, num_seats>;
    auto button()                    const noexcept -> seat;
    auto player_to_act()             const noexcept -> seat;
    auto num_active_players()        const noexcept -> std::size_t;
    auto pots()                      const noexcept -> span<const pot>;
    auto round_of_betting()          const noexcept -> poker::round_of_betting;
    auto community_cards()           const noexcept -> const poker::community_cards&;

    // Automatic actions
    auto automatic_actions()            const noexcept -> span<const std::optional<automatic_action>, num_seats>;
    auto can_set_automatic_action(seat) const noexcept -> bool;
    auto legal_automatic_actions(seat)  const noexcept -> automatic_action;

    //
    // Modifiers
    //
    void set_forced_bets(poker::forced_bets) noexcept;

    // Adding/removing players
    void sit_down(seat, chips buy_in) noexcept;
    void stand_up(seat) noexcept;

    // Dealer
    template<class URBG> void start_hand(URBG&&) noexcept;
    void action_taken(action, chips bet = {0})   noexcept;
    void end_betting_round()                     noexcept;
    void showdown()                              noexcept;

    // Automatic actions
    void set_automatic_action(seat, automatic_action);

private:
    void take_automatic_action(automatic_action) noexcept;
    void amend_automatic_actions() noexcept;
    void act_passively() noexcept;
    void increment_button() noexcept;
    void update_table_players() noexcept;

private:
    std::array<std::optional<player>,num_seats>           _hand_players;
    bool                                                  _first_time_button = true;
    std::array<std::optional<player>,num_seats>::iterator _button            = std::begin(_hand_players);
    poker::forced_bets                                    _forced_bets       = {};
    deck                                                  _deck;
    poker::community_cards                                _community_cards;
    dealer                                                _dealer;

    // All the players physically present at the table
    std::array<std::optional<player>,num_seats>           _table_players;
    // All players who took a seat before the .start_hand()
    std::array<bool,num_seats>                            _staged = {};
    std::array<bool,num_seats>                            _sitting_out = {};
    std::array<std::optional<automatic_action>,num_seats> _automatic_actions;
};

inline table::table(poker::forced_bets fb) noexcept
    : _forced_bets{fb}
{
}

inline void table::take_automatic_action(automatic_action a) noexcept {
    const auto& player = **_dealer.player_to_act();
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
    for (auto s = seat{0}; s < num_seats; ++s) {
        if (auto& aa = _automatic_actions[s]) {
            const auto& player = *_hand_players[s];
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

inline auto table::player_to_act() const noexcept -> seat {
    assert(betting_round_in_progress());

    return static_cast<std::size_t>(std::distance(std::cbegin(_dealer.players()), _dealer.player_to_act()));
}

inline auto table::button() const noexcept -> seat {
    assert(hand_in_progress());

    return static_cast<std::size_t>(std::distance(std::cbegin(_dealer.players()), _dealer.button()));
}

inline auto table::seats() const noexcept -> span<const std::optional<player>, num_seats> {
    return _table_players;
}

inline auto table::hand_players() const noexcept -> span<player* const, num_seats> {
    assert(hand_in_progress());

    return _dealer.players();
}

inline auto table::num_active_players() const noexcept -> std::size_t {
    assert(hand_in_progress());

    return _dealer.num_active_players();
}

inline auto table::pots() const noexcept -> span<const pot> {
    assert(hand_in_progress());

    return _dealer.pots();
}

inline auto table::forced_bets() const noexcept -> poker::forced_bets {
    return _forced_bets;
}

inline void table::set_forced_bets(poker::forced_bets fb) noexcept {
    assert(!hand_in_progress());

    _forced_bets = fb;
}

inline void table::increment_button() noexcept {
    if (_first_time_button) {
        auto it = std::find_if(std::begin(_hand_players), std::end(_hand_players), [] (auto&& p) { return !!p; });
        assert(it != std::end(_hand_players)); // There has to be at least one other valid player in every case.
        _button = it;
        _first_time_button = false;
        return;
    }
    do {
        ++_button;
        if (_button == std::end(_hand_players)) {
            _button = std::begin(_hand_players);
        }
    } while (*_button == std::nullopt);
}

inline void table::update_table_players() noexcept {
    for (auto s = seat{0}; s < num_seats; ++s) {
        if (!_staged[s] && _hand_players[s]) {
            _table_players[s] = _hand_players[s];
        }
    }
}

template<class URBG>
inline void table::start_hand(URBG&& g) noexcept {
    assert(!hand_in_progress());
    assert(std::count_if(std::cbegin(_table_players), std::cend(_table_players), [] (auto&& p) { return !!p; }) >= 2);

    _staged = {};
    _automatic_actions = {};
    _hand_players = _table_players;
    increment_button();
    _deck = {std::forward<URBG>(g)};
    auto players = std::array<player*, num_seats>{};
    std::transform(std::begin(_hand_players), std::end(_hand_players), std::begin(players), [] (auto& p) -> player* {
        if (p) return &*p;
        else return nullptr;
    });
    new (&_dealer) dealer{players, std::begin(players) + distance(std::begin(_hand_players), _button), _forced_bets, _deck, _community_cards};
    _dealer.start_hand();
    update_table_players();
}

inline auto table::hand_in_progress() const noexcept -> bool {
    return _dealer.hand_in_progress();
}

inline auto table::betting_round_in_progress() const noexcept -> bool {
    assert(hand_in_progress());

    return _dealer.betting_round_in_progress();
}

inline auto table::betting_rounds_completed() const noexcept -> bool {
    assert(!betting_round_in_progress());

    return _dealer.betting_rounds_completed();
}

inline auto table::round_of_betting() const noexcept -> poker::round_of_betting {
    assert(hand_in_progress());

    return _dealer.round_of_betting();
}

inline auto table::community_cards() const noexcept -> const poker::community_cards& {
    assert(hand_in_progress());

    return _community_cards;
}

inline void table::action_taken(action a, chips bet) noexcept {
    assert(betting_round_in_progress());

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
    update_table_players();
}

inline void table::end_betting_round() noexcept {
    assert(!betting_round_in_progress());
    assert(!betting_rounds_completed());

    _dealer.end_betting_round();
    amend_automatic_actions();
    update_table_players();
}

inline void table::showdown() noexcept {
    assert(!betting_round_in_progress());
    assert(betting_rounds_completed());

    _dealer.showdown();
    update_table_players();
}

inline auto table::automatic_actions() const noexcept -> span<const std::optional<automatic_action>, num_seats> {
    assert(hand_in_progress());

    return _automatic_actions;
}

inline auto table::can_set_automatic_action(seat s) const noexcept -> bool {
    assert(betting_round_in_progress());

    // (1) This is only ever true for players that have been in the hand since the start.
    // Every following sit-down is accompanied by a _staged[s] = true
    // (2) If a player is not seated at the table, he obviously cannot set his automatic actions.
    return !_staged[s] && _table_players[s];
}

inline auto table::legal_automatic_actions(seat s) const noexcept -> automatic_action {
    assert(can_set_automatic_action(s));

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
    const auto& player = *_table_players[s];
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

inline void table::set_automatic_action(seat s, automatic_action a) {
    assert(can_set_automatic_action(s));
    assert(s != player_to_act());
    assert(std::bitset<CHAR_BIT>(static_cast<unsigned char>(a)).count() == 1);
    assert(static_cast<bool>(a & legal_automatic_actions(s)));

    _automatic_actions[s] = a;
}

inline void table::sit_down(seat s, chips buy_in) noexcept {
    assert(s >= 0);
    assert(s < table::num_seats);
    assert(!_table_players[s]);

    _table_players[s] = player(buy_in);
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
inline void table::stand_up(seat s) noexcept {
    assert(s >= 0);
    assert(s < table::num_seats);
    assert(_table_players[s]);

    if (hand_in_progress()) {
        assert(betting_round_in_progress());
        if (s == player_to_act()) {
            action_taken(action::fold);
        } else if (_hand_players[s]) {
            set_automatic_action(s, automatic_action::fold);
        }

        _table_players[s] = std::nullopt;
        _staged[s] = true;

        const auto player_count = std::count_if(std::cbegin(_table_players), std::cend(_table_players), [] (auto&& p) { return !!p; });
        if (player_count == 1) {
            // We only need to take action for this one player, and the other automatic actions will unfold automatically.
            act_passively();
        }
    } else {
        _table_players[s] = std::nullopt;
    }
}

} // namespace poker

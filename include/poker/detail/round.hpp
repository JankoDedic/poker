#pragma once

#include <algorithm>
#include <cassert>

#include <poker/seat_index.hpp>
#include "poker/detail/utility.hpp"

namespace poker::detail {

class round {
public:
    //
    // Constants
    //
    static constexpr auto num_players = std::size_t{9};

    //
    // Types
    //
    enum class action {
        leave      = 1 << 0,
        passive    = 1 << 1,
        aggressive = 1 << 2
    };
    POKER_DETAIL_DEFINE_FRIEND_FLAG_OPERATIONS(action)

    //
    // Constructors
    //
    round() = default;
    round(const std::array<bool, num_players>& active_players, seat_index first_to_act) noexcept;

    //
    // Observers
    //
    auto active_players()        const noexcept -> const std::array<bool,num_players>&;
    auto player_to_act()         const noexcept -> seat_index;
    auto last_aggressive_actor() const noexcept -> seat_index;
    auto num_active_players()    const noexcept -> std::size_t;
    auto in_progress()           const noexcept -> bool;

    //
    // Modifiers
    //
    void action_taken(action) noexcept;

    // Used for testing betting_round.
    friend auto operator==(const round&, const round&) noexcept -> bool;

private:
    void increment_player() noexcept;

private:
    std::array<bool,num_players> _active_players     = {};
    seat_index                   _player_to_act;
    seat_index                   _last_aggressive_actor;
    bool                         _contested          = false;      // passive or aggressive action was taken this round
    bool                         _first_action       = true;
    std::size_t                  _num_active_players = 0;
};

inline auto operator==(const round& x, const round& y) noexcept -> bool {
    return x._active_players        == y._active_players
        && x._player_to_act         == y._player_to_act
        && x._last_aggressive_actor == y._last_aggressive_actor
        && x._contested             == y._contested
        && x._num_active_players    == y._num_active_players;
}

inline round::round(const std::array<bool, num_players>& active_players, seat_index first_to_act) noexcept
    : _active_players{active_players}
    , _player_to_act{first_to_act}
    , _last_aggressive_actor{first_to_act}
    , _num_active_players{static_cast<std::size_t>(std::count(std::cbegin(active_players), std::cend(active_players), true))}
{
    assert(first_to_act < num_players);
}

inline auto round::active_players() const noexcept -> const std::array<bool,num_players>& {
    return _active_players;
}

inline auto round::player_to_act() const noexcept -> seat_index {
    return _player_to_act;
}

inline auto round::last_aggressive_actor() const noexcept -> seat_index {
    return _last_aggressive_actor;
}

inline auto round::num_active_players() const noexcept -> std::size_t {
    return _num_active_players;
}

inline auto round::in_progress() const noexcept -> bool {
    return (_contested || _num_active_players > 1) && (_first_action || _player_to_act != _last_aggressive_actor);
}

inline void round::action_taken(action a) noexcept {
    assert(in_progress());
    assert(!(static_cast<bool>(a & action::passive) && static_cast<bool>(a & action::aggressive)));
    if (_first_action) _first_action = false;
    // Implication: if there is aggressive action => the next player is contested
    if (static_cast<bool>(a & action::aggressive)) {
        _last_aggressive_actor = _player_to_act;
        _contested = true;
    } else if (static_cast<bool>(a & action::passive)) {
        _contested = true;
    }
    if (static_cast<bool>(a & action::leave)) {
        _active_players[_player_to_act] = false;
        --_num_active_players;
    }
    increment_player();
}

inline void round::increment_player() noexcept {
    do {
        ++_player_to_act;
        if (_player_to_act == num_players) _player_to_act = 0;
        if (_player_to_act == _last_aggressive_actor) break;
    } while (!_active_players[_player_to_act]);
}

} // namespace poker::detail

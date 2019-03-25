#pragma once

#include <poker/player.hpp>

#include "poker/detail/utility.hpp"

namespace poker::detail {

class round {
public:
    //
    // Constants
    //
    static constexpr auto max_players = 9;

    //
    // Types
    //
    using player_container = std::array<player*, max_players>;

    enum class action {
        leave      = 1 << 0,
        passive    = 1 << 1,
        aggressive = 1 << 2
    };
    POKER_DETAIL_DEFINE_FRIEND_FLAG_OPERATIONS(action)

    //
    // Special functions
    //
    round()              = default;
    round(const round&)  = delete;
    round(round&&)       = delete;
    auto operator=(const round&) -> round& = delete;
    auto operator=(round&&)      -> round& = delete;

    //
    // Constructors
    //
    round(const player_container& players, player_container::const_iterator current) noexcept;

    //
    // Observers
    //
    auto players()               const noexcept -> const player_container&;
    auto player_to_act()         const noexcept -> player_container::const_iterator;
    auto last_aggressive_actor() const noexcept -> player_container::const_iterator;
    auto num_active_players()    const noexcept -> std::size_t;
    auto over()                  const noexcept -> bool;

    //
    // Modifiers
    //
    void action_taken(action) noexcept;

    // Used for testing betting_round.
    friend auto operator==(const round&, const round&) noexcept -> bool;

private:
    void increment_player() noexcept;

private:
    player_container                 _players = {};
    player_container::iterator       _player_to_act;
    player_container::const_iterator _last_aggressive_actor;
    bool                             _contested = false;      // passive or aggressive action was taken this round
    bool                             _first_action = true;
    std::size_t                      _num_active_players = 0;
};

inline round::round(const player_container& players, player_container::const_iterator current) noexcept
    : _players(players)
    , _player_to_act(_players.begin() + std::distance(players.begin(), current))
    , _last_aggressive_actor(_player_to_act)
    , _num_active_players(std::count_if(begin(_players), end(_players), [] (player* p) { return p != nullptr; }))
{
}

inline auto round::players() const noexcept -> const player_container& {
    return _players;
}

inline auto round::player_to_act() const noexcept -> player_container::const_iterator {
    assert(!over());
    return _player_to_act;
}

inline auto round::last_aggressive_actor() const noexcept -> player_container::const_iterator {
    return _last_aggressive_actor;
}

inline auto round::num_active_players() const noexcept -> std::size_t {
    return _num_active_players;
}

inline auto round::over() const noexcept -> bool {
    return (!_contested && _num_active_players <= 1) || (!_first_action && _player_to_act == _last_aggressive_actor);
}

inline void round::action_taken(action a) noexcept {
    assert(!over());
    if (_first_action) _first_action = false;
    assert(!(bool(a & action::passive) && bool(a & action::aggressive)));
    // Implication: if there is aggressive action => the next player is contested
    if (bool(a & action::aggressive)) {
        _last_aggressive_actor = _player_to_act;
        _contested = true;
    } else if (bool(a & action::passive)) {
        _contested = true;
    }
    if (bool(a & action::leave)) {
        *_player_to_act = nullptr;
        --_num_active_players;
    }
    increment_player();
}

inline auto operator==(const round& x, const round& y) noexcept -> bool {
    return x._players == y._players
        && (x._player_to_act - x._players.begin()) == (y._player_to_act - y._players.begin())
        && (x._last_aggressive_actor - x._players.begin()) == (y._last_aggressive_actor - y._players.begin())
        && x._contested == y._contested
        && x._num_active_players == y._num_active_players;
}

inline void round::increment_player() noexcept {
    do {
        ++_player_to_act;
        if (_player_to_act == _players.end()) _player_to_act = _players.begin();
        if (_player_to_act == _last_aggressive_actor) break;
    } while (*_player_to_act == nullptr);
}

} // namespace poker::detail

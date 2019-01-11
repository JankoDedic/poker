#pragma once

#include <array>
#include <algorithm>

#include <poker/deck.hpp>
#include <poker/player.hpp>
#include <poker/pot.hpp>

namespace poker {

enum class action_type { fold, check, call, bet, raise };

bool
is_aggressive(action_type atype) noexcept
{
    return atype >= action_type::bet;
}

class action {
    action_type _type;
    chips _bet;

public:
    action() noexcept = default;

    action(action_type type)
        : _type(type)
    {
        assert(!is_aggressive(type));
    }

    action(action_type type, chips bet)
        : _type(type)
        , _bet(bet)
    {
        assert(is_aggressive(type));
        assert(bet > 0);
    }

    action_type
    type() const
    {
        return _type;
    }

    chips
    bet_size() const
    {
        assert(is_aggressive(_type));
        return _bet;
    }
};

bool
is_aggressive(action a) noexcept
{
    return is_aggressive(a.type());
}

enum class valid_action_types {
    check_bet,
    check_raise,
    call_raise,
    call_only
};

class action_criteria {
    valid_action_types _vats;
    chips _min;
    chips _max;

public:
    action_criteria() noexcept = default;

    action_criteria(valid_action_types vats, chips min, chips max)
        : _vats(vats)
        , _min(min)
        , _max(max)
    {
    }

    valid_action_types
    action_types() const noexcept
    {
        return _vats;
    }

    chips
    min() const noexcept
    {
        return _min;
    }

    chips
    max() const noexcept
    {
        return _max;
    }

    bool
    is_satisfied_by(action a) const
    {
        auto is_action_type_valid = bool();
        if (_vats == valid_action_types::check_bet) {
            constexpr auto check = action_type::check;
            constexpr auto bet = action_type::bet;
            is_action_type_valid = a.type() == check || a.type() == bet;
        } else if (_vats == valid_action_types::call_raise) {
            constexpr auto call = action_type::call;
            constexpr auto raise = action_type::raise;
            is_action_type_valid = a.type() == call || a.type() == raise;
        } else if (_vats == valid_action_types::call_only) {
            is_action_type_valid = a.type() == action_type::call;
        }
        if (!is_action_type_valid) {
            return false;
        }
        if (is_aggressive(a)) {
            const auto bet = a.bet_size();
            const auto is_bet_size_valid = _min <= bet && bet <= _max;
            return is_bet_size_valid;
        }
        return true;
    }
};

enum class round_of_betting { preflop, flop, turn, river };

struct blinds {
    chips small;
    chips big;
    chips ante;
};

class dealer {
public:
    using player_container = std::array<player*, 4>;  // this can be a static_vector of reasonable capacity
    using player_iterator = player_container::iterator;

    player_container _players;
    player* _button;
    player_iterator _current_player;
    player_iterator _last_aggressive_actor;
    bool _betting_round_first_action;

    community_cards* _community_cards;
    deck* _deck;

    blinds _blinds;
    chips _last_raise;
    chips _biggest_bet;
    action_criteria _action_criteria;
    round_of_betting _rob;

    void
    cyclic_increment()
    {
        ++_current_player;
        if (_current_player == end(_players)) {
            _current_player = begin(_players);
        }
    }

    void
    nonull_increment()
    {
        do {
            cyclic_increment();
        } while (*_current_player == nullptr);
    }

    void
    action_increment(bool is_aggressive)
    {
        if (is_aggressive) {
            _last_aggressive_actor = _current_player;
        }
        nonull_increment();
        evaluate_action_criteria();
    }

    void
    evaluate_action_criteria()
    {
        const auto& player = **_current_player;
        if (_biggest_bet == 0) {
            const auto vats = valid_action_types::check_bet;
            const auto min = std::min(_blinds.big, player.stack());
            const auto max = player.stack();
            _action_criteria = action_criteria(vats, min, max);
        } else if (player.stack() <= _biggest_bet) {
            const auto vats = valid_action_types::call_only;
            const auto min = player.stack();
            const auto max = player.stack();
            _action_criteria = action_criteria(vats, min, max);
        } else {
            auto vats = valid_action_types();
            if (player.bet_size() == _biggest_bet) {
                vats = valid_action_types::check_raise;
            } else {
                vats = valid_action_types::call_raise;
            }
            const auto min = std::min(_biggest_bet + _last_raise, player.stack());
            const auto max = player.stack();
            _action_criteria = action_criteria(vats, min, max);
        }
    }

    void
    action_taken(action a)
    {
        assert(!betting_round_done());
        // TODO: action_criteria, validity check
        switch (a.type()) {
        case action_type::fold:
            // pot.add(_current_player->take_bet());
            *_current_player = nullptr;
            action_increment(false);
            break;
        case action_type::check:
            action_increment(false);
            break;
        case action_type::call:
            (*_current_player)->bet(_biggest_bet);
            action_increment(false);
            break;
        case action_type::bet:
            (*_current_player)->bet(a.bet_size());
            _last_raise = a.bet_size() - _biggest_bet;
            _biggest_bet = a.bet_size();
            action_increment(true);
            break;
        case action_type::raise:
            (*_current_player)->bet(a.bet_size());
            action_increment(true);
            break;
        default:
            assert(false);
            break;
        }
        _betting_round_first_action = false;
    }

    // TODO: We need functions like end_betting_round() and end_hand()
    // We also need a boolean like '_first_round_action' or something because
    // _last_aggressive_actor and _current_player are always equal at the start
    // of each betting round.

    void
    end_betting_round()
    {
        assert(betting_round_done());

        // gather the bets and form the pot(s)
        //
        // - all the bets are simply collected with .take_from_bet(amount)
        // - all players that are not null are eligible to win the pot
        // - there might be multiple pots and range of eligible players will most likely not be contiguous
        //
        // const auto a1 = min_element(players, &player::stack);
        // for (auto p : players) p.take_from_stack(a1);
        // const auto a2 = min_element(players, &player::stack);
        // if (a2 > 0) /* repeat the process */
        //
        // At the end of each betting round, the list of eligible players is
        // renewed (because we don't take players that folded into account
        // anymore).
        // We always look at the last pot, because the pots before are always
        // finalized.
        // If N is number of players, N-1 is max number of pots.

        // One player past the button is first to act.
        /* const auto first = std::cbegin(_players); */
        /* const auto last = std::cend(_players); */
        // FIXME: We would actually need to find the button in _players and
        // then find the next player after the button who is also not
        // nulled-out of _players.
        /* _current_player = std::find(first, last, _button); */
        /* cyclic_increment(); */
    }

    bool
    betting_round_done() const
    {
        if (_betting_round_first_action) {
            return false;
        } else {
            return _current_player == _last_aggressive_actor;
        }
    }

    bool
    done() const
    {
        constexpr auto river = round_of_betting::river;
        const auto betting_rounds_done = _rob == river && betting_round_done();
        const auto first = cbegin(_players);
        const auto last = cend(_players);
        constexpr auto not_null = [] (player* p) { return p != nullptr; };
        const auto num_active_players = std::count_if(first, last, not_null);
        return num_active_players == 1 || betting_rounds_done;
    }

    template<class R>
    dealer(R* players, player* button, community_cards* cc, deck* d, blinds bs)
        : _button(button)
        , _community_cards(cc)
        , _deck(d)
        , _blinds(bs)
        , _rob(round_of_betting::preflop)
        , _betting_round_first_action(true)
    {
        constexpr auto addressof = [] (auto& obj) {
            return std::addressof(obj);
        };
        auto first = std::begin(*players);
        auto last = std::end(*players);
        std::transform(first, last, begin(_players), addressof);
        _current_player = std::find(begin(_players), end(_players), button);
        _last_aggressive_actor = _current_player; // FIXME: FOR NOW
        _last_raise = 0;
        _biggest_bet = 0;
        evaluate_action_criteria();
    }

    span<player*>
    players() noexcept
    {
        // NOTE: The user can get all the data directly from his container
        // anyways. And he needs to know somehow which players are inactive.
        return span<player*>(_players);
    }

    player&
    player_to_act() noexcept
    {
        assert(!betting_round_done());
        return **_current_player;
    }

    void
    post_blinds()
    {
        cyclic_increment();
        (*_current_player)->bet(std::min(_blinds.small, player_to_act().stack()));
        cyclic_increment();
        (*_current_player)->bet(std::min(_blinds.big, player_to_act().stack()));
        cyclic_increment();
        _last_aggressive_actor = _current_player;
        _last_raise = _blinds.small;
        _biggest_bet = _blinds.big;
        evaluate_action_criteria();
    }
};

// JUST AN IDEA HERE
// The actions you need to explicitly tell the dealer to commit.
enum class dealer_action {
    post_ante,
    post_blinds,
    end_betting_round,
    end_hand
};

} // namespace poker

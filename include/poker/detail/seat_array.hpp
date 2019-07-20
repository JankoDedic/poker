#pragma once

#include <array>

#include <poker/player.hpp>

#include "poker/detail/seat_index.hpp"

namespace poker {

class seat_array {
public:
    static constexpr auto num_seats = std::size_t{9};

    constexpr auto occupancy() const noexcept -> const std::array<bool, num_seats>& {
        return _occupancy;
    }

    constexpr auto operator[](seat_index seat) noexcept -> player& {
        assert(occupancy()[seat]);
        return _players[seat];
    }

    constexpr auto operator[](seat_index seat) const noexcept -> const player& {
        assert(occupancy()[seat]);
        return _players[seat];
    }

    constexpr void add_player(seat_index seat, player p) noexcept {
        assert(!occupancy()[seat]);
        _players[seat] = p;
        _occupancy[seat] = true;
    }

    constexpr void remove_player(seat_index seat) noexcept {
        assert(occupancy()[seat]);
        _occupancy[seat] = false;
    }

    class iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = player;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;

        constexpr iterator(seat_array& players, std::size_t index) noexcept
            : _players{&players}
            , _index{index}
        {
        }

        constexpr auto operator*() noexcept -> player& {
            return (*_players)[_index];
        }

        constexpr auto operator*() const noexcept -> player& {
            return (*_players)[_index];
        }

        constexpr void operator++() noexcept {
            do {
                ++_index;
            } while (_index < num_seats && !_players->occupancy()[_index]);
        }

        constexpr auto operator==(const iterator& other) const noexcept -> bool {
            return _index == other._index;
        }

        constexpr auto operator!=(const iterator& other) const noexcept -> bool {
            return _index != other._index;
        }

        constexpr auto index() const noexcept -> std::size_t {
            return _index;
        }

    private:
        seat_array* _players = nullptr;
        std::size_t _index = 0;
    };

    constexpr auto begin() noexcept -> iterator {
        auto i = std::size_t{0};
        for (; i < num_seats; ++i) {
            if (_occupancy[i]) {
                break;
            }
        }
        return {*this, i};
    }

    constexpr auto end() noexcept -> iterator {
        return {*this, num_seats};
    }

private:
    std::array<player, num_seats> _players = {};
    std::array<bool, num_seats> _occupancy = {};
};

class seat_array_view {
public:
    static constexpr auto num_seats = seat_array::num_seats;

    seat_array_view() = default;

    seat_array_view(seat_array& players)
        : _players{&players}
        , _filter{players.occupancy()}
    {
    }

    seat_array_view(seat_array& players, const std::array<bool, num_seats>& filter)
        : _players{&players}
        , _filter{filter}
    {
        // CONTRACT CHECK
        for (auto i = 0; i < num_seats; ++i) {
            if (filter[i]) assert(players.occupancy()[i]);
        }
    }

    constexpr auto underlying() const noexcept -> const seat_array& {
        return *_players;
    }

    constexpr auto underlying() noexcept -> seat_array& {
        return *_players;
    }

    constexpr auto filter() const noexcept -> const std::array<bool, num_seats>& {
        return _filter;
    }

    constexpr auto operator[](seat_index seat) noexcept -> player& {
        assert(filter()[seat]);
        return (*_players)[seat];
    }

    constexpr auto operator[](seat_index seat) const noexcept -> const player& {
        assert(filter()[seat]);
        return (*_players)[seat];
    }

    constexpr void exclude_player(seat_index seat) noexcept {
        assert(filter()[seat]);
        _filter[seat] = false;
    }

    class iterator {
    public:
        using difference_type = std::ptrdiff_t;
        using value_type = player;
        using pointer = value_type*;
        using reference = value_type&;
        using iterator_category = std::bidirectional_iterator_tag;

        constexpr iterator(seat_array_view& players, std::size_t index) noexcept
            : _players{&players}
            , _index{index}
        {
        }

        constexpr auto operator*() noexcept -> player& {
            return (*_players)[_index];
        }

        constexpr auto operator*() const noexcept -> player& {
            return (*_players)[_index];
        }

        constexpr void operator++() noexcept {
            do {
                ++_index;
            } while (_index < num_seats && !_players->filter()[_index]);
        }

        constexpr auto operator==(const iterator& other) const noexcept -> bool {
            return _index == other._index;
        }

        constexpr auto operator!=(const iterator& other) const noexcept -> bool {
            return _index != other._index;
        }

        constexpr auto index() const noexcept -> std::size_t {
            return _index;
        }

    private:
        seat_array_view* _players = nullptr;
        std::size_t _index = 0;
    };

    constexpr auto begin() noexcept -> iterator {
        auto i = std::size_t{0};
        for (; i < num_seats; ++i) {
            if (_filter[i]) {
                break;
            }
        }
        return {*this, i};
    }

    constexpr auto end() noexcept -> iterator {
        return {*this, num_seats};
    }

private:
    seat_array* _players = nullptr;
    std::array<bool, num_seats> _filter = {};
};

} // namespace poker

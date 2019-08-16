#pragma once

#include <algorithm>
#include <array>
#include <cassert>

#include "poker/detail/span.hpp"

namespace poker {

template<typename T, std::size_t N>
class slot_view;

template<typename T, std::size_t N>
class slot_array {
public:
    friend class slot_view<T, N>;

    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using difference_type = std::ptrdiff_t;
    using size_type       = std::size_t;

    class iterator;
    class const_iterator;

    constexpr auto begin()        noexcept -> iterator;
    constexpr auto begin()  const noexcept -> const_iterator;
    constexpr auto end()          noexcept -> iterator;
    constexpr auto end()    const noexcept -> const_iterator;
    constexpr auto cbegin() const noexcept -> const_iterator;
    constexpr auto cend()   const noexcept -> const_iterator;

    constexpr friend auto operator==(const slot_array&, const slot_array&) noexcept -> bool;
    constexpr friend auto operator!=(const slot_array&, const slot_array&) noexcept -> bool;

    constexpr void swap(slot_array& other) noexcept {
        using std::swap;
        swap(_items, other._items);
        swap(_occupancy, other._occupancy);
    }

    constexpr friend void swap(slot_array& x, slot_array& y) noexcept {
        x.swap(y);
    }

    constexpr auto size() const noexcept -> size_type;

    constexpr auto max_size() const noexcept -> size_type {
        return _items.max_size();
    }

    constexpr auto empty() const noexcept -> bool;

    constexpr slot_array() = default;

    constexpr void add(std::size_t index, T value) noexcept {
        assert(index < N);
        assert(!_occupancy[index]);

        _items[index] = value;
        _occupancy[index] = true;
    }

    constexpr void remove(std::size_t index) noexcept {
        assert(index < N);
        assert(_occupancy[index]);

        _occupancy[index] = false;
    }

    constexpr auto occupancy() const noexcept -> std::array<bool, N> {
        return _occupancy;
    }

    constexpr auto operator[](std::size_t index) const noexcept -> const T& {
        assert(index < N);
        assert(_occupancy[index]);

        return _items[index];
    }

    constexpr auto operator[](std::size_t index) noexcept -> T& {
        assert(index < N);
        assert(_occupancy[index]);

        return _items[index];
    }

private:
    std::array<T, N> _items = {};
    std::array<bool, N> _occupancy = {};
};

template<typename T, std::size_t N>
class slot_array<T, N>::iterator {
public:
    friend class slot_array<T, N>;

    using value_type        = T;
    using reference         = T&;
    using pointer           = T*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    constexpr iterator() = default;

    constexpr auto operator*() const noexcept -> reference {
        return (*_container)[_index];
    }

    constexpr auto operator->() const noexcept -> pointer {
        return &operator*();
    }

    constexpr auto operator++() noexcept -> iterator& {
        if (_index != N) {
            while (++_index != N && !_container->occupancy()[_index]);
        }
        return *this;
    }

    constexpr auto operator++(int) noexcept -> iterator& {
        const auto tmp = *this;
        operator++();
        return tmp;
    }

    constexpr friend auto operator==(const iterator& x, const iterator& y) noexcept -> bool {
        return x._container == y._container && x._index == y._index;
    }

    constexpr friend auto operator!=(const iterator& x, const iterator& y) noexcept -> bool {
        return !(x == y);
    }

private:
    constexpr iterator(slot_array<T, N>* container) noexcept
        : _container{container}
    {
        const auto occupancy = container->occupancy();
        const auto first = std::find(occupancy.cbegin(), occupancy.cend(), true);
        _index = static_cast<std::size_t>(std::distance(occupancy.cbegin(), first));
    }

    constexpr iterator(slot_array<T, N>* container, std::size_t index) noexcept
        : _container{container}
        , _index{index}
    {}

private:
    slot_array<T, N>* _container = nullptr;
    std::size_t _index = 0;
};

template<typename T, std::size_t N>
class slot_array<T, N>::const_iterator {
public:
    friend class slot_array<T, N>;

    using value_type        = const T;
    using reference         = const T&;
    using pointer           = const T*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    constexpr const_iterator() = default;

    constexpr const_iterator(iterator i) noexcept
        : _container{i._container}
        , _index{i._index}
    {}

    constexpr auto operator*() const noexcept -> reference {
        return (*_container)[_index];
    }

    constexpr auto operator->() const noexcept -> pointer {
        return &operator*();
    }

    constexpr auto operator++() noexcept -> const_iterator& {
        if (_index != N) {
            while (++_index != N && !_container->occupancy()[_index]);
        }
        return *this;
    }

    constexpr auto operator++(int) noexcept -> const_iterator& {
        const auto tmp = *this;
        operator++();
        return tmp;
    }

    friend constexpr auto operator==(const const_iterator& x, const const_iterator& y) noexcept -> bool {
        return x._container == y._container && x._index == y._index;
    }

    friend constexpr auto operator!=(const const_iterator& x, const const_iterator& y) noexcept -> bool {
        return !(x == y);
    }

private:
    constexpr const_iterator(const slot_array<T, N>* container) noexcept
        : _container{container}
    {
        const auto occupancy = container->occupancy();
        const auto first = std::find(occupancy.cbegin(), occupancy.cend(), true);
        _index = static_cast<std::size_t>(std::distance(occupancy.cbegin(), first));
    }

    constexpr const_iterator(const slot_array<T, N>* container, std::size_t index) noexcept
        : _container{container}
        , _index{index}
    {}

private:
    const slot_array<T, N>* _container = nullptr;
    std::size_t _index = 0;
};

template<typename T, std::size_t N>
constexpr auto slot_array<T, N>::begin() noexcept -> iterator {
    return {this};
}

template<typename T, std::size_t N>
constexpr auto slot_array<T, N>::begin() const noexcept -> const_iterator {
    return {this};
}

template<typename T, std::size_t N>
constexpr auto slot_array<T, N>::end() noexcept -> iterator {
    return {this, N};
}

template<typename T, std::size_t N>
constexpr auto slot_array<T, N>::end() const noexcept -> const_iterator {
    return {this, N};
}

template<typename T, std::size_t N>
constexpr auto slot_array<T, N>::cbegin() const noexcept -> const_iterator {
    return {this};
}

template<typename T, std::size_t N>
constexpr auto slot_array<T, N>::cend() const noexcept -> const_iterator {
    return {this, N};
}

template<typename T, std::size_t N>
constexpr auto operator==(const slot_array<T, N>& x, const slot_array<T, N>& y) noexcept -> bool {
    const auto& xc = x._container;
    const auto& yc = y._container;
    return std::equal(xc.cbegin(), xc.cend(), yc.cbegin(), yc.cend()) && x._index == y._index;
}

template<typename T, std::size_t N>
constexpr auto operator!=(const slot_array<T, N>& x, const slot_array<T, N>& y) noexcept -> bool {
    return !(x == y);
}

template<typename T, std::size_t N>
constexpr auto slot_array<T, N>::size() const noexcept -> size_type {
    return std::count(_occupancy.begin(), _occupancy.end(), true);
}

template<typename T, std::size_t N>
constexpr auto slot_array<T, N>::empty() const noexcept -> bool {
    return size() == 0;
}

template<typename T, std::size_t N>
class slot_view {
public:
    using value_type      = T;
    using reference       = T&;
    using const_reference = const T&;
    using difference_type = std::ptrdiff_t;
    using size_type       = std::size_t;

    class iterator;
    class const_iterator;

    constexpr auto begin()        noexcept -> iterator;
    constexpr auto begin()  const noexcept -> const_iterator;
    constexpr auto end()          noexcept -> iterator;
    constexpr auto end()    const noexcept -> const_iterator;
    constexpr auto cbegin() const noexcept -> const_iterator;
    constexpr auto cend()   const noexcept -> const_iterator;

    constexpr slot_view(span<T, N> items) noexcept
        : _items{items}
    {
        std::fill(_filter.begin(), _filter.end(), true);
    }

    constexpr slot_view(span<T, N> items, std::array<bool, N> filter) noexcept
        : _items{items}
        , _filter{filter}
    {}

    constexpr slot_view(slot_array<T, N>& items) noexcept
        : _items{items._items}
        , _filter{items.occupancy()}
    {}

    constexpr slot_view(slot_array<T, N>& items, std::array<bool, N> filter) noexcept
        : _items{items}
        , _filter{filter}
    {
        const auto occupancy = items.occupancy();
        assert(std::lexicographical_compare(filter.begin(), filter.end(), occupancy.begin(), occupancy.end(), std::less_equal{}));
    }

    constexpr auto filter() const noexcept -> std::array<bool, N> {
        return _filter;
    }

    constexpr void filter_out(std::size_t index) noexcept {
        assert(index < N);
        assert(_filter[index]);

        _filter[index] = false;
    }

    constexpr auto operator[](std::size_t index) const noexcept -> const T& {
        assert(index < N);
        assert(_filter[index]);

        return _items[index];
    }

    constexpr auto operator[](std::size_t index) noexcept -> T& {
        assert(index < N);
        assert(_filter[index]);

        return _items[index];
    }

private:
    span<T, N> _items = {};
    std::array<bool, N> _filter = {};
};

template<typename T, std::size_t N>
class slot_view<T, N>::iterator {
public:
    friend class slot_view<T, N>;

    using value_type        = T;
    using reference         = T&;
    using pointer           = T*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    constexpr iterator() = default;

    constexpr auto operator*() const noexcept -> reference {
        return (*_view)[_index];
    }

    constexpr auto operator->() const noexcept -> pointer {
        return &operator*();
    }

    constexpr auto operator++() noexcept -> iterator& {
        if (_index != N) {
            while (++_index != N && !_view->filter()[_index]);
        }
        return *this;
    }

    constexpr auto operator++(int) noexcept -> iterator& {
        const auto tmp = *this;
        operator++();
        return tmp;
    }

    constexpr friend auto operator==(const iterator& x, const iterator& y) noexcept -> bool {
        return x._view == y._view && x._index == y._index;
    }

    constexpr friend auto operator!=(const iterator& x, const iterator& y) noexcept -> bool {
        return !(x == y);
    }

private:
    constexpr iterator(slot_view<T, N>* view) noexcept
        : _view{view}
    {
        const auto filter = view->filter();
        const auto first = std::find(filter.cbegin(), filter.cend(), true);
        _index = static_cast<std::size_t>(std::distance(filter.cbegin(), first));
    }

    constexpr iterator(slot_view<T, N>* view, std::size_t index) noexcept
        : _view{view}
        , _index{index}
    {}

private:
    slot_view<T, N>* _view = nullptr;
    std::size_t _index = 0;
};

template<typename T, std::size_t N>
class slot_view<T, N>::const_iterator {
public:
    friend class slot_view<T, N>;

    using value_type        = const T;
    using reference         = const T&;
    using pointer           = const T*;
    using difference_type   = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    constexpr const_iterator() = default;

    constexpr const_iterator(iterator i) noexcept
        : _view{i._view}
        , _index{i._index}
    {}

    constexpr auto operator*() const noexcept -> reference {
        return (*_view)[_index];
    }

    constexpr auto operator->() const noexcept -> pointer {
        return &operator*();
    }

    constexpr auto operator++() noexcept -> const_iterator& {
        if (_index != N) {
            while (++_index != N && !_view->filter()[_index]);
        }
        return *this;
    }

    constexpr auto operator++(int) noexcept -> const_iterator& {
        const auto tmp = *this;
        operator++();
        return tmp;
    }

    friend constexpr auto operator==(const const_iterator& x, const const_iterator& y) noexcept -> bool {
        return x._container == y._container && x._index == y._index;
    }

    friend constexpr auto operator!=(const const_iterator& x, const const_iterator& y) noexcept -> bool {
        return !(x == y);
    }

private:
    constexpr const_iterator(const slot_view<T, N>* view) noexcept
        : _view{view}
    {
        const auto filter = view->filter();
        const auto first = std::find(filter.cbegin(), filter.cend(), true);
        _index = static_cast<std::size_t>(std::distance(filter.cbegin(), first));
    }

    constexpr const_iterator(const slot_view<T, N>* view, std::size_t index) noexcept
        : _view{view}
        , _index{index}
    {
        assert(view->filter()[index]);
    }

private:
    const slot_view<T, N>* _view = nullptr;
    std::size_t _index = 0;
};

template<typename T, std::size_t N>
constexpr auto slot_view<T, N>::begin() noexcept -> iterator {
    return {this};
}

template<typename T, std::size_t N>
constexpr auto slot_view<T, N>::begin() const noexcept -> const_iterator {
    return {this};
}

template<typename T, std::size_t N>
constexpr auto slot_view<T, N>::end() noexcept -> iterator {
    return {this, N};
}

template<typename T, std::size_t N>
constexpr auto slot_view<T, N>::end() const noexcept -> const_iterator {
    return {this, N};
}

template<typename T, std::size_t N>
constexpr auto slot_view<T, N>::cbegin() const noexcept -> const_iterator {
    return {this};
}

template<typename T, std::size_t N>
constexpr auto slot_view<T, N>::cend() const noexcept -> const_iterator {
    return {this, N};
}

} // namespace poker

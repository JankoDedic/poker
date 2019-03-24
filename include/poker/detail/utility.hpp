#pragma once

#include <type_traits>

#define POKER_DETAIL_DEFINE_FRIEND_FLAG_OPERATIONS(Type)                      \
    friend constexpr auto operator~(Type x) noexcept -> Type {                \
        using poker::detail::to_underlying;                                   \
        return static_cast<Type>(~to_underlying(x));                          \
    }                                                                         \
                                                                              \
    friend constexpr auto operator|(Type x, Type y) noexcept -> Type {        \
        using poker::detail::to_underlying;                                   \
        return static_cast<Type>(to_underlying(x) | to_underlying(y));        \
    }                                                                         \
                                                                              \
    friend constexpr auto operator&(Type x, Type y) noexcept -> Type {        \
        using poker::detail::to_underlying;                                   \
        return static_cast<Type>(to_underlying(x) & to_underlying(y));        \
    }                                                                         \
                                                                              \
    friend constexpr auto operator^(Type x, Type y) noexcept -> Type {        \
        using poker::detail::to_underlying;                                   \
        return static_cast<Type>(to_underlying(x) ^ to_underlying(y));        \
    }                                                                         \
                                                                              \
    friend constexpr auto operator|=(Type &x, Type y) noexcept -> Type & {    \
        return x = x | y;                                                     \
    }                                                                         \
                                                                              \
    friend constexpr auto operator&=(Type &x, Type y) noexcept -> Type & {    \
        return x = x & y;                                                     \
    }                                                                         \
                                                                              \
    friend constexpr auto operator^=(Type &x, Type y) noexcept -> Type & {    \
        return x = x & y;                                                     \
    }

namespace poker::detail {

template<typename Enum>
constexpr auto to_underlying(Enum value) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(value);
}

template<typename R>
struct range_value {
    using type = typename R::value_type;
};

template<typename T, std::size_t N>
struct range_value<T[N]> {
    using type = T;
};

template<typename R>
using range_value_t = typename range_value<R>::type;

} // namespace poker::detail

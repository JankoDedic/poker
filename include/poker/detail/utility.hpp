#pragma once

#include <type_traits>

#define POKER_DETAIL_DEFINE_FRIEND_FLAG_OPERATIONS(Type)                      \
    friend constexpr auto operator|(Type x, Type y) noexcept -> Type {        \
        return static_cast<Type>(to_underlying(x) | to_underlying(y));        \
    }                                                                         \
                                                                              \
    friend constexpr auto operator&(Type x, Type y) noexcept -> Type {        \
        return static_cast<Type>(to_underlying(x) & to_underlying(y));        \
    }                                                                         \
                                                                              \
    friend constexpr auto operator|=(Type &x, Type y) noexcept -> Type & {    \
        return x = x | y;                                                     \
    }                                                                         \
                                                                              \
    friend constexpr auto operator&=(Type &x, Type y) noexcept -> Type & {    \
        return x = x & y;                                                     \
    }

namespace poker {

template<typename Enum>
constexpr auto to_underlying(Enum value) noexcept {
    return static_cast<std::underlying_type_t<Enum>>(value);
}

} // namespace poker

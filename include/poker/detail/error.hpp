#pragma once

#include <cassert>
#include <stdexcept>

#ifdef POKER_THROW_ON_ERROR
#   define POKER_DETAIL_THROWS_ON_ERROR 1
#   define POKER_DETAIL_NOEXCEPT
#   define POKER_DETAIL_ASSERT(condition, explanation) if (!(condition)) throw std::logic_error{explanation}
#else
#   define POKER_DETAIL_NOEXCEPT noexcept
#   define POKER_DETAIL_ASSERT(condition, explanation) assert((condition) && explanation)
#endif
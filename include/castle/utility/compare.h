#ifndef CASTLE_UTILITY_COMPARE_H
#define CASTLE_UTILITY_COMPARE_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"

namespace castle
{

template <typename T>
struct less
{
    CASTLE_CONSTEXPR bool operator()(CASTLE_CONST T& lhs, CASTLE_CONST T& rhs) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return lhs < rhs;
    }
};

template <typename T>
struct greater
{
    CASTLE_CONSTEXPR bool operator()(CASTLE_CONST T& lhs, CASTLE_CONST T& rhs) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return lhs > rhs;
    }
};

template <typename T>
struct equal_to
{
    CASTLE_CONSTEXPR bool operator()(CASTLE_CONST T& lhs, CASTLE_CONST T& rhs) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return lhs == rhs;
    }
};

// ============================================================================
// castle::compare
// ----------------------------------------------------------------------------
// Defines the remaining relational operations in terms of a less-than
// comparator. This follows the same useful design as etl::compare while
// remaining independent of STL and heap-backed facilities.
//
// TLess must provide:
//
//     bool operator()(CASTLE_CONST T&, CASTLE_CONST T&) CASTLE_CONST;
//
// Only the less-than operation is required. Equality and the other relational
// operations are derived from it, which is particularly useful for embedded
// value types where implementing one canonical ordering is desirable.
// ============================================================================
template <typename T, typename TLess = less<T>>
struct compare
{
    static_assert(meta::is_invocable_r<bool, TLess, CASTLE_CONST T&, CASTLE_CONST T&>::value,
                  "TLess must be callable with (CASTLE_CONST T&, CASTLE_CONST T&) and return bool");

    enum cmp_result
    {
        Less    = -1,
        Equal   = 0,
        Greater = 1
    };

    using first_argument_type  = typename meta::add_lvalue_reference<CASTLE_CONST T>::type;
    using second_argument_type = typename meta::add_lvalue_reference<CASTLE_CONST T>::type;
    using result_type          = bool;

    static CASTLE_CONSTEXPR bool
    lt(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return TLess()(lhs, rhs);
    }

    static CASTLE_CONSTEXPR bool
    gt(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return TLess()(rhs, lhs);
    }

    static CASTLE_CONSTEXPR bool
    lte(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return !gt(lhs, rhs);
    }

    static CASTLE_CONSTEXPR bool
    gte(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return !lt(lhs, rhs);
    }

    static CASTLE_CONSTEXPR bool
    eq(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return gte(lhs, rhs) && lte(lhs, rhs);
    }

    static CASTLE_CONSTEXPR bool
    ne(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return !eq(lhs, rhs);
    }

    static CASTLE_CONSTEXPR cmp_result
    cmp(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return lt(lhs, rhs)
               ? Less
               : (gt(lhs, rhs)
                 ? Greater
                 : Equal);
    }
};

// ----------------------------------------------------------------------------
// Return -1, 0, or 1 according to the ordering of lhs and rhs.
// ----------------------------------------------------------------------------
template <typename T, typename TLess = less<T>>
CASTLE_CONSTEXPR int
cmp_3_ways (CASTLE_CONST T& lhs, CASTLE_CONST T& rhs) CASTLE_NOEXCEPT
{
    return static_cast<int>(compare<T, TLess>::cmp(lhs, rhs));
}

} // namespace castle

#endif // CASTLE_UTILITY_COMPARE_H

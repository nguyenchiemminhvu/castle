#ifndef CASTLE_UTILITY_PAIR_H
#define CASTLE_UTILITY_PAIR_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/core/traits.h"
#include "castle/utility/forward.h"
#include "castle/utility/move.h"
#include "castle/utility/swap.h"

namespace castle
{
// ============================================================================
// pair
//
// Fixed-size heterogeneous two-value aggregate.
//
// first and second are intentionally public, matching std::pair/etl::pair and
// making pair useful as a small embedded key/value record.
// ============================================================================
template <typename T1, typename T2>
struct pair
{
    using first_type = T1;
    using second_type = T2;

    T1 first;
    T2 second;

    pair() CASTLE_DEFAULT;
    pair(CASTLE_CONST T1& first_value,
         CASTLE_CONST T2& second_value)
        : first(first_value)
        , second(second_value)
    {
    }

    pair(T1&& first_value,
         T2&& second_value)
        : first(CASTLE_MOVE(first_value))
        , second(CASTLE_MOVE(second_value))
    {
    }

    template <typename U1, typename U2>
    pair(U1&& first_value, U2&& second_value)
        : first(CASTLE_FORWARD<U1>(first_value))
        , second(CASTLE_FORWARD<U2>(second_value))
    {
    }

    pair(CASTLE_CONST pair&) CASTLE_DEFAULT;
    pair(pair&&) CASTLE_DEFAULT;

    pair& operator=(CASTLE_CONST pair&) CASTLE_DEFAULT;
    pair& operator=(pair&&) CASTLE_DEFAULT;

    void swap(pair& other) CASTLE_NOEXCEPT(
        CASTLE_NOEXCEPT(castle::swap(first, other.first)) &&
        CASTLE_NOEXCEPT(castle::swap(second, other.second)))
    {
        castle::swap(first, other.first);
        castle::swap(second, other.second);
    }
};

// ============================================================================
// make_pair
// ============================================================================
template <typename T1, typename T2>
pair<meta::decay_t<T1>, meta::decay_t<T2>>
make_pair(T1&& first_value, T2&& second_value) CASTLE_NOEXCEPT(
    meta::is_nothrow_constructible<meta::decay_t<T1>, T1&&>::value && meta::is_nothrow_constructible<meta::decay_t<T2>, T2&&>::value)
{
    return pair<meta::decay_t<T1>, meta::decay_t<T2>>(
        CASTLE_FORWARD<T1>(first_value), CASTLE_FORWARD<T2>(second_value));
}

// ============================================================================
// swap(pair, pair)
// ============================================================================
template <typename T1, typename T2>
void swap(pair<T1, T2>& lhs, pair<T1, T2>& rhs)
    CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

// ============================================================================
// get<0>() / get<1>()
// ============================================================================
namespace detail
{

template <size_type Index, typename T1, typename T2>
struct pair_getter;

template <typename T1, typename T2>
struct pair_getter<0U, T1, T2>
{
    static T1& get(pair<T1, T2>& value) CASTLE_NOEXCEPT
    {
        return value.first;
    }

    static CASTLE_CONST T1& get(CASTLE_CONST pair<T1, T2>& value) CASTLE_NOEXCEPT
    {
        return value.first;
    }
};

template <typename T1, typename T2>
struct pair_getter<1U, T1, T2>
{
    static T2& get(pair<T1, T2>& value) CASTLE_NOEXCEPT
    {
        return value.second;
    }

    static CASTLE_CONST T2& get(CASTLE_CONST pair<T1, T2>& value) CASTLE_NOEXCEPT
    {
        return value.second;
    }
};

} // namespace detail

template <size_type Index, typename T1, typename T2>
typename meta::conditional<Index == 0U, T1&, T2&>::type
get(pair<T1, T2>& value)
{
    static_assert(Index < 2U, "castle::get<pair>: index out of range");
    return detail::pair_getter<Index, T1, T2>::get(value);
}

template <size_type Index, typename T1, typename T2>
typename meta::conditional<Index == 0U, CASTLE_CONST T1&, CASTLE_CONST T2&>::type
get(CASTLE_CONST pair<T1, T2>& value)
{
    static_assert(Index < 2U, "castle::get<pair>: index out of range");
    return detail::pair_getter<Index, T1, T2>::get(value);
}

template <size_type Index, typename T1, typename T2>
typename meta::conditional<Index == 0U, T1&&, T2&&>::type
get(pair<T1, T2>&& value)
{
    return CASTLE_MOVE(castle::get<Index>(value));
}

template <size_type Index, typename T1, typename T2>
typename meta::conditional<Index == 0U, CASTLE_CONST T1&&, CASTLE_CONST T2&&>::type
get(CASTLE_CONST pair<T1, T2>&& value)
{
    return CASTLE_MOVE(castle::get<Index>(value));
}

// ============================================================================
// pair comparisons
// ============================================================================
template <typename T1, typename T2, typename U1, typename U2>
bool operator==(CASTLE_CONST pair<T1, T2>& lhs,
                CASTLE_CONST pair<U1, U2>& rhs)
{
    return lhs.first == rhs.first && lhs.second == rhs.second;
}

template <typename T1, typename T2, typename U1, typename U2>
bool operator!=(CASTLE_CONST pair<T1, T2>& lhs,
                CASTLE_CONST pair<U1, U2>& rhs)
{
    return !(lhs == rhs);
}

template <typename T1, typename T2, typename U1, typename U2>
bool operator<(CASTLE_CONST pair<T1, T2>& lhs,
               CASTLE_CONST pair<U1, U2>& rhs)
{
    if (lhs.first < rhs.first)
    {
        return true;
    }
    if (rhs.first < lhs.first)
    {
        return false;
    }
    return lhs.second < rhs.second;
}

template <typename T1, typename T2, typename U1, typename U2>
bool operator>(CASTLE_CONST pair<T1, T2>& lhs,
               CASTLE_CONST pair<U1, U2>& rhs)
{
    return rhs < lhs;
}

template <typename T1, typename T2, typename U1, typename U2>
bool operator<=(CASTLE_CONST pair<T1, T2>& lhs,
                CASTLE_CONST pair<U1, U2>& rhs)
{
    return !(rhs < lhs);
}

template <typename T1, typename T2, typename U1, typename U2>
bool operator>=(CASTLE_CONST pair<T1, T2>& lhs,
                CASTLE_CONST pair<U1, U2>& rhs)
{
    return !(lhs < rhs);
}

} // namespace castle

#endif // CASTLE_UTILITY_PAIR_H

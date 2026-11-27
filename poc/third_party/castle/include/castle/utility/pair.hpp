// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file pair.hpp
 * @brief Fixed-size heterogeneous two-value aggregate.
 *
 * Use this header when two related values should travel together without heap
 * allocation or a standard-library dependency. `castle::pair` exposes its
 * members publicly, supports structured access through `get`, and provides the
 * usual comparison and swap helpers.
 *
 * @code
 * auto item = castle::make_pair(1U, 2U);
 * auto first = castle::get<0>(item);
 * @endcode
 */
#ifndef CASTLE_UTILITY_PAIR_HPP
#define CASTLE_UTILITY_PAIR_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"
#include "castle/utility/forward.hpp"
#include "castle/utility/move.hpp"
#include "castle/utility/swap.hpp"

namespace castle
{
/**
 * @brief Stores two heterogeneous values in a single aggregate.
 * @tparam T1 Type of `first`.
 * @tparam T2 Type of `second`.
 */
template <typename T1, typename T2>
struct pair
{
    using first_type = T1;
    using second_type = T2;

    T1 first;
    T2 second;

    /** @brief Default-constructs both members. */
    pair() CASTLE_DEFAULT;
    /**
     * @brief Constructs both members from lvalue inputs.
     * @param first_value Value used to initialize `first`.
     * @param second_value Value used to initialize `second`.
     */
    pair(CASTLE_CONST T1& first_value,
         CASTLE_CONST T2& second_value)
        : first(first_value)
        , second(second_value)
    {
    }

    /**
     * @brief Constructs both members from rvalue inputs.
     * @param first_value Value moved into `first`.
     * @param second_value Value moved into `second`.
     */
    pair(T1&& first_value,
         T2&& second_value)
        : first(CASTLE_MOVE(first_value))
        , second(CASTLE_MOVE(second_value))
    {
    }

    /**
     * @brief Constructs both members from arbitrary compatible arguments.
     * @tparam U1 Source type for `first`.
     * @tparam U2 Source type for `second`.
     * @param first_value Value forwarded into `first`.
     * @param second_value Value forwarded into `second`.
     */
    template <typename U1, typename U2>
    pair(U1&& first_value, U2&& second_value)
        : first(CASTLE_FORWARD<U1>(first_value))
        , second(CASTLE_FORWARD<U2>(second_value))
    {
    }

    /** @brief Copy-constructs the pair. */
    pair(CASTLE_CONST pair&) CASTLE_DEFAULT;
    /** @brief Move-constructs the pair. */
    pair(pair&&) CASTLE_DEFAULT;

    /** @brief Copy-assigns the pair. */
    pair& operator=(CASTLE_CONST pair&) CASTLE_DEFAULT;
    /** @brief Move-assigns the pair. */
    pair& operator=(pair&&) CASTLE_DEFAULT;

    /**
     * @brief Exchanges both members with another pair.
     * @param other Pair to swap with.
     */
    void swap(pair& other) CASTLE_NOEXCEPT(
        CASTLE_NOEXCEPT(castle::swap(first, other.first)) &&
        CASTLE_NOEXCEPT(castle::swap(second, other.second)))
    {
        castle::swap(first, other.first);
        castle::swap(second, other.second);
    }
};

/**
 * @brief Creates a `castle::pair` with decayed element types.
 * @tparam T1 Source type for the first element.
 * @tparam T2 Source type for the second element.
 * @param first_value Value forwarded into the first element.
 * @param second_value Value forwarded into the second element.
 * @return A `pair<meta::decay_t<T1>, meta::decay_t<T2>>`.
 */
template <typename T1, typename T2>
pair<meta::decay_t<T1>, meta::decay_t<T2>>
make_pair(T1&& first_value, T2&& second_value) CASTLE_NOEXCEPT(
    meta::is_nothrow_constructible<meta::decay_t<T1>, T1&&>::value && meta::is_nothrow_constructible<meta::decay_t<T2>, T2&&>::value)
{
    return pair<meta::decay_t<T1>, meta::decay_t<T2>>(
        CASTLE_FORWARD<T1>(first_value), CASTLE_FORWARD<T2>(second_value));
}

/**
 * @brief Swaps two pairs.
 * @tparam T1 First element type.
 * @tparam T2 Second element type.
 * @param lhs First pair.
 * @param rhs Second pair.
 */
template <typename T1, typename T2>
void swap(pair<T1, T2>& lhs, pair<T1, T2>& rhs)
    CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(lhs.swap(rhs)))
{
    lhs.swap(rhs);
}

namespace detail
{

/**
 * Helper struct to access pair elements by compile-time index.
 * @tparam Index Element index, either `0` or `1`.
 * @tparam T1 First element type.
 * @tparam T2 Second element type.
 * Provides static member functions to access the pair elements by index.
 */
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

/**
 * @brief Returns a pair element by compile-time index.
 * @tparam Index Element index, either `0` or `1`.
 * @tparam T1 First element type.
 * @tparam T2 Second element type.
 * @param value Pair to inspect.
 * @return Reference to the requested element.
 */
template <size_type Index, typename T1, typename T2>
typename meta::conditional<Index == 0U, T1&, T2&>::type
get(pair<T1, T2>& value)
{
    static_assert(Index < 2U, "castle::get<pair>: index out of range");
    return detail::pair_getter<Index, T1, T2>::get(value);
}

/**
 * @brief Returns a const pair element by compile-time index.
 * @tparam Index Element index, either `0` or `1`.
 * @tparam T1 First element type.
 * @tparam T2 Second element type.
 * @param value Pair to inspect.
 * @return Const reference to the requested element.
 */
template <size_type Index, typename T1, typename T2>
typename meta::conditional<Index == 0U, CASTLE_CONST T1&, CASTLE_CONST T2&>::type
get(CASTLE_CONST pair<T1, T2>& value)
{
    static_assert(Index < 2U, "castle::get<pair>: index out of range");
    return detail::pair_getter<Index, T1, T2>::get(value);
}

/**
 * @brief Returns a moved pair element by compile-time index.
 * @tparam Index Element index, either `0` or `1`.
 * @tparam T1 First element type.
 * @tparam T2 Second element type.
 * @param value Pair to inspect.
 * @return Rvalue reference to the requested element.
 */
template <size_type Index, typename T1, typename T2>
typename meta::conditional<Index == 0U, T1&&, T2&&>::type
get(pair<T1, T2>&& value)
{
    return CASTLE_MOVE(castle::get<Index>(value));
}

/**
 * @brief Returns a moved const pair element by compile-time index.
 * @tparam Index Element index, either `0` or `1`.
 * @tparam T1 First element type.
 * @tparam T2 Second element type.
 * @param value Pair to inspect.
 * @return Const rvalue reference to the requested element.
 */
template <size_type Index, typename T1, typename T2>
typename meta::conditional<Index == 0U, CASTLE_CONST T1&&, CASTLE_CONST T2&&>::type
get(CASTLE_CONST pair<T1, T2>&& value)
{
    return CASTLE_MOVE(castle::get<Index>(value));
}

/** @brief Returns whether two pairs compare equal element by element. */
template <typename T1, typename T2, typename U1, typename U2>
bool operator==(CASTLE_CONST pair<T1, T2>& lhs,
                CASTLE_CONST pair<U1, U2>& rhs)
{
    return lhs.first == rhs.first && lhs.second == rhs.second; // LCOV_EXCL_BR_LINE
}

/** @brief Returns whether two pairs compare unequal element by element. */
template <typename T1, typename T2, typename U1, typename U2>
bool operator!=(CASTLE_CONST pair<T1, T2>& lhs,
                CASTLE_CONST pair<U1, U2>& rhs)
{
    return !(lhs == rhs);
}

/**
 * @brief Lexicographically compares two pairs.
 * @return `true` when `lhs` sorts before `rhs`.
 */
template <typename T1, typename T2, typename U1, typename U2>
bool operator<(CASTLE_CONST pair<T1, T2>& lhs,
               CASTLE_CONST pair<U1, U2>& rhs)
{
    if (lhs.first < rhs.first)
    {
        return true;
    }
    // LCOV_EXCL_START
    if (rhs.first < lhs.first)
    {
        return false;
    }
    // LCOV_EXCL_STOP
    return lhs.second < rhs.second;
}

/** @brief Returns whether `lhs` sorts after `rhs`. */
template <typename T1, typename T2, typename U1, typename U2>
bool operator>(CASTLE_CONST pair<T1, T2>& lhs,
               CASTLE_CONST pair<U1, U2>& rhs)
{
    return rhs < lhs;
}

/** @brief Returns whether `lhs` sorts before or equals `rhs`. */
template <typename T1, typename T2, typename U1, typename U2>
bool operator<=(CASTLE_CONST pair<T1, T2>& lhs,
                CASTLE_CONST pair<U1, U2>& rhs)
{
    return !(rhs < lhs);
}

/** @brief Returns whether `lhs` sorts after or equals `rhs`. */
template <typename T1, typename T2, typename U1, typename U2>
bool operator>=(CASTLE_CONST pair<T1, T2>& lhs,
                CASTLE_CONST pair<U1, U2>& rhs)
{
    return !(lhs < rhs);
}

} // namespace castle

#endif // CASTLE_UTILITY_PAIR_HPP

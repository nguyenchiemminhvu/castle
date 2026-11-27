// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file compare.hpp
 * @brief Comparator helpers and derived relational operations.
 *
 * Use this header when embedded value types expose a single canonical ordering
 * and the remaining comparison operations should be derived from it. The
 * utilities are constexpr, allocation-free, exception-free, and independent of
 * the standard library.
 *
 * @code
 * using order = castle::compare<uint32_t>;
 * bool ordered = order::lt(1U, 2U);
 * @endcode
 */
#ifndef CASTLE_UTILITY_COMPARE_HPP
#define CASTLE_UTILITY_COMPARE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{

/**
 * @brief Default less-than comparator for `T`.
 * @tparam T Value type to compare.
 */
template <typename T>
struct less
{
    /**
     * @brief Returns whether `lhs` is less than `rhs`.
     * @param lhs Left operand.
     * @param rhs Right operand.
     * @return `true` when `lhs < rhs`.
     */
    CASTLE_CONSTEXPR bool operator()(CASTLE_CONST T& lhs, CASTLE_CONST T& rhs) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return lhs < rhs;
    }
};

/**
 * @brief Default greater-than comparator for `T`.
 * @tparam T Value type to compare.
 */
template <typename T>
struct greater
{
    /**
     * @brief Returns whether `lhs` is greater than `rhs`.
     * @param lhs Left operand.
     * @param rhs Right operand.
     * @return `true` when `lhs > rhs`.
     */
    CASTLE_CONSTEXPR bool operator()(CASTLE_CONST T& lhs, CASTLE_CONST T& rhs) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return lhs > rhs;
    }
};

/**
 * @brief Default equality comparator for `T`.
 * @tparam T Value type to compare.
 */
template <typename T>
struct equal_to
{
    /**
     * @brief Returns whether `lhs` equals `rhs`.
     * @param lhs Left operand.
     * @param rhs Right operand.
     * @return `true` when `lhs == rhs`.
     */
    CASTLE_CONSTEXPR bool operator()(CASTLE_CONST T& lhs, CASTLE_CONST T& rhs) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return lhs == rhs;
    }
};

/**
 * @brief Derives all relational operations from a less-than comparator.
 * @tparam T Value type to compare.
 * @tparam TLess Comparator type used for ordering.
 * @note `TLess` must be callable as `bool(const T&, const T&)`.
 */
template <typename T, typename TLess = less<T>>
struct compare
{
    static_assert(meta::is_invocable_r<bool, TLess, CASTLE_CONST T&, CASTLE_CONST T&>::value,
                  "TLess must be callable with (CASTLE_CONST T&, CASTLE_CONST T&) and return bool");

    /**
     * @brief Three-way comparison result values used by `cmp()`.
     */
    enum cmp_result
    {
        Less    = -1,
        Equal   = 0,
        Greater = 1
    };

    using first_argument_type  = typename meta::add_lvalue_reference<CASTLE_CONST T>::type;
    using second_argument_type = typename meta::add_lvalue_reference<CASTLE_CONST T>::type;
    using result_type          = bool;

    /** @brief Returns whether `lhs` is less than `rhs`. */
    static CASTLE_CONSTEXPR bool
    lt(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return TLess()(lhs, rhs);
    }

    /** @brief Returns whether `lhs` is greater than `rhs`. */
    static CASTLE_CONSTEXPR bool
    gt(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return TLess()(rhs, lhs);
    }

    /** @brief Returns whether `lhs` is less than or equal to `rhs`. */
    static CASTLE_CONSTEXPR bool
    lte(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return !gt(lhs, rhs);
    }

    /** @brief Returns whether `lhs` is greater than or equal to `rhs`. */
    static CASTLE_CONSTEXPR bool
    gte(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return !lt(lhs, rhs);
    }

    /** @brief Returns whether `lhs` and `rhs` are equivalent under `TLess`. */
    static CASTLE_CONSTEXPR bool
    eq(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return gte(lhs, rhs) && lte(lhs, rhs); // LCOV_EXCL_BR_LINE
    }

    /** @brief Returns whether `lhs` and `rhs` are not equivalent under `TLess`. */
    static CASTLE_CONSTEXPR bool
    ne(first_argument_type lhs, second_argument_type rhs) CASTLE_NOEXCEPT
    {
        return !eq(lhs, rhs);
    }

    /**
     * @brief Returns a three-way ordering result for `lhs` and `rhs`.
     * @param lhs Left operand.
     * @param rhs Right operand.
     * @return `Less`, `Equal`, or `Greater`.
     */
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

/**
 * @brief Returns `-1`, `0`, or `1` according to the ordering of two values.
 * @tparam T Value type to compare.
 * @tparam TLess Comparator type used for ordering.
 * @param lhs Left operand.
 * @param rhs Right operand.
 * @return Negative, zero, or positive ordering result.
 */
template <typename T, typename TLess = less<T>>
CASTLE_CONSTEXPR int
cmp_3_ways (CASTLE_CONST T& lhs, CASTLE_CONST T& rhs) CASTLE_NOEXCEPT
{
    return static_cast<int>(compare<T, TLess>::cmp(lhs, rhs));
}

} // namespace castle

#endif // CASTLE_UTILITY_COMPARE_HPP

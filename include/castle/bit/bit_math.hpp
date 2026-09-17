// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bit_math.hpp
 * @brief Integer math helpers expressed in terms of bit patterns.
 *
 * Use this header for parity tests, power-of-two rounding, alignment math, and
 * sign queries that need to stay constexpr-friendly and deterministic on
 * embedded targets.
 *
 * Key constraints:
 * - Accepts Castle integer types that satisfy `castle::meta::is_valid_integer`.
 * - Alignment helpers assume a meaningful power-of-two alignment supplied by the caller.
 * - No allocation, exceptions, RTTI, virtual dispatch, or STL facilities are used.
 */
#ifndef CASTLE_BIT_BIT_MATH_HPP
#define CASTLE_BIT_BIT_MATH_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/core/traits.hpp"

#include <stdint.h>
#include <limits.h>

namespace castle
{
namespace bit
{

/**
 * @brief Tests whether an integer value is even.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param v Value to inspect.
 * @return `true` when the least-significant bit is clear; otherwise `false`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
is_even(T v) CASTLE_NOEXCEPT
{
    return (v & 1) == 0;
}

/**
 * @brief Compile-time evenness test.
 * @tparam N Compile-time value to inspect.
 */
template <size_type N>
struct is_even_const
{
    static CASTLE_CONSTEXPR bool value = ((N & 1) == 0);
};

/**
 * @brief Tests whether an integer value is odd.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param v Value to inspect.
 * @return `true` when the least-significant bit is set; otherwise `false`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
is_odd(T v) CASTLE_NOEXCEPT
{
    return (v & 1) != 0;
}

/**
 * @brief Compile-time oddness test.
 * @tparam N Compile-time value to inspect.
 */
template <size_type N>
struct is_odd_const
{
    static CASTLE_CONSTEXPR bool value = ((N & 1) != 0);
};

/**
 * @brief Tests whether a value is an exact power of two.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param v Value to inspect.
 * @return `true` when `v` is positive and has exactly one set bit; otherwise `false`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
is_power_of_two(T v) CASTLE_NOEXCEPT
{
    if (v < 0)
        return false;

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(v);
    return uval != 0 && (uval & (uval - 1)) == 0;
}

/**
 * @brief Compile-time power-of-two test.
 * @tparam N Compile-time value to inspect.
 */
template <size_type N>
struct is_power_of_two_const
{
    static CASTLE_CONSTEXPR bool value = (N != 0 && (N & (N - 1)) == 0);
};

/**
 * @brief Rounds a value up to the next power of two.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param v Value to round.
 * @return The smallest power of two greater than or equal to `v`. Returns 1 when `v` is 0.
 * @warning If the mathematical result does not fit in `T`, the returned value follows the implementation's unsigned wraparound behavior.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
next_power_of_two(T v) CASTLE_NOEXCEPT
{
    if (v == 0)
    {
        return 1;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(v);

    --uval; // Turn an exact power of two into the preceding all-ones pattern.
    for (size_type i = 1; i < sizeof(T) * CHAR_BIT; i *= 2)
    {
        uval |= uval >> i; // Smear the highest set bit downward.
    }

    return static_cast<T>(uval + 1);
}

/**
 * @brief Compile-time power-of-two rounding for `castle::size_type` values.
 * @tparam N Compile-time value to round.
 * @return The smallest power of two greater than or equal to `N`. Returns 1 when `N` is 0.
 */
template <size_type N>
CASTLE_CONSTEXPR size_type next_power_of_two() CASTLE_NOEXCEPT
{
    if (N == 0)
    {
        return 1;
    }

    size_type v = N - 1;
    for (size_type i = 1; i < sizeof(size_type) * CHAR_BIT; i *= 2)
    {
        v |= v >> i;
    }

    return v + 1;
}

/**
 * @brief Compile-time wrapper for `next_power_of_two<N>()`.
 * @tparam N Compile-time value to round.
 */
template <size_type N>
struct next_power_of_two_const
{
    static CASTLE_CONSTEXPR size_type value = next_power_of_two<N>();
};

/**
 * @brief Rounds a value down to the previous power of two.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param v Value to round.
 * @return The largest power of two less than or equal to `v`. Returns 0 when `v` is 0.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
previous_power_of_two(T v) CASTLE_NOEXCEPT
{
    if (v == 0)
    {
        return 0;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(v);

    for (size_type i = 1; i < sizeof(T) * CHAR_BIT; i *= 2)
    {
        uval |= uval >> i; // Smear every bit below the topmost 1.
    }

    return static_cast<T>(uval - (uval >> 1));
}

/**
 * @brief Compile-time previous-power-of-two query for `castle::size_type` values.
 * @tparam N Compile-time value to round.
 * @return The largest power of two less than or equal to `N`. Returns 0 when `N` is 0.
 */
template <size_type N>
CASTLE_CONSTEXPR size_type previous_power_of_two() CASTLE_NOEXCEPT
{
    if (N == 0)
    {
        return 0;
    }

    size_type v = N;
    for (size_type i = 1; i < sizeof(size_type) * CHAR_BIT; i *= 2)
    {
        v |= v >> i;
    }

    return v - (v >> 1);
}

/**
 * @brief Compile-time wrapper for `previous_power_of_two<N>()`.
 * @tparam N Compile-time value to inspect.
 */
template <size_type N>
struct previous_power_of_two_const
{
    static CASTLE_CONSTEXPR size_type value = previous_power_of_two<N>();
};

/**
 * @brief Rounds a value up to the next multiple of a power-of-two alignment.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Value to align.
 * @param alignment Requested alignment.
 * @return `value` rounded up to the next aligned boundary.
 * @warning `alignment` is not validated; callers are expected to pass a non-zero power of two.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
align_up(T value, T alignment) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    UnsignedT mask = static_cast<UnsignedT>(alignment) - UnsignedT{1U};
    return static_cast<T>((uval + mask) & ~mask);
}

/**
 * @brief Rounds a value down to the previous multiple of a power-of-two alignment.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Value to align.
 * @param alignment Requested alignment.
 * @return `value` rounded down to the previous aligned boundary.
 * @warning `alignment` is not validated; callers are expected to pass a non-zero power of two.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
align_down(T value, T alignment) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    UnsignedT mask = static_cast<UnsignedT>(alignment) - UnsignedT{1U};
    return static_cast<T>(uval & ~mask);
}

/**
 * @brief Tests whether a value is already aligned.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Value to inspect.
 * @param alignment Requested alignment.
 * @return `true` when `value` is a multiple of `alignment`; otherwise `false`.
 * @warning `alignment` is not validated; callers are expected to pass a non-zero power of two.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
is_aligned(T value, T alignment) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT mask = static_cast<UnsignedT>(alignment) - UnsignedT{1U};
    return (static_cast<UnsignedT>(value) & mask) == UnsignedT{0U};
}

/**
 * @brief Returns the sign of a runtime value.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param v Value to inspect.
 * @return `-1` when `v` is negative, `0` when `v` is zero, and `1` when `v` is positive.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, int>
sign(T v) CASTLE_NOEXCEPT
{
    return (v > 0) - (v < 0);
}

/**
 * @brief Returns the sign of a compile-time `castle::size_type` value.
 * @tparam N Compile-time value to inspect.
 * @return `0` when `N` is 0; otherwise `1`.
 * @note `N` is a `castle::size_type`, so this overload cannot represent negative values.
 */
template <size_type N>
CASTLE_CONSTEXPR int sign() CASTLE_NOEXCEPT
{
    return (N > 0) - (N < 0);
}

}
}

#endif

// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bit_count.hpp
 * @brief Population-count and bit-position counting helpers for integer values.
 *
 * Use this header when code needs deterministic counts of set bits, zero bits,
 * leading or trailing zeros, bit widths, or parity without relying on compiler
 * intrinsics or lookup tables.
 *
 * Key constraints:
 * - Accepts Castle integer types that satisfy `castle::meta::is_valid_integer`.
 * - Generic overloads operate on the corresponding unsigned representation.
 * - No allocation, exceptions, RTTI, virtual dispatch, or STL facilities are used.
 */
#ifndef CASTLE_BIT_BIT_COUNT_HPP
#define CASTLE_BIT_BIT_COUNT_HPP

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
 * @brief Counts the number of set bits in a 64-bit value.
 * @param value Source value.
 * @return Number of 1 bits in `value`.
 * @note Uses a branch-free SWAR reduction.
 */
CASTLE_CONSTEXPR uint32_t popcount(uint64_t value) CASTLE_NOEXCEPT
{
    uint64_t x = value;
    x -= (x >> 1) & 0x5555555555555555ULL; // Count bits in each 2-bit lane.
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL); // Merge into 4-bit lane counts.
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL; // Keep one population count per byte.
    return static_cast<uint32_t>((x * 0x0101010101010101ULL) >> 56);
}

/**
 * @brief Counts the number of set bits in an integer value.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return Number of 1 bits in the corresponding unsigned representation of `value`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
popcount(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    uint64_t uval64 = static_cast<UnsignedT>(value);

    return popcount(uval64);
}

/**
 * @brief Alias for `popcount`.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return Number of 1 bits in the corresponding unsigned representation of `value`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
count_ones(T value) CASTLE_NOEXCEPT
{
    return popcount(value);
}

/**
 * @brief Counts the number of zero bits across the full width of `T`.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return `sizeof(T) * CHAR_BIT - popcount(value)`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
count_zeros(T value) CASTLE_NOEXCEPT
{
    return sizeof(T) * CHAR_BIT - popcount(value);
}

/**
 * @brief Counts consecutive zero bits from the most-significant bit toward the least-significant bit.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return Number of leading zero bits. Returns `sizeof(T) * CHAR_BIT` when `value` is 0.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
count_leading_zeros(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0)
    {
        return sizeof(T) * CHAR_BIT;
    }

    uint32_t count = 0;

    for (size_type i = (sizeof(T) * CHAR_BIT >> 1); i > 0; i >>= 1)
    {
        if ((uval >> i) == 0)
        {
            count += static_cast<uint32_t>(i);
        }
        else
        {
            uval >>= i; // Continue searching within the half that still contains the topmost 1 bit.
        }
    }

    return count;
}

/**
 * @brief Counts consecutive zero bits from the least-significant bit toward the most-significant bit.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return Number of trailing zero bits. Returns `sizeof(T) * CHAR_BIT` when `value` is 0.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
count_trailing_zeros(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0)
    {
        return sizeof(T) * CHAR_BIT;
    }

    uint32_t count = 0;

    for (size_type i = (sizeof(T) * CHAR_BIT >> 1); i > 0; i >>= 1)
    {
        if ((uval & ((UnsignedT{1U} << i) - UnsignedT{1U})) == 0) // Test whether the lower i bits are all zero.
        {
            count += static_cast<uint32_t>(i);
            uval >>= i;
        }
    }

    return count;
}

/**
 * @brief Returns the number of bits required to represent a non-zero value.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return `0` when `value` is 0; otherwise `floor(log2(u)) + 1`, where `u` is
 *         the corresponding unsigned representation of `value`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
bit_width(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0)
    {
        return 0;
    }

    return sizeof(T) * CHAR_BIT - count_leading_zeros(uval);
}

/**
 * @brief Returns `floor(log2(u))` for a non-zero value.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return `bit_width(value) - 1` for non-zero inputs.
 * @warning `log2(0)` is mathematically undefined; this implementation returns 0 when `value` is 0.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
log2_floor(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0)
    {
        return 0;
    }

    return bit_width(uval) - 1U;
}

/**
 * @brief Returns the parity of a value.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return `1` when `value` contains an odd number of set bits; otherwise `0`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
parity(T value) CASTLE_NOEXCEPT
{
    return popcount(value) & 1;
}

/**
 * @brief Returns the parity of a compile-time constant.
 * @tparam N Compile-time value to inspect.
 * @return `1` when `N` contains an odd number of set bits; otherwise `0`.
 */
template <size_type N>
CASTLE_CONSTEXPR uint32_t parity() CASTLE_NOEXCEPT
{
    return popcount(static_cast<uint64_t>(N)) & 1;
}

}
}

#endif

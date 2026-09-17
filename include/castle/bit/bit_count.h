#ifndef CASTLE_BIT_BIT_COUNT_H
#define CASTLE_BIT_BIT_COUNT_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/core/traits.h"

#include <stdint.h>
#include <limits.h>

namespace castle
{
namespace bit
{

// ──────────────────────────────────────────────────────────────
// popcount — count the number of set bits (1s) in a value.
// Uses the SWAR (SIMD Within A Register) algorithm.
// Branch-free, CASTLE_CONSTEXPR, no lookup tables.
// ──────────────────────────────────────────────────────────────

CASTLE_CONSTEXPR uint32_t popcount(uint64_t value) CASTLE_NOEXCEPT
{
    uint64_t x = value;
    x -= (x >> 1) & 0x5555555555555555ULL;
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return static_cast<uint32_t>((x * 0x0101010101010101ULL) >> 56);
}

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
popcount(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    uint64_t uval64 = static_cast<UnsignedT>(value);

    return popcount(uval64);
}

// ──────────────────────────────────────────────────────────────
// count_ones / count_zeros — convenience aliases.
//   count_ones  returns the number of 1-bits (same as popcount).
//   count_zeros returns the number of 0-bits.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
count_ones(T value) CASTLE_NOEXCEPT
{
    return popcount(value);
}

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
count_zeros(T value) CASTLE_NOEXCEPT
{
    return sizeof(T) * CHAR_BIT - popcount(value);
}

// ──────────────────────────────────────────────────────────────
// count_leading_zeros — number of consecutive zero bits
// starting from the most-significant bit.
// Returns sizeof(T)*CHAR_BIT for input 0.
// Uses a branch-free binary-search approach.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
count_leading_zeros(T value) CASTLE_NOEXCEPT
{
    // Make value unsigned to guarantee safe bitwise right-shifts
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0)
    {
        return sizeof(T) * CHAR_BIT;
    }

    uint32_t count = 0;

    // Binary search by shifting the value RIGHT instead of left
    for (size_type i = (sizeof(T) * CHAR_BIT >> 1); i > 0; i >>= 1)
    {
        if ((uval >> i) == 0)
        {
            count += static_cast<uint32_t>(i);
        }
        else
        {
            uval >>= i; // Move down to check the lower half
        }
    }

    return count;
}

// ──────────────────────────────────────────────────────────────
// count_trailing_zeros — number of consecutive zero bits
// starting from the least-significant bit.
// ──────────────────────────────────────────────────────────────

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
        if ((uval & ((UnsignedT{1U} << i) - UnsignedT{1U})) == 0)
        {
            count += static_cast<uint32_t>(i);
            uval >>= i;
        }
    }

    return count;
}

// ──────────────────────────────────────────────────────────────
// bit_width — minimum number of bits needed to represent
// a non-negative value (equivalent to floor(log2(v)) + 1).
// Returns 0 for input 0.
// ──────────────────────────────────────────────────────────────

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

// ──────────────────────────────────────────────────────────────
// log2_floor — floor(log2(v)). Undefined for v == 0.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
log2_floor(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0)
    {
        return 0; // defensive; mathematically undefined
    }

    return bit_width(uval) - 1U;
}

// ──────────────────────────────────────────────────────────────
// parity — returns 1 if the number of set bits is odd,
// 0 if even. Useful for error-detection schemes.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, uint32_t>
parity(T value) CASTLE_NOEXCEPT
{
    return popcount(value) & 1;
}

template <size_type N>
CASTLE_CONSTEXPR uint32_t parity() CASTLE_NOEXCEPT
{
    return popcount(static_cast<uint64_t>(N)) & 1;
}

} // namespace bit
} // namespace castle

#endif // CASTLE_BIT_BIT_COUNT_H

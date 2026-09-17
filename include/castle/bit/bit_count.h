#pragma once

#include "castle/types/traits.h"

#include <cstdint>
#include <climits>

using namespace castle::types;

namespace castle
{
namespace bit
{

// ──────────────────────────────────────────────────────────────
// popcount — count the number of set bits (1s) in a value.
// Uses the SWAR (SIMD Within A Register) algorithm.
// Branch-free, constexpr, no lookup tables.
// ──────────────────────────────────────────────────────────────

constexpr std::uint32_t popcount(std::uint64_t value) noexcept
{
    std::uint64_t x = value;
    x -= (x >> 1) & 0x5555555555555555ULL;
    x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
    x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return static_cast<std::uint32_t>((x * 0x0101010101010101ULL) >> 56);
}

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, std::uint32_t>
popcount(T value) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;
    uint64_t uval64 = static_cast<UnsignedT>(value);

    return popcount(uval64);
}

// ──────────────────────────────────────────────────────────────
// count_ones / count_zeros — convenience aliases.
//   count_ones  returns the number of 1-bits (same as popcount).
//   count_zeros returns the number of 0-bits.
// ──────────────────────────────────────────────────────────────

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, std::uint32_t>
count_ones(T value) noexcept
{
    return popcount(value);
}

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, std::uint32_t>
count_zeros(T value) noexcept
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
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, std::uint32_t>
count_leading_zeros(T value) noexcept
{
    // Make value unsigned to guarantee safe bitwise right-shifts
    using UnsignedT = typename std::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0)
        return sizeof(T) * CHAR_BIT;

    std::uint32_t count = 0;

    // Binary search by shifting the value RIGHT instead of left
    for (std::size_t i = (sizeof(T) * CHAR_BIT >> 1); i > 0; i >>= 1)
    {
        if ((uval >> i) == 0)
        {
            count += static_cast<std::uint32_t>(i);
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
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, std::uint32_t>
count_trailing_zeros(T value) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0)
        return sizeof(T) * CHAR_BIT;

    std::uint32_t count = 0;

    for (std::size_t i = (sizeof(T) * CHAR_BIT >> 1); i > 0; i >>= 1)
    {
        if ((uval & ((UnsignedT{1U} << i) - UnsignedT{1U})) == 0)
        {
            count += static_cast<std::uint32_t>(i);
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
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, std::uint32_t>
bit_width(T value) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0)
        return 0;

    return sizeof(T) * CHAR_BIT - count_leading_zeros(uval);
}

// ──────────────────────────────────────────────────────────────
// log2_floor — floor(log2(v)). Undefined for v == 0.
// ──────────────────────────────────────────────────────────────

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, std::uint32_t>
log2_floor(T value) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0)
        return 0; // defensive; mathematically undefined

    return bit_width(uval) - 1U;
}

// ──────────────────────────────────────────────────────────────
// parity — returns 1 if the number of set bits is odd,
// 0 if even. Useful for error-detection schemes.
// ──────────────────────────────────────────────────────────────

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, std::uint32_t>
parity(T value) noexcept
{
    return popcount(value) & 1;
}

template <std::size_t N>
constexpr std::uint32_t parity() noexcept
{
    return popcount(static_cast<std::uint64_t>(N)) & 1;
}

} // namespace bit
} // namespace castle
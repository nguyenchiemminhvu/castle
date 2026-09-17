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
// rotate_left — circular left rotation of bits.
//   shift : number of bit positions to rotate.
// Bits shifted out at the MSB re-enter at the LSB.
// ──────────────────────────────────────────────────────────────

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
rotate_left(T value, std::uint32_t shift) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    constexpr std::uint32_t bit_count = sizeof(T) * CHAR_BIT;

    shift %= bit_count;
    if (shift == 0)
    {
        return value;
    }

    uval = (uval << shift) | (uval >> (bit_count - shift));
    return static_cast<T>(uval);
}

// ──────────────────────────────────────────────────────────────
// rotate_right — circular right rotation of bits.
//   shift : number of bit positions to rotate.
// Bits shifted out at the LSB re-enter at the MSB.
// ──────────────────────────────────────────────────────────────

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
rotate_right(T value, std::uint32_t shift) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    constexpr std::uint32_t bit_count = sizeof(T) * CHAR_BIT;

    shift %= bit_count;
    if (shift == 0)
    {
        return value;
    }

    uval = (uval >> shift) | (uval << (bit_count - shift));
    return static_cast<T>(uval);
}

} // namespace bit
} // namespace castle
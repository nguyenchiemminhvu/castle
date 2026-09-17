#ifndef CASTLE_BIT_BIT_REVERSE_H
#define CASTLE_BIT_BIT_REVERSE_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

#include <stdint.h>
#include <limits.h>

namespace castle
{
namespace bit
{

// ──────────────────────────────────────────────────────────────
// reverse_bits — reverse_bits the order of all bits in an integral value.
// Uses parallel swap (SWAR) for each fixed-width size.
// E.g. reverse_bits(0b10110000) → 0b00001101.
// ──────────────────────────────────────────────────────────────

CASTLE_CONSTEXPR uint8_t reverse_bits(uint8_t n) CASTLE_NOEXCEPT
{
    n = ((n & 0xF0) >> 4) | ((n & 0x0F) << 4);
    n = ((n & 0xCC) >> 2) | ((n & 0x33) << 2);
    n = ((n & 0xAA) >> 1) | ((n & 0x55) << 1);
    return n;
}

CASTLE_CONSTEXPR uint16_t reverse_bits(uint16_t n) CASTLE_NOEXCEPT
{
    n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
    n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
    n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
    n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1);
    return n;
}

CASTLE_CONSTEXPR uint32_t reverse_bits(uint32_t n) CASTLE_NOEXCEPT
{
    n = ((n & 0xFFFF0000) >> 16) | ((n & 0x0000FFFF) << 16);
    n = ((n & 0xFF00FF00) >> 8)  | ((n & 0x00FF00FF) << 8);
    n = ((n & 0xF0F0F0F0) >> 4)  | ((n & 0x0F0F0F0F) << 4);
    n = ((n & 0xCCCCCCCC) >> 2)  | ((n & 0x33333333) << 2);
    n = ((n & 0xAAAAAAAA) >> 1)  | ((n & 0x55555555) << 1);
    return n;
}

CASTLE_CONSTEXPR uint64_t reverse_bits(uint64_t n) CASTLE_NOEXCEPT
{
    n = ((n & 0xFFFFFFFF00000000ULL) >> 32) | ((n & 0x00000000FFFFFFFFULL) << 32);
    n = ((n & 0xFFFF0000FFFF0000ULL) >> 16) | ((n & 0x0000FFFF0000FFFFULL) << 16);
    n = ((n & 0xFF00FF00FF00FF00ULL) >> 8)  | ((n & 0x00FF00FF00FF00FFULL) << 8);
    n = ((n & 0xF0F0F0F0F0F0F0F0ULL) >> 4)  | ((n & 0x0F0F0F0F0F0F0F0FULL) << 4);
    n = ((n & 0xCCCCCCCCCCCCCCCCULL) >> 2)  | ((n & 0x3333333333333333ULL) << 2);
    n = ((n & 0xAAAAAAAAAAAAAAAAULL) >> 1)  | ((n & 0x5555555555555555ULL) << 1);
    return n;
}

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
reverse_bits(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(reverse_bits(uval));
}

// ──────────────────────────────────────────────────────────────
// byte_swap — reverse the byte order of an integral value.
// Essential for converting between little-endian and big-endian
// representations in embedded communication protocols.
// ──────────────────────────────────────────────────────────────

CASTLE_CONSTEXPR uint8_t byte_swap(uint8_t v) CASTLE_NOEXCEPT
{
    return v; // single byte — nothing to swap
}

CASTLE_CONSTEXPR uint16_t byte_swap(uint16_t v) CASTLE_NOEXCEPT
{
    return static_cast<uint16_t>(
        ((v & 0x00FFU) << 8) |
        ((v & 0xFF00U) >> 8)
    );
}

CASTLE_CONSTEXPR uint32_t byte_swap(uint32_t v) CASTLE_NOEXCEPT
{
    v = ((v & 0x00FF00FFU) << 8)  | ((v & 0xFF00FF00U) >> 8);
    v = ((v & 0x0000FFFFU) << 16) | ((v & 0xFFFF0000U) >> 16);
    return v;
}

CASTLE_CONSTEXPR uint64_t byte_swap(uint64_t v) CASTLE_NOEXCEPT
{
    v = ((v & 0x00FF00FF00FF00FFULL) << 8)  | ((v & 0xFF00FF00FF00FF00ULL) >> 8);
    v = ((v & 0x0000FFFF0000FFFFULL) << 16) | ((v & 0xFFFF0000FFFF0000ULL) >> 16);
    v = ((v & 0x00000000FFFFFFFFULL) << 32) | ((v & 0xFFFFFFFF00000000ULL) >> 32);
    return v;
}

// Generic dispatcher for signed / other integral types.
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
byte_swap(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    return static_cast<T>(byte_swap(static_cast<UnsignedT>(value)));
}

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
reverse_bytes(T value) CASTLE_NOEXCEPT
{
    return byte_swap(value);
}

} // namespace bit
} // namespace castle

#endif // CASTLE_BIT_BIT_REVERSE_H

#pragma once

#include "castle/types/traits.h"
#include "castle/bit/bit_endian.h"

#include <cstdint>

using namespace castle::types;

namespace castle
{
namespace bit
{

// ──────────────────────────────────────────────────────────────
// reverse_bits — reverse_bits the order of all bits in an integral value.
// Uses parallel swap (SWAR) for each fixed-width size.
// E.g. reverse_bits(0b10110000) → 0b00001101.
// ──────────────────────────────────────────────────────────────

constexpr uint8_t reverse_bits(uint8_t n) noexcept
{
    n = ((n & 0xF0) >> 4) | ((n & 0x0F) << 4);
    n = ((n & 0xCC) >> 2) | ((n & 0x33) << 2);
    n = ((n & 0xAA) >> 1) | ((n & 0x55) << 1);
    return n;
}

constexpr uint16_t reverse_bits(uint16_t n) noexcept
{
    n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8);
    n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4);
    n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2);
    n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1);
    return n;
}

constexpr uint32_t reverse_bits(uint32_t n) noexcept
{
    n = ((n & 0xFFFF0000) >> 16) | ((n & 0x0000FFFF) << 16);
    n = ((n & 0xFF00FF00) >> 8)  | ((n & 0x00FF00FF) << 8);
    n = ((n & 0xF0F0F0F0) >> 4)  | ((n & 0x0F0F0F0F) << 4);
    n = ((n & 0xCCCCCCCC) >> 2)  | ((n & 0x33333333) << 2);
    n = ((n & 0xAAAAAAAA) >> 1)  | ((n & 0x55555555) << 1);
    return n;
}

constexpr uint64_t reverse_bits(uint64_t n) noexcept
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
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
reverse_bits(T value) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(reverse_bits(uval));
}

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
reverse_bytes(T value) noexcept
{
    return byte_swap(value);
}

} // namespace bit
} // namespace castle
// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bit_reverse.hpp
 * @brief Bit-order and byte-order reversal helpers for fixed-width integers.
 *
 * Use this header when protocol code, serialization logic, or register access
 * needs to reverse the order of bits within a value or swap byte order across
 * the full width of an integer.
 *
 * Key constraints:
 * - Generic overloads operate on the corresponding unsigned representation.
 * - Bit reversal is about bit positions inside the value, not memory endianness.
 * - No allocation, exceptions, RTTI, virtual dispatch, or STL facilities are used.
 */
#ifndef CASTLE_BIT_BIT_REVERSE_HPP
#define CASTLE_BIT_BIT_REVERSE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"

#include <stdint.h>
#include <limits.h>

namespace castle
{
namespace bit
{

/**
 * @brief Reverses the bit order of an 8-bit value.
 * @param n Value to reverse.
 * @return `n` with bit 0 moved to bit 7, bit 1 moved to bit 6, and so on.
 */
CASTLE_CONSTEXPR uint8_t reverse_bits(uint8_t n) CASTLE_NOEXCEPT
{
    n = ((n & 0xF0) >> 4) | ((n & 0x0F) << 4); // Swap nibbles.
    n = ((n & 0xCC) >> 2) | ((n & 0x33) << 2); // Swap 2-bit groups.
    n = ((n & 0xAA) >> 1) | ((n & 0x55) << 1); // Swap adjacent bits.
    return n;
}

/**
 * @brief Reverses the bit order of a 16-bit value.
 * @param n Value to reverse.
 * @return `n` with its 16 bit positions reversed.
 */
CASTLE_CONSTEXPR uint16_t reverse_bits(uint16_t n) CASTLE_NOEXCEPT
{
    n = ((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8); // Swap bytes.
    n = ((n & 0xF0F0) >> 4) | ((n & 0x0F0F) << 4); // Swap nibbles.
    n = ((n & 0xCCCC) >> 2) | ((n & 0x3333) << 2); // Swap 2-bit groups.
    n = ((n & 0xAAAA) >> 1) | ((n & 0x5555) << 1); // Swap adjacent bits.
    return n;
}

/**
 * @brief Reverses the bit order of a 32-bit value.
 * @param n Value to reverse.
 * @return `n` with its 32 bit positions reversed.
 */
CASTLE_CONSTEXPR uint32_t reverse_bits(uint32_t n) CASTLE_NOEXCEPT
{
    n = ((n & 0xFFFF0000) >> 16) | ((n & 0x0000FFFF) << 16); // Swap 16-bit halves.
    n = ((n & 0xFF00FF00) >> 8)  | ((n & 0x00FF00FF) << 8); // Swap bytes.
    n = ((n & 0xF0F0F0F0) >> 4)  | ((n & 0x0F0F0F0F) << 4); // Swap nibbles.
    n = ((n & 0xCCCCCCCC) >> 2)  | ((n & 0x33333333) << 2); // Swap 2-bit groups.
    n = ((n & 0xAAAAAAAA) >> 1)  | ((n & 0x55555555) << 1); // Swap adjacent bits.
    return n;
}

/**
 * @brief Reverses the bit order of a 64-bit value.
 * @param n Value to reverse.
 * @return `n` with its 64 bit positions reversed.
 */
CASTLE_CONSTEXPR uint64_t reverse_bits(uint64_t n) CASTLE_NOEXCEPT
{
    n = ((n & 0xFFFFFFFF00000000ULL) >> 32) | ((n & 0x00000000FFFFFFFFULL) << 32); // Swap 32-bit halves.
    n = ((n & 0xFFFF0000FFFF0000ULL) >> 16) | ((n & 0x0000FFFF0000FFFFULL) << 16); // Swap 16-bit lanes.
    n = ((n & 0xFF00FF00FF00FF00ULL) >> 8)  | ((n & 0x00FF00FF00FF00FFULL) << 8); // Swap bytes.
    n = ((n & 0xF0F0F0F0F0F0F0F0ULL) >> 4)  | ((n & 0x0F0F0F0F0F0F0F0FULL) << 4); // Swap nibbles.
    n = ((n & 0xCCCCCCCCCCCCCCCCULL) >> 2)  | ((n & 0x3333333333333333ULL) << 2); // Swap 2-bit groups.
    n = ((n & 0xAAAAAAAAAAAAAAAAULL) >> 1)  | ((n & 0x5555555555555555ULL) << 1); // Swap adjacent bits.
    return n;
}

/**
 * @brief Reverses the bit order of an integer value.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Value to reverse.
 * @return `value` with its bit positions reversed across the full width of `T`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
reverse_bits(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(reverse_bits(uval));
}

/**
 * @brief Reverses the byte order of an 8-bit value.
 * @param v Value to reverse.
 * @return `v` unchanged.
 */
CASTLE_CONSTEXPR uint8_t byte_swap(uint8_t v) CASTLE_NOEXCEPT
{
    return v;
}

/**
 * @brief Reverses the byte order of a 16-bit value.
 * @param v Value to reverse.
 * @return `v` with its two bytes swapped.
 */
CASTLE_CONSTEXPR uint16_t byte_swap(uint16_t v) CASTLE_NOEXCEPT
{
    return static_cast<uint16_t>(
        ((v & 0x00FFU) << 8) |
        ((v & 0xFF00U) >> 8)
    );
}

/**
 * @brief Reverses the byte order of a 32-bit value.
 * @param v Value to reverse.
 * @return `v` with its four bytes reversed.
 */
CASTLE_CONSTEXPR uint32_t byte_swap(uint32_t v) CASTLE_NOEXCEPT
{
    v = ((v & 0x00FF00FFU) << 8)  | ((v & 0xFF00FF00U) >> 8); // Swap adjacent bytes.
    v = ((v & 0x0000FFFFU) << 16) | ((v & 0xFFFF0000U) >> 16); // Swap 16-bit halves.
    return v;
}

/**
 * @brief Reverses the byte order of a 64-bit value.
 * @param v Value to reverse.
 * @return `v` with its eight bytes reversed.
 */
CASTLE_CONSTEXPR uint64_t byte_swap(uint64_t v) CASTLE_NOEXCEPT
{
    v = ((v & 0x00FF00FF00FF00FFULL) << 8)  | ((v & 0xFF00FF00FF00FF00ULL) >> 8); // Swap adjacent bytes.
    v = ((v & 0x0000FFFF0000FFFFULL) << 16) | ((v & 0xFFFF0000FFFF0000ULL) >> 16); // Swap 16-bit lanes.
    v = ((v & 0x00000000FFFFFFFFULL) << 32) | ((v & 0xFFFFFFFF00000000ULL) >> 32); // Swap 32-bit halves.
    return v;
}

/**
 * @brief Reverses the byte order of an integer value.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Value to reverse.
 * @return `value` with its byte order reversed across the full width of `T`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
byte_swap(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    return static_cast<T>(byte_swap(static_cast<UnsignedT>(value)));
}

/**
 * @brief Alias for `byte_swap`.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Value to reverse.
 * @return `byte_swap(value)`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
reverse_bytes(T value) CASTLE_NOEXCEPT
{
    return byte_swap(value);
}

}
}

#endif

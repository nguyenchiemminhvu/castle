// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bit_rotate.hpp
 * @brief Circular bit-rotation helpers for fixed-width integers.
 *
 * Use this header when a value must be rotated rather than shifted, such as in
 * checksums, hashes, protocol packing, or register encodings that wrap bits
 * around the storage width.
 *
 * Key constraints:
 * - Accepts Castle integer types that satisfy `castle::meta::is_valid_integer`.
 * - Shift counts are reduced modulo the bit width of `T`.
 * - No allocation, exceptions, RTTI, virtual dispatch, or STL facilities are used.
 */
#ifndef CASTLE_BIT_BIT_ROTATE_HPP
#define CASTLE_BIT_BIT_ROTATE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

#include <stdint.h>
#include <limits.h>

namespace castle
{
namespace bit
{

/**
 * @brief Rotates an integer value to the left.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Value to rotate.
 * @param shift Number of bit positions to rotate.
 * @return `value` rotated left by `shift` positions within the full width of `T`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
rotate_left(T value, uint32_t shift) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    CASTLE_CONSTEXPR uint32_t bit_count = sizeof(T) * CHAR_BIT;

    shift %= bit_count;
    if (shift == 0)
    {
        return value; // Avoid shifting by the full storage width after the modulo reduction.
    }

    uval = (uval << shift) | (uval >> (bit_count - shift));
    return static_cast<T>(uval);
}

/**
 * @brief Rotates an integer value to the right.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Value to rotate.
 * @param shift Number of bit positions to rotate.
 * @return `value` rotated right by `shift` positions within the full width of `T`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
rotate_right(T value, uint32_t shift) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    CASTLE_CONSTEXPR uint32_t bit_count = sizeof(T) * CHAR_BIT;

    shift %= bit_count;
    if (shift == 0)
    {
        return value; // Avoid shifting by the full storage width after the modulo reduction.
    }

    uval = (uval >> shift) | (uval << (bit_count - shift));
    return static_cast<T>(uval);
}

}
}

#endif

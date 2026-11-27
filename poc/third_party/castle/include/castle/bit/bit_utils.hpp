// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bit_utils.hpp
 * @brief Helpers for isolating set bits and extracting or inserting bit fields.
 *
 * Use this header when code needs to isolate the lowest or highest set bit, or
 * read and write contiguous bit fields inside register-sized integers.
 *
 * Key constraints:
 * - Accepts Castle integer types that satisfy `castle::meta::is_valid_integer`.
 * - Runtime field helpers guard zero widths and out-of-range starting positions, but they do not fully validate every possible range.
 * - No allocation, exceptions, RTTI, virtual dispatch, or STL facilities are used.
 */
#ifndef CASTLE_BIT_BIT_UTILS_HPP
#define CASTLE_BIT_BIT_UTILS_HPP

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
 * @brief Isolates the lowest set bit of a value.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return A value with only the lowest set bit preserved. Returns 0 when `value` is 0.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
extract_lowest_set_bit(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval & -uval); // Two's-complement trick: keep only the least-significant 1 bit.
}

/**
 * @brief Isolates the highest set bit of a value.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return A value with only the highest set bit preserved. Returns 0 when `value` is 0.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
extract_highest_set_bit(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0)
    {
        return 0;
    }

    for (size_type i = 1; i < sizeof(T) * CHAR_BIT; i *= 2)
    {
        uval |= uval >> i; // Smear the topmost 1 bit downward.
    }

    return static_cast<T>(uval - (uval >> 1));
}

/**
 * @brief Extracts a contiguous bit field and right-aligns it.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @param start_bit Zero-based least-significant bit position of the field.
 * @param width Number of bits in the field.
 * @return The selected field shifted down to bit 0. Returns 0 when `width` is 0
 *         or when `start_bit` is outside the bit width of `T`.
 * @note When `width` is greater than or equal to the bit width of `T`, the
 *       result is `value` shifted right by `start_bit`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
extract_field(T value, uint32_t start_bit, uint32_t width) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (width == 0 || start_bit >= sizeof(T) * CHAR_BIT)
    {
        return static_cast<T>(UnsignedT{0U});
    }

    if (width >= sizeof(T) * CHAR_BIT)
    {
        return static_cast<T>(uval >> start_bit);
    }

    UnsignedT mask = (UnsignedT{1U} << width) - UnsignedT{1U};
    return static_cast<T>((uval >> start_bit) & mask);
}

/**
 * @brief Extracts a compile-time-selected contiguous bit field and right-aligns it.
 * @tparam start_bit Zero-based least-significant bit position of the field.
 * @tparam width Number of bits in the field.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return The selected field shifted down to bit 0.
 * @note This overload enforces `start_bit + width <= sizeof(T) * CHAR_BIT` and `width > 0` with `static_assert`.
 */
template <size_type start_bit, size_type width, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
extract_field(T value) CASTLE_NOEXCEPT
{
    static_assert(start_bit + width <= sizeof(T) * CHAR_BIT,
                  "extract_field: start_bit + width exceeds bit width of T");
    static_assert(width > 0, "extract_field: width must be > 0");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    CASTLE_CONSTEXPR UnsignedT mask = (UnsignedT{1U} << width) - UnsignedT{1U};
    return static_cast<T>((uval >> start_bit) & mask);
}

/**
 * @brief Replaces a contiguous bit field inside a destination value.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param dest Destination value to update.
 * @param field_val Replacement field value. Bits above `width` are discarded.
 * @param start_bit Zero-based least-significant bit position of the field.
 * @param width Number of bits in the field.
 * @return `dest` with the selected field replaced. Returns `dest` unchanged when
 *         `width` is 0 or when `start_bit` is outside the bit width of `T`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
insert_field(T dest, T field_val, uint32_t start_bit, uint32_t width) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT udest = static_cast<UnsignedT>(dest);
    UnsignedT ufield = static_cast<UnsignedT>(field_val);

    if (width == 0 || start_bit >= sizeof(T) * CHAR_BIT)
    {
        return dest;
    }

    UnsignedT mask;
    if (width >= sizeof(T) * CHAR_BIT)
    {
        mask = ~UnsignedT{0U};
    }
    else
    {
        mask = (UnsignedT{1U} << width) - UnsignedT{1U};
    }

    ufield &= mask; // Ignore bits that do not fit inside the destination field.
    udest &= ~(mask << start_bit); // Clear the destination field before inserting the replacement bits.
    udest |= (ufield << start_bit);
    return static_cast<T>(udest);
}

/**
 * @brief Replaces a compile-time-selected contiguous bit field inside a destination value.
 * @tparam start_bit Zero-based least-significant bit position of the field.
 * @tparam width Number of bits in the field.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param dest Destination value to update.
 * @param field_val Replacement field value. Bits above `width` are discarded.
 * @return `dest` with the selected field replaced.
 * @note This overload enforces `start_bit + width <= sizeof(T) * CHAR_BIT` and `width > 0` with `static_assert`.
 */
template <size_type start_bit, size_type width, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
insert_field(T dest, T field_val) CASTLE_NOEXCEPT
{
    static_assert(start_bit + width <= sizeof(T) * CHAR_BIT,
                  "insert_field: start_bit + width exceeds bit width of T");
    static_assert(width > 0, "insert_field: width must be > 0");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT udest = static_cast<UnsignedT>(dest);
    UnsignedT ufield = static_cast<UnsignedT>(field_val);
    CASTLE_CONSTEXPR UnsignedT mask = (UnsignedT{1U} << width) - UnsignedT{1U};

    ufield &= mask;
    udest &= ~(mask << start_bit);
    udest |= (ufield << start_bit);
    return static_cast<T>(udest);
}

}
}

#endif

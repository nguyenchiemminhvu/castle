// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bit_core.hpp
 * @brief Single-bit test, set, clear, and toggle helpers for fixed-width integers.
 *
 * Use this header when register, protocol, or packed-field code needs
 * deterministic manipulation of individual bits without repeating raw shifts
 * and masks at each call site.
 *
 * Key constraints:
 * - Accepts Castle integer types that satisfy `castle::meta::is_valid_integer`.
 * - Runtime index overloads return a safe fallback when the index is out of range.
 * - No allocation, exceptions, RTTI, virtual dispatch, or STL facilities are used.
 */
#ifndef CASTLE_BIT_BIT_CORE_HPP
#define CASTLE_BIT_BIT_CORE_HPP

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
 * @brief Tests whether one bit is set.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @param bit_index Zero-based bit position to inspect.
 * @return `true` when the selected bit is 1; `false` when it is 0 or when
 *         `bit_index` is outside the bit width of `T`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
test(T value, uint32_t bit_index) CASTLE_NOEXCEPT
{
    if (bit_index >= sizeof(T) * CHAR_BIT)
    {
        return false;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return ((uval >> bit_index) & UnsignedT{1U}) != UnsignedT{0U};
}

/**
 * @brief Tests whether one compile-time-selected bit is set.
 * @tparam bit_index Zero-based bit position to inspect.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return `true` when the selected bit is 1; otherwise `false`.
 * @note This overload enforces `bit_index < sizeof(T) * CHAR_BIT` with
 *       `static_assert`.
 */
template <size_type bit_index, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
test(T value) CASTLE_NOEXCEPT
{
    static_assert(bit_index < sizeof(T) * CHAR_BIT, "bit_index is out of range for the type T");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return ((uval >> bit_index) & UnsignedT{1U}) != UnsignedT{0U};
}

/**
 * @brief Sets one bit.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @param bit_index Zero-based bit position to set.
 * @return `value` with the selected bit set to 1, or the original value when
 *         `bit_index` is outside the bit width of `T`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
set(T value, uint32_t bit_index) CASTLE_NOEXCEPT
{
    if (bit_index >= sizeof(T) * CHAR_BIT)
    {
        return value;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval | (UnsignedT{1U} << bit_index));
}

/**
 * @brief Sets one compile-time-selected bit.
 * @tparam bit_index Zero-based bit position to set.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return `value` with the selected bit set to 1.
 * @note This overload enforces `bit_index < sizeof(T) * CHAR_BIT` with
 *       `static_assert`.
 */
template <size_type bit_index, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
set(T value) CASTLE_NOEXCEPT
{
    static_assert(bit_index < sizeof(T) * CHAR_BIT, "bit_index is out of range for the type T");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval | (UnsignedT{1U} << bit_index));
}

/**
 * @brief Clears one bit.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @param bit_index Zero-based bit position to clear.
 * @return `value` with the selected bit forced to 0, or the original value
 *         when `bit_index` is outside the bit width of `T`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
clear(T value, uint32_t bit_index) CASTLE_NOEXCEPT
{
    if (bit_index >= sizeof(T) * CHAR_BIT)
    {
        return value;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval & static_cast<UnsignedT>(~(UnsignedT{1U} << bit_index)));
}

/**
 * @brief Clears one compile-time-selected bit.
 * @tparam bit_index Zero-based bit position to clear.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return `value` with the selected bit forced to 0.
 * @note This overload enforces `bit_index < sizeof(T) * CHAR_BIT` with
 *       `static_assert`.
 */
template <size_type bit_index, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
clear(T value) CASTLE_NOEXCEPT
{
    static_assert(bit_index < sizeof(T) * CHAR_BIT, "bit_index is out of range for the type T");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval & static_cast<UnsignedT>(~(UnsignedT{1U} << bit_index)));
}

/**
 * @brief Toggles one bit.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @param bit_index Zero-based bit position to toggle.
 * @return `value` with the selected bit inverted, or the original value when
 *         `bit_index` is outside the bit width of `T`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
toggle(T value, uint32_t bit_index) CASTLE_NOEXCEPT
{
    if (bit_index >= sizeof(T) * CHAR_BIT)
    {
        return value;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval ^ (UnsignedT{1U} << bit_index));
}

/**
 * @brief Toggles one compile-time-selected bit.
 * @tparam bit_index Zero-based bit position to toggle.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param value Source value.
 * @return `value` with the selected bit inverted.
 * @note This overload enforces `bit_index < sizeof(T) * CHAR_BIT` with
 *       `static_assert`.
 */
template <size_type bit_index, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
toggle(T value) CASTLE_NOEXCEPT
{
    static_assert(bit_index < sizeof(T) * CHAR_BIT, "bit_index is out of range for the type T");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval ^ (UnsignedT{1U} << bit_index));
}

}
}

#endif

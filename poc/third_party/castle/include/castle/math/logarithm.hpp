// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file logarithm.hpp
 * @brief Compile-time integer logarithm helpers.
 *
 * This header provides deterministic, template-based floor logarithms for
 * integral constants. Use it when bit widths, decimal digit counts, radix
 * sizing, or lookup-table dimensions must be derived at compile time without
 * floating-point code or runtime state.
 *
 * Example:
 * @code
 * static_assert(castle::math::logarithm<81U, 3U>::value == 4U, "base-3 log");
 * static_assert(castle::math::log2<1024U>::value == 10U, "base-2 log");
 * static_assert(castle::math::log10<999U>::value == 2U, "base-10 log");
 * @endcode
 *
 * @note Results are rounded down to the nearest integer exponent.
 */
#ifndef CASTLE_MATH_LOGARITHM_HPP
#define CASTLE_MATH_LOGARITHM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Computes the floor of the integer logarithm at compile time.
 * @tparam Value Input value to evaluate. The primary template requires
 *         `Value > 0`.
 * @tparam Base Logarithm base. Must satisfy `Base > 1`.
 * @note The result is exposed as `value`.
 * @warning `Base <= 1` is invalid. Direct instantiations with `Value == 0`
 *          use the explicit specialization below.
 */
template <size_type Value, size_type Base>
struct logarithm
{
    static_assert(Base > 1, "Base must be greater than 1");
    static_assert(Value > 0, "Value must be greater than 0");

    static CASTLE_CONSTEXPR size_type value =
        (Value >= Base)
        ? (1U + logarithm<Value / Base, Base>::value)
        : 0U;
};

/**
 * @brief Specialization for an input value of one.
 * @tparam Base Logarithm base. Must satisfy `Base > 1`.
 * @note `floor(log_Base(1))` is defined here as `0`.
 */
template <size_type Base>
struct logarithm<1U, Base>
{
    static_assert(Base > 1, "Base must be greater than 1");

    static CASTLE_CONSTEXPR size_type value = 0U;
};

/**
 * @brief Specialization for an input value of zero.
 * @tparam Base Logarithm base. Must satisfy `Base > 1`.
 * @note This header defines `logarithm<0U, Base>::value` as `0`.
 * @warning This is a library-defined edge-case result, not a mathematical
 *          logarithm.
 */
template <size_type Base>
struct logarithm<0U, Base>
{
    static_assert(Base > 1, "Base must be greater than 1");

    static CASTLE_CONSTEXPR size_type value = 0U;
};

/**
 * @brief Computes the floor base-2 logarithm at compile time.
 * @tparam Value Positive input value.
 * @note Equivalent to `logarithm<Value, 2U>`.
 * @warning `Value` must be greater than zero.
 */
template <size_type Value>
struct log2
{
    static_assert(Value > 0, "Value must be greater than 0");

    static CASTLE_CONSTEXPR size_type value = logarithm<Value, 2U>::value;
};

/**
 * @brief Computes the floor base-10 logarithm at compile time.
 * @tparam Value Positive input value.
 * @note Equivalent to `logarithm<Value, 10U>`.
 * @warning `Value` must be greater than zero.
 */
template <size_type Value>
struct log10
{
    static_assert(Value > 0, "Value must be greater than 0");

    static CASTLE_CONSTEXPR size_type value = logarithm<Value, 10U>::value;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_LOGARITHM_HPP
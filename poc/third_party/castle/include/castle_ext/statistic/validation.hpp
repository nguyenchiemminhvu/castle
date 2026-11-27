// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file validation.hpp
 * @brief Sample validation and result-type selection shared by `castle::statistic`.
 *
 * Every statistics building block accepts an arithmetic sample type `T` and
 * reports results in `result_t<T>`: `T` itself for floating-point samples
 * (so `float` pipelines stay in single precision on FPU-limited targets) and
 * `double` for integral samples. The helpers below reject NaN and infinity
 * without `<math.h>`, so every component shares one definition of a usable
 * sample.
 *
 * @code
 * #include "castle_ext/statistic/validation.hpp"
 *
 * using result = castle::statistic::result_t<uint32_t>; // double
 * const bool usable = castle::statistic::is_finite(1.5);
 * const bool valid_p = castle::statistic::is_probability(0.95);
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_VALIDATION_HPP
#define CASTLE_EXT_STATISTIC_VALIDATION_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"

namespace castle
{
namespace statistic
{

/**
 * @brief Floating-point type used for results computed from samples of type `T`.
 * @tparam T Arithmetic sample type.
 * @note `float`, `double`, and `long double` map to themselves; integral types map to `double`.
 */
template <typename T>
using result_t = meta::conditional_t<meta::is_floating_point<T>::value, T, double>;

/**
 * @brief Reports whether a floating-point value is finite (neither NaN nor infinite).
 * @tparam T Floating-point type.
 * @param value Value to test.
 * @return `true` when `lowest() <= value <= max()`; NaN fails both comparisons.
 * @note Complexity: O(1). Does not depend on `<math.h>`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, bool>::type
is_finite(T value) CASTLE_NOEXCEPT
{
    return (value >= numeric_limits<T>::lowest()) && (value <= numeric_limits<T>::max());
}

/**
 * @brief Integral overload: every integral value is finite.
 * @tparam T Integral type.
 * @return Always `true`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value, bool>::type
is_finite(T) CASTLE_NOEXCEPT
{
    return true;
}

/**
 * @brief Reports whether a value is a probability in the closed interval [0, 1].
 * @tparam T Floating-point type.
 * @param value Value to test.
 * @return `true` when `0 <= value <= 1`; NaN returns `false`.
 * @note Complexity: O(1).
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, bool>::type
is_probability(T value) CASTLE_NOEXCEPT
{
    return (value >= static_cast<T>(0)) && (value <= static_cast<T>(1));
}

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_VALIDATION_HPP

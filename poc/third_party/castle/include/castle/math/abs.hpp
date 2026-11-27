// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file abs.hpp
 * @brief Deterministic absolute-value helpers for arithmetic types.
 * @details Provides same-type absolute value via `abs()` and full-range
 * unsigned magnitude via `uabs()` without depending on the STL. Signed
 * integral `abs()` detects the unrepresentable minimum value and reports it
 * through Castle's error handler instead of wrapping. Floating-point `abs()`
 * uses a sign comparison and negation; on IEEE-754 targets, `-0.0` compares
 * equal to zero and is therefore returned unchanged. Use `uabs()` when the
 * magnitude of the full signed range, including `min()`, must be representable.
 *
 * @code
 * #include "castle/math/abs.hpp"
 *
 * constexpr int signed_magnitude = castle::math::abs(-7);
 * constexpr unsigned int full_range =
 *     castle::math::uabs(castle::numeric_limits<int>::min());
 * @endcode
 */

#ifndef CASTLE_MATH_ABS_HPP
#define CASTLE_MATH_ABS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"

namespace castle
{
namespace math
{

namespace detail
{

/**
 * @brief Reports an absolute-value overflow for the signed minimum.
 * @tparam T Signed integral type being processed.
 * @return `T(0)` after routing the failure through Castle's error handler.
 * @note This helper is intentionally not `constexpr` so `abs(min())` is
 * rejected during constant evaluation instead of silently wrapping.
 * @warning The return value is only a fallback for control-flow completion;
 * callers must treat this path as an error.
 */
template <typename T>
CASTLE_INLINE T signed_min_error() CASTLE_NOEXCEPT
{
    CASTLE_ASSERT_FAIL(CASTLE_ERROR_GENERIC("castle::math::abs: |min()| is not representable in the signed type")); // LCOV_EXCL_LINE
    return T(0);
}

}

/**
 * @brief Returns the absolute value of a signed integral input.
 * @tparam T Signed integral type.
 * @param value Value whose magnitude is requested.
 * @return `|value|` in the same signed type.
 * @note For every representable value except `numeric_limits<T>::min()`, the
 * result is returned without changing the type.
 * @warning `numeric_limits<T>::min()` has no positive counterpart in the same
 * signed type; this path triggers Castle's error handler and is rejected in
 * constant evaluation.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_signed<T>::value && meta::is_integral<T>::value, T>::type
abs(T value) CASTLE_NOEXCEPT
{
    // LCOV_EXCL_BR_START
    return (value == castle::numeric_limits<T>::min())
           ? detail::signed_min_error<T>()
           : static_cast<T>((value < static_cast<T>(0)) ? -value : value);
    // LCOV_EXCL_BR_END
}

/**
 * @brief Returns the absolute value of a floating-point input.
 * @tparam T Floating-point type.
 * @param value Value whose magnitude is requested.
 * @return `-value` when `value < 0`, otherwise `value`.
 * @note This overload is `constexpr` and does not depend on `<math.h>`.
 * @warning Negative zero is returned unchanged because `-0.0 < 0.0` is false.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
abs(T value) CASTLE_NOEXCEPT
{
    return (value < static_cast<T>(0)) ? static_cast<T>(-value) : value; // LCOV_EXCL_BR_LINE
}

/**
 * @brief Returns an unsigned integral input unchanged.
 * @tparam T Unsigned integral type.
 * @param value Value whose magnitude is requested.
 * @return `value`.
 * @note Unsigned magnitudes are already non-negative, so this overload is an
 * identity operation.
 * @warning This overload does not perform any range conversion; the return
 * type remains `T`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_unsigned<T>::value, T>::type
abs(T value) CASTLE_NOEXCEPT
{
    return value;
}

/**
 * @brief Returns the magnitude of a signed integral input as an unsigned type.
 * @tparam T Signed integral type.
 * @param value Value whose magnitude is requested.
 * @return `|value|` converted to `meta::make_unsigned<T>::type`.
 * @note This overload is safe for the full signed range, including
 * `numeric_limits<T>::min()`.
 * @warning The return type is the unsigned counterpart of `T`, not `T` itself.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_signed<T>::value && meta::is_integral<T>::value,
                        typename meta::make_unsigned<T>::type>::type
uabs(T value) CASTLE_NOEXCEPT
{
    using UType = typename meta::make_unsigned<T>::type;

    // LCOV_EXCL_BR_START
    // The minimum signed value has magnitude `max / 2 + 1` in the
    // corresponding unsigned type.
    return (value == castle::numeric_limits<T>::min())
           ? static_cast<UType>((castle::numeric_limits<UType>::max() / static_cast<UType>(2)) + static_cast<UType>(1))
           : (value < static_cast<T>(0))
               ? static_cast<UType>(-value)
               : static_cast<UType>(value);
    // LCOV_EXCL_BR_END
}

/**
 * @brief Returns an unsigned integral input unchanged.
 * @tparam T Unsigned integral type.
 * @param value Value whose magnitude is requested.
 * @return `value`.
 * @note This overload mirrors `abs()` for unsigned inputs and preserves the
 * original type.
 * @warning No widening occurs because the input is already unsigned.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_unsigned<T>::value, T>::type
uabs(T value) CASTLE_NOEXCEPT
{
    return value;
}

}
}

#endif

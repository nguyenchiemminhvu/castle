// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file isqrt.hpp
 * @brief Integer floor square root for signed and unsigned integral values.
 * @details Returns the largest integer `r` such that `r * r <= value`. The
 * implementation uses a binary search and compares with `value / middle`
 * rather than `middle * middle`, which avoids multiplication overflow during
 * the search. Signed inputs are accepted only when non-negative.
 *
 * @code
 * #include "castle/math/isqrt.hpp"
 *
 * constexpr unsigned int root16 = castle::math::isqrt(16U);
 * constexpr int root15 = castle::math::isqrt(15);
 * @endcode
 */

#ifndef CASTLE_MATH_ISQRT_HPP
#define CASTLE_MATH_ISQRT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Computes the floor square root of an unsigned integer.
 * @tparam T Unsigned integral type.
 * @param value Value whose floor square root is requested.
 * @return The largest `r` such that `r * r <= value`.
 * @note The binary search runs in `O(log value)` time and returns `value`
 * directly for `0` and `1`.
 * @warning This overload only accepts unsigned types; use the signed overload
 * only for non-negative values.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_unsigned<T>::value, T>::type
isqrt(T value) CASTLE_NOEXCEPT
{
    if (value < static_cast<T>(2))
    {
        return value;
    }

    T low = static_cast<T>(1);
    T high = static_cast<T>(value / static_cast<T>(2) + static_cast<T>(1));

    while (low <= high)
    {
        CASTLE_CONST T middle = static_cast<T>(low + (high - low) / static_cast<T>(2));
        if (middle <= value / middle)
        {
            low = static_cast<T>(middle + static_cast<T>(1));
        }
        else
        {
            high = static_cast<T>(middle - static_cast<T>(1));
        }
    }
    return high;
}

/**
 * @brief Computes the floor square root of a signed integer.
 * @tparam T Signed integral type.
 * @param value Value whose floor square root is requested.
 * @return The largest `r` such that `r * r <= value`.
 * @note Non-negative signed values are forwarded to the unsigned overload
 * after conversion to the corresponding unsigned type.
 * @warning Negative inputs violate the function contract and trigger Castle's
 * error handling when checks are enabled.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_signed<T>::value, T>::type
isqrt(T value) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(value >= T{0}, // LCOV_EXCL_BR_LINE
                  CASTLE_ERROR_GENERIC("castle::math::isqrt: negative input"));
    return isqrt(static_cast<typename meta::make_unsigned<T>::type>(value));
}

}
}

#endif

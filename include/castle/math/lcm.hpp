// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file lcm.hpp
 * @brief Compile-time least-common-multiple calculation.
 * @details Builds the least common multiple from the compile-time
 * greatest-common-divisor helper with the identity
 * `(A / gcd<A, B>::value) * B`. This header is intended for positive
 * `intmax_t` template arguments such as ratio denominators and chrono period
 * computations. No runtime overload is provided in this header.
 *
 * @code
 * #include "castle/math/lcm.hpp"
 *
 * constexpr intmax_t common = castle::math::lcm<12, 15>::value;
 * @endcode
 */

#ifndef CASTLE_MATH_LCM_HPP
#define CASTLE_MATH_LCM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/gcd.hpp"

#include <stdint.h>

namespace castle
{
namespace math
{

/**
 * @brief Computes the least common multiple of `A` and `B` at compile time.
 * @tparam A First positive compile-time operand.
 * @tparam B Second positive compile-time operand.
 * @note The calculation divides before multiplying to reduce intermediate
 * growth relative to `A * B / gcd<A, B>::value`.
 * @warning `A` and `B` must both be strictly positive; zero or negative values
 * fail at compile time, and large results can still overflow `intmax_t`.
 */
template <intmax_t A, intmax_t B>
struct lcm
{
    static_assert(A > 0, "castle::chrono::lcm expects a positive value");
    static_assert(B > 0, "castle::chrono::lcm expects a positive value");

    /**
     * @brief The compile-time least common multiple of `A` and `B`.
     * @return The least common multiple as `intmax_t`.
     * @note Access as `castle::math::lcm<A, B>::value`.
     * @warning The computation is not overflow-checked beyond the positive
     * operand `static_assert`s.
     */
    static CASTLE_CONSTEXPR intmax_t value =
        (A / gcd<A, B>::value) * B;
};

template <intmax_t A, intmax_t B>
CASTLE_CONSTEXPR intmax_t lcm<A, B>::value;

}
}

#endif

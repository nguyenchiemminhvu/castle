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

template <intmax_t A, intmax_t B>
struct lcm
{
private:
    static CASTLE_CONSTEXPR intmax_t divisor = gcd<A, B>::value;

public:
    static CASTLE_CONSTEXPR intmax_t value =
        (A == 0 || B == 0)
        ? 0
        : ((A / divisor) * B < 0
            ? -((A / divisor) * B)
            : ((A / divisor) * B));
};

template <intmax_t A, intmax_t B>
CASTLE_CONSTEXPR intmax_t lcm<A, B>::value;

} // namespace math
} // namespace castle

#endif

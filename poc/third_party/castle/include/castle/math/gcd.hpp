// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file gcd.hpp
 * @brief Compile-time greatest-common-divisor calculation.
 * @details Implements Euclid's algorithm as a template metafunction over
 * `intmax_t` non-type parameters. The implementation is primarily intended for
 * normalized, non-negative compile-time ratios and period computations. No
 * runtime overload is provided in this header.
 *
 * @code
 * #include "castle/math/gcd.hpp"
 *
 * constexpr intmax_t divisor = castle::math::gcd<84, 30>::value;
 * @endcode
 */

#ifndef CASTLE_MATH_GCD_HPP
#define CASTLE_MATH_GCD_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/abs.hpp"

#include <stdint.h>

namespace castle
{
namespace math
{

namespace detail
{

constexpr uintmax_t gcd_value(uintmax_t lhs, uintmax_t rhs) noexcept
{
    return rhs == 0U ? lhs : gcd_value(rhs, lhs % rhs);
}

}

/**
 * @brief Computes the greatest common divisor of `A` and `B` at compile time.
 * @tparam A First compile-time operand.
 * @tparam B Second compile-time operand.
 * @note The recursion follows Euclid's algorithm and terminates when the
 * second operand becomes zero.
 * @warning Operands are not normalized to non-negative values in this header;
 * negative template arguments can therefore produce a negative result.
 */
template <intmax_t A, intmax_t B>
struct gcd
{
    /**
     * @brief The compile-time greatest common divisor of `A` and `B`.
     * @return The Euclidean remainder-chain result as `intmax_t`.
     * @note Access as `castle::math::gcd<A, B>::value`.
     * @warning Intended for non-negative operands when a canonical positive GCD
     * is required.
     */
    static CASTLE_CONSTEXPR intmax_t value = static_cast<intmax_t>(
        detail::gcd_value(uabs(A), uabs(B))
    );
};

template <intmax_t A, intmax_t B>
CASTLE_CONSTEXPR intmax_t gcd<A, B>::value;

}
}

#endif


////

// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file gcd.hpp
 * @brief Compile-time greatest-common-divisor utility for integral constants.
 *
 * This small math primitive is used by ratio and duration arithmetic. It is
 * entirely compile-time and introduces no runtime state or allocation.
 */
#ifndef CASTLE_MATH_GCD_HPP
#define CASTLE_MATH_GCD_HPP

#include <stdint.h>

namespace castle
{
namespace math
{


template <intmax_t A, intmax_t B>
struct gcd
{
    
};



} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GCD_HPP

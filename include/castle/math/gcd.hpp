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

template <intmax_t A, intmax_t B>
struct gcd
{
    static CASTLE_CONSTEXPR intmax_t value = gcd<B, A % B>::value;
};

template <intmax_t A>
struct gcd<A, 0>
{
    static CASTLE_CONSTEXPR intmax_t value = A;
};

template <intmax_t A, intmax_t B>
CASTLE_CONSTEXPR intmax_t gcd<A, B>::value;

template <intmax_t A>
CASTLE_CONSTEXPR intmax_t gcd<A, 0>::value;

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_GCD_HPP

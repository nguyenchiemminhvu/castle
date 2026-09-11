#ifndef CASTLE_MATH_GCD_H
#define CASTLE_MATH_GCD_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/math/abs.h"

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

#endif // CASTLE_MATH_GCD_H

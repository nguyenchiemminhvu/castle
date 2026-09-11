#ifndef CASTLE_MATH_LCM_H
#define CASTLE_MATH_LCM_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/math/gcd.h"

#include <stdint.h>

namespace castle
{
namespace math
{

template <intmax_t A, intmax_t B>
struct lcm
{
    static_assert(A > 0, "castle::chrono::lcm expects a positive value");
    static_assert(B > 0, "castle::chrono::lcm expects a positive value");

    static CASTLE_CONSTEXPR intmax_t value =
        (A / gcd<A, B>::value) * B;
};

template <intmax_t A, intmax_t B>
CASTLE_CONSTEXPR intmax_t lcm<A, B>::value;

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_LCM_H

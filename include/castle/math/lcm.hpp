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
    static_assert(A > 0, "castle::chrono::lcm expects a positive value");
    static_assert(B > 0, "castle::chrono::lcm expects a positive value");

    static CASTLE_CONSTEXPR intmax_t value =
        (A / gcd<A, B>::value) * B;
};

template <intmax_t A, intmax_t B>
CASTLE_CONSTEXPR intmax_t lcm<A, B>::value;

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_LCM_HPP

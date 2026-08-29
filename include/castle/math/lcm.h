#pragma once

#include "castle/types/traits.h"
#include "castle/math/gcd.h"

using namespace castle::types;

namespace castle
{
namespace math
{

/// Compile-time LCM via template
template <intmax_t A, intmax_t B>
struct lcm_v
{
    static constexpr intmax_t value = (A / gcd_v<A, B>::value) * B;
};

/// Runtime LCM for any integral type
template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
lcm(T a, T b) noexcept
{
    // Handle signs for signed types
    if constexpr (std::is_signed<T>::value)
    {
        if (a < 0) a = -a;
        if (b < 0) b = -b;
    }

    if (a == T{0} || b == T{0})
    {
        return T{0};
    }

    return (a / gcd(a, b)) * b;
}

} // namespace math
} // namespace castle
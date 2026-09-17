#pragma once

#include "castle/types/traits.h"

#include <cstdint>

using namespace castle::types;

namespace castle
{
namespace math
{

/// Compile-time GCD via template recursion
template <intmax_t A, intmax_t B>
struct gcd_v : gcd_v<B, A % B> {};

template <intmax_t A>
struct gcd_v<A, 0>
{
    static constexpr intmax_t value = (A < 0) ? -A : A;
};

/// Runtime GCD for any integral type
template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
gcd(T a, T b) noexcept
{
    // Handle signs for signed types
    if constexpr (std::is_signed<T>::value)
    {
        if (a < 0) a = -a;
        if (b < 0) b = -b;
    }

    while (b != T{0})
    {
        T r = a % b;
        a = b;
        b = r;
    }
    return a;
}

/// Variadic GCD: compute GCD of multiple values
template <typename T, typename... Args>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
gcd(T a, T b, Args... args) noexcept
{
    T g = gcd(a, b);
    if (g == T{1})
    {
        return T{1};
    }
    return gcd(g, args...);
}

} // namespace math
} // namespace castle


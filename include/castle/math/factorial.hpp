#ifndef CASTLE_MATH_FACTORIAL_HPP
#define CASTLE_MATH_FACTORIAL_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

namespace castle
{
namespace math
{

template <size_type N>
struct factorial
{
    static CASTLE_CONSTEXPR size_type value = N * factorial<N - 1>::value;
};

template <>
struct factorial<0>
{
    static CASTLE_CONSTEXPR size_type value = 1;
};

CASTLE_CONSTEXPR size_type factorial_v(size_type n)
{
    size_type result = 1;
    for (size_type i = 2; i <= n; ++i)
    {
        result *= i;
    }
    return result;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_FACTORIAL_HPP
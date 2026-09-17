#ifndef CASTLE_MATH_FIB_H
#define CASTLE_MATH_FIB_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"

namespace castle
{
namespace math
{

template <size_type N>
CASTLE_CONSTEXPR size_type fib() CASTLE_NOEXCEPT
{
    CASTLE_IF_CONSTEXPR (N == 0)
    {
        return 0;
    }
    else CASTLE_IF_CONSTEXPR (N == 1)
    {
        return 1;
    }

    size_type a{0};
    size_type b{1};
    for (size_type i = 2; i <= N; ++i)
    {
        size_type next = a + b;
        a = b;
        b = next;
    }
    return b;
}

CASTLE_CONSTEXPR size_type fib(size_type n) CASTLE_NOEXCEPT
{
    if (n == 0)
    {
        return 0;
    }
    
    if (n == 1)
    {
        return 1;
    }

    size_type a{0};
    size_type b{1};
    for (size_type i = 2; i <= n; ++i)
    {
        size_type next = a + b;
        a = b;
        b = next;
    }
    return b;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_FIB_H

// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file fib.hpp
 * @brief Compile-time and runtime Fibonacci helpers for `size_type`.
 * @details Provides a template form for fixed sequence indices and an
 * iterative runtime form for variable indices. Both implementations are
 * iterative after the base cases, so they avoid recursion at runtime while
 * remaining `constexpr`. Results are stored in `castle::size_type`; once the
 * sequence exceeds that range, arithmetic follows the platform's native
 * `size_type` behavior.
 *
 * @code
 * #include "castle/math/fib.hpp"
 *
 * constexpr castle::size_type f10 = castle::math::fib<10>();
 * constexpr castle::size_type dynamic = castle::math::fib(7);
 * @endcode
 */

#ifndef CASTLE_MATH_FIB_HPP
#define CASTLE_MATH_FIB_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Computes the `N`th Fibonacci number at compile time.
 * @tparam N Zero-based Fibonacci index.
 * @return The `N`th Fibonacci number as `castle::size_type`.
 * @note `fib<0>()` returns `0` and `fib<1>()` returns `1`.
 * @warning No overflow detection is performed; sufficiently large indices wrap
 * according to `size_type` arithmetic.
 */
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

/**
 * @brief Computes the `n`th Fibonacci number for a runtime index.
 * @param n Zero-based Fibonacci index.
 * @return The `n`th Fibonacci number as `castle::size_type`.
 * @note The loop executes `O(n)` additions and handles `0` and `1` as direct
 * base cases.
 * @warning No overflow detection is performed; sufficiently large indices wrap
 * according to `size_type` arithmetic.
 */
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

}
}

#endif

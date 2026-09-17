// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file factorial.hpp
 * @brief Compile-time and runtime factorial helpers for `size_type`.
 * @details Exposes a template metafunction for constant expressions and a
 * small iterative `constexpr` runtime form for variable inputs. Both forms use
 * `castle::size_type`, so arithmetic follows the behavior of the platform's
 * `size_t`. No overflow checks are performed; once the factorial exceeds the
 * representable range, the result follows the underlying unsigned arithmetic
 * rules of `size_type`.
 *
 * @code
 * #include "castle/math/factorial.hpp"
 *
 * constexpr castle::size_type table_size = castle::math::factorial<5>::value;
 * constexpr castle::size_type permutations = castle::math::factorial_v(4);
 * @endcode
 */

#ifndef CASTLE_MATH_FACTORIAL_HPP
#define CASTLE_MATH_FACTORIAL_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Computes `N!` at compile time.
 * @tparam N Non-negative factorial argument expressed as `castle::size_type`.
 * @note Instantiation depth grows linearly with `N`.
 * @warning The stored result is not range-checked; large `N` values wrap
 * according to `size_type` arithmetic.
 */
template <size_type N>
struct factorial
{
    /**
     * @brief The compile-time value of `N!`.
     * @return This member stores the computed factorial as `size_type`.
     * @note Access as `castle::math::factorial<N>::value`.
     * @warning No overflow detection is performed.
     */
    static CASTLE_CONSTEXPR size_type value = N * factorial<N - 1>::value;
};

/**
 * @brief Base case for `0!`.
 * @note `0!` is defined as `1`.
 * @warning This specialization only defines the compile-time constant; it does
 * not change overflow behavior for larger instantiations.
 */
template <>
struct factorial<0>
{
    /**
     * @brief The compile-time value of `0!`.
     * @return Always `1`.
     * @note This terminates the recursive template definition.
     * @warning The type remains `castle::size_type`.
     */
    static CASTLE_CONSTEXPR size_type value = 1;
};

/**
 * @brief Computes `n!` with an iterative `constexpr` loop.
 * @param n Non-negative factorial argument expressed as `castle::size_type`.
 * @return The factorial of `n` as `castle::size_type`.
 * @note The loop performs `O(n)` multiplications and returns `1` for `n == 0`
 * and `n == 1`.
 * @warning No overflow detection is performed; large results wrap according to
 * `size_type` arithmetic.
 */
CASTLE_CONSTEXPR size_type factorial_v(size_type n)
{
    size_type result = 1;
    for (size_type i = 2; i <= n; ++i)
    {
        result *= i;
    }
    return result;
}

}
}

#endif

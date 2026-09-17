// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sqrt.hpp
 * @brief Compile-time integer floor square root.
 *
 * This header computes the largest integer `r` such that `r * r <= Value`
 * using a deterministic binary search that can be evaluated at compile time.
 * Use it for buffer sizing, grid dimensions, and other integer-only sizing
 * decisions where floating-point math is unnecessary.
 *
 * Example:
 * @code
 * static_assert(castle::sqrt<144U>::value == 12U, "exact square");
 * static_assert(castle::sqrt<145U>::value == 12U, "floor square root");
 * @endcode
 *
 * @note The `sqrt` template lives in namespace `castle`, not `castle::math`.
 */
#ifndef CASTLE_MATH_SQRT_HPP
#define CASTLE_MATH_SQRT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

namespace castle
{

namespace detail
{

/**
 * @brief Helper function for computing the floor square root using binary search.
 * @param value The value to compute the floor square root for.
 * @param low The lower bound for the binary search.
 * @param high The upper bound for the binary search.
 * @return The floor square root of `value`.
 */
template <typename T>
CASTLE_CONSTEXPR T sqrt_floor(T value, T low, T high) CASTLE_NOEXCEPT
{
    T result = T{0};

    while (low <= high)
    {
        CASTLE_CONST T mid = low + (high - low) / T{2};

        // Compare through division so the test never forms mid * mid.
        if (mid == T{0} || mid <= value / mid)
        {
            result = mid;
            low = mid + T{1};
        }
        else
        {
            high = mid - T{1};
        }
    }

    return result;
}

} // namespace detail

/**
 * @brief Computes the floor square root of an integer constant.
 * @tparam Value Input constant.
 * @tparam Root Initial lower bound for the internal binary search.
 * @note The result is available both as `value` and as `type`.
 * @warning `Root` is an internal search hint and should not exceed the desired
 *          floor square root.
 */
template <size_type Value, size_type Root = 1U>
struct sqrt
{
private:
    static CASTLE_CONSTEXPR size_type calculate() CASTLE_NOEXCEPT
    {
        return detail::sqrt_floor<size_type>(
            Value,
            Root,
            Value
        );
    }

public:
    /**
     * @brief Floor square root result.
     */
    static CASTLE_CONSTEXPR size_type value = calculate();

    /**
     * @brief Value type used for the result.
     */
    using value_type = size_type;

    /**
     * @brief Integral-constant wrapper around `value`.
     */
    using type = meta::integral_constant<size_type, value>;
};

} // namespace castle

#endif // CASTLE_MATH_SQRT_HPP

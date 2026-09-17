#ifndef CASTLE_MATH_SQRT_HPP
#define CASTLE_MATH_SQRT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

namespace castle
{

// ============================================================================
// Compile-time integer square root.
//
// Calculates the largest integer Root such that:
//
//     Root * Root <= Value
//
//     castle::sqrt<Value>::value
//     castle::sqrt<Value>::type
//
// Example:
//
//     static_assert(castle::sqrt<0U>::value == 0U, "");
//     static_assert(castle::sqrt<144U>::value == 12U, "");
//     static_assert(castle::sqrt<145U>::value == 12U, "");
// ============================================================================

namespace detail
{

template <typename T>
CASTLE_CONSTEXPR T sqrt_floor(T value, T low, T high) CASTLE_NOEXCEPT
{
    T result = T{0};

    while (low <= high)
    {
        CASTLE_CONST T mid = low + (high - low) / T{2};

        // Avoid mid * mid overflow.
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


// --------------------------------------------------------------------------
// Compile-time square root.
// --------------------------------------------------------------------------

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
    static CASTLE_CONSTEXPR size_type value = calculate();

    using value_type = size_type;
    using type = meta::integral_constant<size_type, value>;
};

} // namespace castle

#endif // CASTLE_MATH_SQRT_HPP

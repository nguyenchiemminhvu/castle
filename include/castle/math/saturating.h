#ifndef CASTLE_MATH_SATURATING_H
#define CASTLE_MATH_SATURATING_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/type_ranges.h"

namespace castle
{
namespace math
{

// ============================================================================
// Saturating integer arithmetic.
//
// Saturation is often preferable to wrap-around for physical quantities where
// an overflow must not turn a large positive command into a large negative one.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value && meta::is_unsigned<T>::value, T>::type
saturating_add(T a, T b) CASTLE_NOEXCEPT
{
    const T maximum = castle::numeric_limits<T>::max();
    return (b > static_cast<T>(maximum - a)) ? maximum : static_cast<T>(a + b);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value && meta::is_unsigned<T>::value, T>::type
saturating_sub(T a, T b) CASTLE_NOEXCEPT
{
    return (b > a) ? T{0} : static_cast<T>(a - b);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value && meta::is_signed<T>::value, T>::type
saturating_add(T a, T b) CASTLE_NOEXCEPT
{
    const T maximum = castle::numeric_limits<T>::max();
    const T minimum = castle::numeric_limits<T>::min();

    if (b > T{0} && a > static_cast<T>(maximum - b))
    {
        return maximum;
    }
    if (b < T{0} && a < static_cast<T>(minimum - b))
    {
        return minimum;
    }
    return static_cast<T>(a + b);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value && meta::is_signed<T>::value, T>::type
saturating_sub(T a, T b) CASTLE_NOEXCEPT
{
    const T maximum = castle::numeric_limits<T>::max();
    const T minimum = castle::numeric_limits<T>::min();

    if (b > T{0} && a < static_cast<T>(minimum + b))
    {
        return minimum;
    }
    if (b < T{0} && a > static_cast<T>(maximum + b))
    {
        return maximum;
    }
    return static_cast<T>(a - b);
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_SATURATING_H

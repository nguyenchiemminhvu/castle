#ifndef CASTLE_MATH_ANGLE_H
#define CASTLE_MATH_ANGLE_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ============================================================================
// Angle constants and unit conversions.
//
// Pi is kept as a function template so the conversion stays in the caller's
// scalar type and can be evaluated at compile time.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
pi() CASTLE_NOEXCEPT
{
    return static_cast<T>(3.141592653589793238462643383279502884L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
tau() CASTLE_NOEXCEPT
{
    return static_cast<T>(6.283185307179586476925286766559005768L);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
degrees_to_radians(T degrees) CASTLE_NOEXCEPT
{
    return degrees * pi<T>() / static_cast<T>(180);
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
radians_to_degrees(T radians) CASTLE_NOEXCEPT
{
    return radians * static_cast<T>(180) / pi<T>();
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_ANGLE_H

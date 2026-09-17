#ifndef CASTLE_MATH_TRIGONOMETRY_H
#define CASTLE_MATH_TRIGONOMETRY_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

#include <math.h>

namespace castle
{
namespace math
{

// ============================================================================
// Thin floating-point wrappers around the C math library.
//
// Keeping the dependency in one header makes trigonometric use explicit and
// gives embedded ports a single replacement point when a platform-specific,
// deterministic implementation is preferred.
// ============================================================================
inline float sin(float radians) CASTLE_NOEXCEPT
{
    return ::sinf(radians);
}

inline double sin(double radians) CASTLE_NOEXCEPT
{
    return ::sin(radians);
}

inline long double sin(long double radians) CASTLE_NOEXCEPT
{
    return ::sinl(radians);
}

inline float cos(float radians) CASTLE_NOEXCEPT
{
    return ::cosf(radians);
}

inline double cos(double radians) CASTLE_NOEXCEPT
{
    return ::cos(radians);
}

inline long double cos(long double radians) CASTLE_NOEXCEPT
{
    return ::cosl(radians);
}

template <typename T>
CASTLE_NODISCARD T radians_sin(T radians) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "radians_sin requires a floating-point type");
    return castle::math::sin(radians);
}

template <typename T>
CASTLE_NODISCARD T radians_cos(T radians) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "radians_cos requires a floating-point type");
    return castle::math::cos(radians);
}

template <typename T>
struct sin_cos_result
{
    T sine;
    T cosine;
};

template <typename T>
CASTLE_NODISCARD sin_cos_result<T> sin_cos(T radians) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<T>::value,
                  "sin_cos requires a floating-point type");
    return sin_cos_result<T>{castle::math::sin(radians), castle::math::cos(radians)};
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_TRIGONOMETRY_H

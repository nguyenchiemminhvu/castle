#ifndef CASTLE_MATH_MATH_H
#define CASTLE_MATH_MATH_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/type_ranges.h"

#include "castle/math/fib.h"
#include "castle/math/gcd.h"
#include "castle/math/lcm.h"
#include "castle/math/abs.h"
#include "castle/math/sqrt.h"
#include "castle/math/clamp.h"
#include "castle/math/mean.h"
#include "castle/math/ratio.h"
#include "castle/math/invert.h"
#include "castle/math/logarithm.h"
#include "castle/math/random.h"
#include "castle/math/angle.h"
#include "castle/math/ceil_div.h"
#include "castle/math/floor_div.h"
#include "castle/math/hypot.h"
#include "castle/math/is_power_of_two.h"
#include "castle/math/isqrt.h"
#include "castle/math/lerp.h"
#include "castle/math/minmax.h"
#include "castle/math/mod.h"
#include "castle/math/near_equal.h"
#include "castle/math/powi.h"
#include "castle/math/saturating.h"
#include "castle/math/sign.h"
#include "castle/math/sqrt_real.h"
#include "castle/math/square.h"
#include "castle/math/geometry.h"

#include <math.h>

namespace castle
{
namespace math
{

template <typename T>
CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_floating_point<T>::value, bool>
is_equal(T a, T b) CASTLE_NOEXCEPT
{
    return (a == b) || (castle::math::abs(a - b) <= castle::floating_epsilon<T>::value);
}

template <typename T>
CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_floating_point<T>::value, bool>
is_zero(T a) CASTLE_NOEXCEPT
{
    return castle::math::abs(a) <= castle::floating_epsilon<T>::value;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_MATH_H
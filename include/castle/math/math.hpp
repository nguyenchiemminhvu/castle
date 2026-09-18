#ifndef CASTLE_MATH_MATH_HPP
#define CASTLE_MATH_MATH_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"

#include "castle/math/fib.hpp"
#include "castle/math/gcd.hpp"
#include "castle/math/lcm.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/sqrt.hpp"
#include "castle/math/clamp.hpp"
#include "castle/math/mean.hpp"
#include "castle/math/ratio.hpp"
#include "castle/math/invert.hpp"
#include "castle/math/logarithm.hpp"
#include "castle/math/random.hpp"
#include "castle/math/angle.hpp"
#include "castle/math/ceil_div.hpp"
#include "castle/math/floor_div.hpp"
#include "castle/math/hypot.hpp"
#include "castle/math/factorial.hpp"
#include "castle/math/is_power_of_two.hpp"
#include "castle/math/isqrt.hpp"
#include "castle/math/lerp.hpp"
#include "castle/math/mod.hpp"
#include "castle/math/near_equal.hpp"
#include "castle/math/powi.hpp"
#include "castle/math/saturating.hpp"
#include "castle/math/sign.hpp"
#include "castle/math/sqrt_real.hpp"
#include "castle/math/square.hpp"
#include "castle/math/geometry.hpp"
#include "castle/math/linalg.hpp"

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
    return (a == b) || (castle::math::abs(a - b) <= castle::floating_epsilon<T>::value); // LCOV_EXCL_BR_LINE
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

#endif // CASTLE_MATH_MATH_HPP
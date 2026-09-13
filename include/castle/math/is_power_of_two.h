#ifndef CASTLE_MATH_IS_POWER_OF_TWO_H
#define CASTLE_MATH_IS_POWER_OF_TWO_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ----------------------------------------------------------------------------
// True when value is a non-zero power of two.
//
// Real-life use cases:
//   - Validating ring-buffer sizes for mask-based indexing.
//   - Checking memory block/page sizes that permit shift-based arithmetic.
// ----------------------------------------------------------------------------
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value, bool>::type
is_power_of_two(T value) CASTLE_NOEXCEPT
{
    return value != T{0} && (value & static_cast<T>(value - T{1})) == T{0};
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_IS_POWER_OF_TWO_H

#ifndef CASTLE_MATH_SQUARE_H
#define CASTLE_MATH_SQUARE_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

namespace castle
{
namespace math
{

// ----------------------------------------------------------------------------
// Square helper. The operation intentionally does not widen the result so the
// caller can see the exact arithmetic type and its overflow requirements.
// ----------------------------------------------------------------------------
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
T square(T value) CASTLE_NOEXCEPT
{
    return value * value;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_SQUARE_H

#ifndef CASTLE_MATH_CLAMP_HPP
#define CASTLE_MATH_CLAMP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"

namespace castle
{
namespace math
{

// ----------------------------------------------------------------------------
// Clamp a value to an inclusive [low, high] range.
//
// The conditional expression is intentionally used instead of an if statement
// so this remains valid as a C++11 constexpr function as well as in later
// language modes.
// ----------------------------------------------------------------------------
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_arithmetic<T>::value, T>::type
clamp(T value, T low, T high) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(low <= high, "clamp precondition violated: low > hi.hpp");

    return (value < low)
           ? low
           : ((high < value) ? high : value);
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_CLAMP_HPP

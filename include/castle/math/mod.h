#ifndef CASTLE_MATH_MOD_H
#define CASTLE_MATH_MOD_H

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"

namespace castle
{
namespace math
{

// ============================================================================
// Positive modulo for integers. Result is always in [0, modulus).
// The modulus must be positive.
//
// Real-life use cases:
//   - Wrapping encoder/timer counters.
//   - Normalizing table indexes and phase angles represented as integers.
// ============================================================================
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value, T>::type
positive_mod(T value, T modulus) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(modulus > T{0},
                  CASTLE_ERROR_GENERIC("castle::math::positive_mod: modulus must be positive"));

    CASTLE_CONST T result = static_cast<T>(value % modulus);
    return (result < T{0}) ? static_cast<T>(result + modulus) : result;
}

template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_integral<T>::value, T>::type
wrap(T value, T low, T high) CASTLE_NOEXCEPT
{
    CASTLE_ASSERT(high > low,
                  CASTLE_ERROR_GENERIC("castle::math::wrap: high must be greater than low"));

    // For common embedded integer widths, widen before subtracting so a
    // value near the ends of the type does not overflow the intermediate.
    using calc_type = typename meta::conditional<
        meta::is_unsigned<T>::value && (sizeof(T) <= sizeof(uint32_t)),
        uint64_t,
        typename meta::conditional<
            meta::is_signed<T>::value && (sizeof(T) <= sizeof(int32_t)),
            int64_t,
            T>::type>::type;

    CASTLE_CONST calc_type shifted = static_cast<calc_type>(value) - static_cast<calc_type>(low);
    CASTLE_CONST calc_type period = static_cast<calc_type>(high) - static_cast<calc_type>(low);
    CASTLE_CONST calc_type wrapped = static_cast<calc_type>(positive_mod(shifted, period));
    return static_cast<T>(static_cast<calc_type>(low) + wrapped);
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_MOD_H

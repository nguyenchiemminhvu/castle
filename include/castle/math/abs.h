#ifndef CASTLE_MATH_ABS_H
#define CASTLE_MATH_ABS_H

// ============================================================================
// castle/math/abs.h
// ----------------------------------------------------------------------------
// Deterministic absolute-value helpers for CASTLE.
//
//   abs(T)   Same-type absolute value.
//                * signed integrals  - traps on T == min() through the CASTLE
//                                      error handler because -min() is not
//                                      representable in T.
//                * unsigned types    - returns value unchanged.
//                * floating point    - branchless magnitude via negation.
//   uabs(T)  Absolute value returned as the unsigned counterpart of T. This
//            is defined for the full signed range (including min()) because
//            the unsigned type has the extra bit needed to represent it.
//
// All variants are SFINAE-selected on castle traits, are CASTLE_NOEXCEPT, and
// carry CASTLE_NODISCARD to catch accidental discards at call sites. There is
// no dependency on <math.h> or std::numeric_limits.
// ============================================================================

#include "castle/core/compiler.h"
#include "castle/core/error_handler.h"
#include "castle/core/traits.h"
#include "castle/core/type_ranges.h"

namespace castle
{
namespace math
{

namespace detail
{

// Non-constexpr trap for the un-representable signed minimum. Kept
// non-constexpr on purpose: if abs() is used in a constant-evaluation
// context with T == min(), the compiler must reject the program instead
// of silently wrapping. At run time this routes through the CASTLE
// error handler so the target can log / halt as it sees fit.
template <typename T>
CASTLE_INLINE T signed_min_error() CASTLE_NOEXCEPT
{
    CASTLE_ASSERT_FAIL(
        CASTLE_ERROR_GENERIC(
            "castle::math::abs: |min()| is not representable in the signed type"
        )
    );
    return T(0);
}

} // namespace detail

// ----------------------------------------------------------------------------
// 1. Signed integrals
// ----------------------------------------------------------------------------
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_signed<T>::value && meta::is_integral<T>::value, T>::type
abs(T value) CASTLE_NOEXCEPT
{
    return (value == castle::numeric_limits<T>::min())
           ? detail::signed_min_error<T>()
           : static_cast<T>((value < static_cast<T>(0)) ? -value : value);
}

// ----------------------------------------------------------------------------
// 2. Floating-point
// ----------------------------------------------------------------------------
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_floating_point<T>::value, T>::type
abs(T value) CASTLE_NOEXCEPT
{
    return (value < static_cast<T>(0)) ? static_cast<T>(-value) : value;
}

// ----------------------------------------------------------------------------
// 3. Unsigned integrals - identity
// ----------------------------------------------------------------------------
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_unsigned<T>::value, T>::type
abs(T value) CASTLE_NOEXCEPT
{
    return value;
}

// ----------------------------------------------------------------------------
// 4. Signed -> unsigned magnitude (safe for the entire signed range)
// ----------------------------------------------------------------------------
template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_signed<T>::value && meta::is_integral<T>::value,
                        typename meta::make_unsigned<T>::type>::type
uabs(T value) CASTLE_NOEXCEPT
{
    using UType = typename meta::make_unsigned<T>::type;

    // |INT_MIN| == INT_MAX + 1 == (UINT_MAX / 2) + 1
    return (value == castle::numeric_limits<T>::min())
           ? static_cast<UType>((castle::numeric_limits<UType>::max() / static_cast<UType>(2)) + static_cast<UType>(1))
           : (value < static_cast<T>(0))
               ? static_cast<UType>(-value)
               : static_cast<UType>(value);
}

template <typename T>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
typename meta::enable_if<meta::is_unsigned<T>::value, T>::type
uabs(T value) CASTLE_NOEXCEPT
{
    return value;
}

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_ABS_H
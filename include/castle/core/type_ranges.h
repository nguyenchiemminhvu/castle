#ifndef CASTLE_CORE_TYPE_RANGES_H
#define CASTLE_CORE_TYPE_RANGES_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"

#include <limits.h>
#include <float.h>
#include <stdint.h>

#ifndef CHAR_MIN
#define CHAR_MIN SCHAR_MIN
#endif

#ifndef CHAR_MAX
#define CHAR_MAX SCHAR_MAX
#endif

#ifndef SCHAR_MIN
#define SCHAR_MIN (-127 - 1)
#endif

#ifndef SCHAR_MAX
#define SCHAR_MAX 127
#endif

#ifndef UCHAR_MAX
#define UCHAR_MAX 255U
#endif

#ifndef SHRT_MIN
#define SHRT_MIN (-32767 - 1)
#endif

#ifndef SHRT_MAX
#define SHRT_MAX 32767
#endif

#ifndef USHRT_MAX
#define USHRT_MAX 65535U
#endif

#ifndef INT_MIN
#define INT_MIN (-2147483647 - 1)
#endif

#ifndef INT_MAX
#define INT_MAX 2147483647
#endif

#ifndef UINT_MAX
#define UINT_MAX 4294967295U
#endif

/*
 * ============================================================================
 * long
 * ============================================================================
 *
 * LONG_MIN/LONG_MAX/ULONG_MAX are intentionally NOT hard-coded here.
 *
 * The width of long is ABI-dependent:
 *
 *     LP64:
 *         long = 64-bit
 *
 *     LLP64:
 *         long = 32-bit
 *
 *     ILP32:
 *         long = 32-bit
 *
 * Zephyr's toolchain knows the actual ABI, so <limits.h> should provide the
 * correct LONG_* values.
 *
 * If a particular freestanding toolchain does not provide them, use compiler
 * predefined width information instead.
 * ============================================================================
 */

#ifndef LONG_MIN

#if defined(__SIZEOF_LONG__) && (__SIZEOF_LONG__ == 8)
#define LONG_MIN (-9223372036854775807L - 1L)
#elif defined(__SIZEOF_LONG__) && (__SIZEOF_LONG__ == 4)
#define LONG_MIN (-2147483647L - 1L)
#endif

#endif

#ifndef LONG_MAX

#if defined(__SIZEOF_LONG__) && (__SIZEOF_LONG__ == 8)
#define LONG_MAX 9223372036854775807L
#elif defined(__SIZEOF_LONG__) && (__SIZEOF_LONG__ == 4)
#define LONG_MAX 2147483647L
#endif

#endif

#ifndef ULONG_MAX

#if defined(__SIZEOF_LONG__) && (__SIZEOF_LONG__ == 8)
#define ULONG_MAX 18446744073709551615UL
#elif defined(__SIZEOF_LONG__) && (__SIZEOF_LONG__ == 4)
#define ULONG_MAX 4294967295UL
#endif

#endif

/*
 * ============================================================================
 * long long
 * ============================================================================
 *
 * C++11 guarantees long long is at least 64 bits.
 *
 * __SIZEOF_LONG_LONG__ is used only as a sanity/portability mechanism if the
 * standard limits.h macros are absent.
 * ============================================================================
 */

#ifndef LLONG_MIN

#if defined(__SIZEOF_LONG_LONG__) && (__SIZEOF_LONG_LONG__ >= 8)
#define LLONG_MIN (-9223372036854775807LL - 1LL)
#else
#define LLONG_MIN (-9223372036854775807LL - 1LL)
#endif

#endif

#ifndef LLONG_MAX

#if defined(__SIZEOF_LONG_LONG__) && (__SIZEOF_LONG_LONG__ >= 8)
#define LLONG_MAX 9223372036854775807LL
#else
#define LLONG_MAX 9223372036854775807LL
#endif

#endif

#ifndef ULLONG_MAX

#if defined(__SIZEOF_LONG_LONG__) && (__SIZEOF_LONG_LONG__ >= 8)
#define ULLONG_MAX 18446744073709551615ULL
#else
#define ULLONG_MAX 18446744073709551615ULL
#endif

#endif

namespace castle
{

/*
 * ============================================================================
 * Primary template
 * ============================================================================
 */

template <typename T>
struct numeric_limits;

/*
 * ============================================================================
 * Integral specialization generator
 * ============================================================================
 */

#define CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(TYPE, MIN_VAL, MAX_VAL, SIGNED) \
template <> \
struct numeric_limits<TYPE> \
{ \
    static CASTLE_CONSTEXPR bool is_specialized = true; \
    static CASTLE_CONSTEXPR bool is_signed = SIGNED; \
    static CASTLE_CONSTEXPR bool is_integer = true; \
    static CASTLE_CONSTEXPR bool is_exact = true; \
    \
    static CASTLE_CONSTEXPR TYPE min() CASTLE_NOEXCEPT \
    { \
        return static_cast<TYPE>(MIN_VAL); \
    } \
    \
    static CASTLE_CONSTEXPR TYPE max() CASTLE_NOEXCEPT \
    { \
        return static_cast<TYPE>(MAX_VAL); \
    } \
    \
    static CASTLE_CONSTEXPR TYPE lowest() CASTLE_NOEXCEPT \
    { \
        return static_cast<TYPE>(MIN_VAL); \
    } \
};

/*
 * ============================================================================
 * char
 * ============================================================================
 */

CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    char,
    CHAR_MIN,
    CHAR_MAX,
    (CHAR_MIN < 0)
)

/*
 * ============================================================================
 * signed char
 * ============================================================================
 */

CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    signed char,
    SCHAR_MIN,
    SCHAR_MAX,
    true
)

/*
 * ============================================================================
 * unsigned char
 * ============================================================================
 */

CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    unsigned char,
    0,
    UCHAR_MAX,
    false
)

/*
 * ============================================================================
 * short
 * ============================================================================
 */

CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    short,
    SHRT_MIN,
    SHRT_MAX,
    true
)

/*
 * ============================================================================
 * unsigned short
 * ============================================================================
 */

CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    unsigned short,
    0,
    USHRT_MAX,
    false
)

/*
 * ============================================================================
 * int
 * ============================================================================
 */

CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    int,
    INT_MIN,
    INT_MAX,
    true
)

/*
 * ============================================================================
 * unsigned int
 * ============================================================================
 */

CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    unsigned int,
    0,
    UINT_MAX,
    false
)

/*
 * ============================================================================
 * long
 * ============================================================================
 *
 * These values come from the actual ABI through <limits.h>.
 *
 * For GCC/Clang/Zephyr:
 *
 *     __SIZEOF_LONG__ == 4
 *
 * or:
 *
 *     __SIZEOF_LONG__ == 8
 *
 * is used only when limits.h fails to provide LONG_*.
 * ============================================================================
 */

#ifdef LONG_MIN
#ifdef LONG_MAX

CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    long,
    LONG_MIN,
    LONG_MAX,
    true
)

#endif
#endif

#ifdef ULONG_MAX

CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    unsigned long,
    0,
    ULONG_MAX,
    false
)

#endif

/*
 * ============================================================================
 * long long
 * ============================================================================
 */

CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    long long,
    LLONG_MIN,
    LLONG_MAX,
    true
)

CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    unsigned long long,
    0,
    ULLONG_MAX,
    false
)

#undef CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC

/*
 * ============================================================================
 * bool
 * ============================================================================
 */

template <>
struct numeric_limits<bool>
{
    static CASTLE_CONSTEXPR bool is_specialized = true;
    static CASTLE_CONSTEXPR bool is_signed = false;
    static CASTLE_CONSTEXPR bool is_integer = true;
    static CASTLE_CONSTEXPR bool is_exact = true;

    static CASTLE_CONSTEXPR bool min() CASTLE_NOEXCEPT
    {
        return false;
    }

    static CASTLE_CONSTEXPR bool max() CASTLE_NOEXCEPT
    {
        return true;
    }

    static CASTLE_CONSTEXPR bool lowest() CASTLE_NOEXCEPT
    {
        return false;
    }
};

/*
 * ============================================================================
 * Floating-point specializations
 * ============================================================================
 */

template <>
struct numeric_limits<float>
{
    static CASTLE_CONSTEXPR bool is_specialized = true;
    static CASTLE_CONSTEXPR bool is_signed = true;
    static CASTLE_CONSTEXPR bool is_integer = false;
    static CASTLE_CONSTEXPR bool is_exact = false;

    static CASTLE_CONSTEXPR float min() CASTLE_NOEXCEPT
    {
        return FLT_MIN;
    }

    static CASTLE_CONSTEXPR float max() CASTLE_NOEXCEPT
    {
        return FLT_MAX;
    }

    static CASTLE_CONSTEXPR float lowest() CASTLE_NOEXCEPT
    {
        return -FLT_MAX;
    }
};

template <>
struct numeric_limits<double>
{
    static CASTLE_CONSTEXPR bool is_specialized = true;
    static CASTLE_CONSTEXPR bool is_signed = true;
    static CASTLE_CONSTEXPR bool is_integer = false;
    static CASTLE_CONSTEXPR bool is_exact = false;

    static CASTLE_CONSTEXPR double min() CASTLE_NOEXCEPT
    {
        return DBL_MIN;
    }

    static CASTLE_CONSTEXPR double max() CASTLE_NOEXCEPT
    {
        return DBL_MAX;
    }

    static CASTLE_CONSTEXPR double lowest() CASTLE_NOEXCEPT
    {
        return -DBL_MAX;
    }
};

template <>
struct numeric_limits<long double>
{
    static CASTLE_CONSTEXPR bool is_specialized = true;
    static CASTLE_CONSTEXPR bool is_signed = true;
    static CASTLE_CONSTEXPR bool is_integer = false;
    static CASTLE_CONSTEXPR bool is_exact = false;

    static CASTLE_CONSTEXPR long double min() CASTLE_NOEXCEPT
    {
        return LDBL_MIN;
    }

    static CASTLE_CONSTEXPR long double max() CASTLE_NOEXCEPT
    {
        return LDBL_MAX;
    }

    static CASTLE_CONSTEXPR long double lowest() CASTLE_NOEXCEPT
    {
        return -LDBL_MAX;
    }
};

} // namespace castle

#endif // CASTLE_CORE_TYPE_RANGES_H

#ifndef CASTLE_CORE_TYPE_RANGES_H
#define CASTLE_CORE_TYPE_RANGES_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"

#include <limits.h> // For integral limits
#include <float.h> // For floating-point limits
#include <stdint.h> // For fixed-width integer types

namespace castle
{

// Base template: purposefully left undefined so that unsupported types
// trigger a compile-time error rather than failing silently at runtime.
template <typename T>
struct numeric_limits;

#define CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(TYPE, MIN_VAL, MAX_VAL, IS_SIGNED) \
template <> \
struct numeric_limits<TYPE> \
{ \
    static CASTLE_CONSTEXPR bool is_specialized = true; \
    static CASTLE_CONSTEXPR bool is_signed = IS_SIGNED; \
    static CASTLE_CONSTEXPR bool is_integer = true; \
    static CASTLE_CONSTEXPR bool is_exact = true; \
    static CASTLE_CONSTEXPR TYPE min() CASTLE_NOEXCEPT { return static_cast<TYPE>(MIN_VAL); } \
    static CASTLE_CONSTEXPR TYPE max() CASTLE_NOEXCEPT { return static_cast<TYPE>(MAX_VAL); } \
    static CASTLE_CONSTEXPR TYPE lowest() CASTLE_NOEXCEPT { return static_cast<TYPE>(MIN_VAL); } \
};

// Character Types
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(char, CHAR_MIN, CHAR_MAX, (CHAR_MIN < 0))

// Signed / Unsigned Character Types
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(signed char,   SCHAR_MIN, SCHAR_MAX, true)
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(unsigned char, 0,         UCHAR_MAX, false)

// Short
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(short,          SHRT_MIN, SHRT_MAX, true)
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(unsigned short, 0,        USHRT_MAX, false)

// Int
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(int,          INT_MIN, INT_MAX, true)
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(unsigned int, 0,       UINT_MAX, false)

// Long
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(long,          LONG_MIN, LONG_MAX, true)
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(unsigned long, 0,       ULONG_MAX, false)

// Long Long
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(long long,          LLONG_MIN, LLONG_MAX, true)
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(unsigned long long, 0,         ULLONG_MAX, false)

// Boolean Type Specialization
template <>
struct numeric_limits<bool>
{
    static CASTLE_CONSTEXPR bool is_specialized = true;
    static CASTLE_CONSTEXPR bool is_signed = false;
    static CASTLE_CONSTEXPR bool is_integer = true;
    static CASTLE_CONSTEXPR bool is_exact = true;

    static CASTLE_CONSTEXPR bool min() CASTLE_NOEXCEPT { return false; }
    static CASTLE_CONSTEXPR bool max() CASTLE_NOEXCEPT { return true; }
    static CASTLE_CONSTEXPR bool lowest() CASTLE_NOEXCEPT { return false; }
};

#undef CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC

// Floating-Point Specializations (IEEE 754 Constants)
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

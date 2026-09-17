// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// Castle numeric range metadata for built-in scalar types.
// Include this header when constexpr code needs a small, deterministic subset
// of numeric-limits information without depending on std::numeric_limits.
// The header supplies fallback builtin-type bounds for freestanding targets and
// exposes Castle's numeric_limits specializations entirely at compile time.

#ifndef CASTLE_CORE_TYPE_RANGES_HPP
#define CASTLE_CORE_TYPE_RANGES_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"

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

/// @brief Castle numeric limit traits for built-in scalar types.

/// @brief Primary numeric limits template declared for Castle specializations.
/// @tparam T Type queried for range metadata.
template <typename T>
struct numeric_limits;

/// @brief Generates a numeric_limits specialization for an integral type.
/// @param TYPE Integral type being specialized.
/// @param MIN_VAL Minimum representable value.
/// @param MAX_VAL Maximum representable value.
/// @param SIGNED Boolean expression describing whether TYPE is signed.
#define CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(TYPE, MIN_VAL, MAX_VAL, SIGNED) \
template <> \
struct numeric_limits<TYPE> \
{ \
    /** @brief Indicates that Castle provides this specialization. */ \
    static CASTLE_CONSTEXPR bool is_specialized = true; \
    /** @brief Reports whether TYPE is signed. */ \
    static CASTLE_CONSTEXPR bool is_signed = SIGNED; \
    /** @brief Reports that TYPE is an integral type. */ \
    static CASTLE_CONSTEXPR bool is_integer = true; \
    /** @brief Reports that TYPE has exact integer representation. */ \
    static CASTLE_CONSTEXPR bool is_exact = true; \
    \
    /** @brief Returns the minimum representable value for TYPE. */ \
    static CASTLE_CONSTEXPR TYPE min() CASTLE_NOEXCEPT \
    { \
        return static_cast<TYPE>(MIN_VAL); \
    } \
    \
    /** @brief Returns the maximum representable value for TYPE. */ \
    static CASTLE_CONSTEXPR TYPE max() CASTLE_NOEXCEPT \
    { \
        return static_cast<TYPE>(MAX_VAL); \
    } \
    \
    /** @brief Returns the lowest representable value for TYPE. */ \
    static CASTLE_CONSTEXPR TYPE lowest() CASTLE_NOEXCEPT \
    { \
        return static_cast<TYPE>(MIN_VAL); \
    } \
};

/// @brief numeric_limits specialization for char.
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    char,
    CHAR_MIN,
    CHAR_MAX,
    (CHAR_MIN < 0)
)

/// @brief numeric_limits specialization for signed char.
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    signed char,
    SCHAR_MIN,
    SCHAR_MAX,
    true
)

/// @brief numeric_limits specialization for unsigned char.
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    unsigned char,
    0,
    UCHAR_MAX,
    false
)

/// @brief numeric_limits specialization for short.
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    short,
    SHRT_MIN,
    SHRT_MAX,
    true
)

/// @brief numeric_limits specialization for unsigned short.
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    unsigned short,
    0,
    USHRT_MAX,
    false
)

/// @brief numeric_limits specialization for int.
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    int,
    INT_MIN,
    INT_MAX,
    true
)

/// @brief numeric_limits specialization for unsigned int.
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    unsigned int,
    0,
    UINT_MAX,
    false
)

#ifdef LONG_MIN
#ifdef LONG_MAX

/// @brief numeric_limits specialization for long when LONG_* macros are available.
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    long,
    LONG_MIN,
    LONG_MAX,
    true
)

#endif
#endif

#ifdef ULONG_MAX

/// @brief numeric_limits specialization for unsigned long when ULONG_MAX is available.
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    unsigned long,
    0,
    ULONG_MAX,
    false
)

#endif

/// @brief numeric_limits specialization for long long.
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    long long,
    LLONG_MIN,
    LLONG_MAX,
    true
)

/// @brief numeric_limits specialization for unsigned long long.
CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC(
    unsigned long long,
    0,
    ULLONG_MAX,
    false
)

#undef CASTLE_NUMERIC_LIMITS_INTEGRAL_SPEC

/// @brief numeric_limits specialization for bool.
template <>
struct numeric_limits<bool>
{
    /// @brief Indicates that Castle provides this specialization.
    static CASTLE_CONSTEXPR bool is_specialized = true;
    /// @brief Reports that bool is unsigned for numeric-limits purposes.
    static CASTLE_CONSTEXPR bool is_signed = false;
    /// @brief Reports that bool is treated as an integral type.
    static CASTLE_CONSTEXPR bool is_integer = true;
    /// @brief Reports that bool has exact representation.
    static CASTLE_CONSTEXPR bool is_exact = true;

    /// @brief Returns the minimum representable bool value.
    /// @return false.
    static CASTLE_CONSTEXPR bool min() CASTLE_NOEXCEPT
    {
        return false;
    }

    /// @brief Returns the maximum representable bool value.
    /// @return true.
    static CASTLE_CONSTEXPR bool max() CASTLE_NOEXCEPT
    {
        return true;
    }

    /// @brief Returns the lowest representable bool value.
    /// @return false.
    static CASTLE_CONSTEXPR bool lowest() CASTLE_NOEXCEPT
    {
        return false;
    }
};

/// @brief numeric_limits specialization for float.
template <>
struct numeric_limits<float>
{
    /// @brief Indicates that Castle provides this specialization.
    static CASTLE_CONSTEXPR bool is_specialized = true;
    /// @brief Reports that float is signed.
    static CASTLE_CONSTEXPR bool is_signed = true;
    /// @brief Reports that float is not an integral type.
    static CASTLE_CONSTEXPR bool is_integer = false;
    /// @brief Reports that float is not an exact integer-style type.
    static CASTLE_CONSTEXPR bool is_exact = false;

    /// @brief Returns the smallest positive normalized float value.
    /// @return FLT_MIN.
    static CASTLE_CONSTEXPR float min() CASTLE_NOEXCEPT
    {
        return FLT_MIN;
    }

    /// @brief Returns the largest finite float value.
    /// @return FLT_MAX.
    static CASTLE_CONSTEXPR float max() CASTLE_NOEXCEPT
    {
        return FLT_MAX;
    }

    /// @brief Returns the lowest finite float value.
    /// @return -FLT_MAX.
    static CASTLE_CONSTEXPR float lowest() CASTLE_NOEXCEPT
    {
        return -FLT_MAX;
    }
};

/// @brief numeric_limits specialization for double.
template <>
struct numeric_limits<double>
{
    /// @brief Indicates that Castle provides this specialization.
    static CASTLE_CONSTEXPR bool is_specialized = true;
    /// @brief Reports that double is signed.
    static CASTLE_CONSTEXPR bool is_signed = true;
    /// @brief Reports that double is not an integral type.
    static CASTLE_CONSTEXPR bool is_integer = false;
    /// @brief Reports that double is not an exact integer-style type.
    static CASTLE_CONSTEXPR bool is_exact = false;

    /// @brief Returns the smallest positive normalized double value.
    /// @return DBL_MIN.
    static CASTLE_CONSTEXPR double min() CASTLE_NOEXCEPT
    {
        return DBL_MIN;
    }

    /// @brief Returns the largest finite double value.
    /// @return DBL_MAX.
    static CASTLE_CONSTEXPR double max() CASTLE_NOEXCEPT
    {
        return DBL_MAX;
    }

    /// @brief Returns the lowest finite double value.
    /// @return -DBL_MAX.
    static CASTLE_CONSTEXPR double lowest() CASTLE_NOEXCEPT
    {
        return -DBL_MAX;
    }
};

/// @brief numeric_limits specialization for long double.
template <>
struct numeric_limits<long double>
{
    /// @brief Indicates that Castle provides this specialization.
    static CASTLE_CONSTEXPR bool is_specialized = true;
    /// @brief Reports that long double is signed.
    static CASTLE_CONSTEXPR bool is_signed = true;
    /// @brief Reports that long double is not an integral type.
    static CASTLE_CONSTEXPR bool is_integer = false;
    /// @brief Reports that long double is not an exact integer-style type.
    static CASTLE_CONSTEXPR bool is_exact = false;

    /// @brief Returns the smallest positive normalized long double value.
    /// @return LDBL_MIN.
    static CASTLE_CONSTEXPR long double min() CASTLE_NOEXCEPT
    {
        return LDBL_MIN;
    }

    /// @brief Returns the largest finite long double value.
    /// @return LDBL_MAX.
    static CASTLE_CONSTEXPR long double max() CASTLE_NOEXCEPT
    {
        return LDBL_MAX;
    }

    /// @brief Returns the lowest finite long double value.
    /// @return -LDBL_MAX.
    static CASTLE_CONSTEXPR long double lowest() CASTLE_NOEXCEPT
    {
        return -LDBL_MAX;
    }
};

}

#endif

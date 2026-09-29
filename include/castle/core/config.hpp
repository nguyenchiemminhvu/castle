// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
//
// Castle-wide compile-time configuration values and platform facts.
// Include this header when code needs portable answers about the active C++
// language level, native endianness, pointer-width class, or Castle's default
// in-place storage sizing. The header computes everything at compile time,
// optionally reuses platform headers when available, and exposes no heap usage,
// exceptions, RTTI, or runtime initialization requirements.

#ifndef CASTLE_CORE_CONFIG_HPP
#define CASTLE_CORE_CONFIG_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

/// @def CASTLE_CPP_11
/// @brief Reports whether the active language mode is at least C++11.
#define CASTLE_CPP_11 (__cplusplus >= 201103L)
/// @def CASTLE_CPP_14
/// @brief Reports whether the active language mode is at least C++14.
#define CASTLE_CPP_14 (__cplusplus >= 201402L)
/// @def CASTLE_CPP_17
/// @brief Reports whether the active language mode is at least C++17.
#define CASTLE_CPP_17 (__cplusplus >= 201703L)
/// @def CASTLE_CPP_20
/// @brief Reports whether the active language mode is at least C++20.
#define CASTLE_CPP_20 (__cplusplus >= 202002L)

/// @def CASTLE_ENDIAN_LITTLE
/// @brief Numeric tag used for little-endian targets.
#define CASTLE_ENDIAN_LITTLE 0
/// @def CASTLE_ENDIAN_BIG
/// @brief Numeric tag used for big-endian targets.
#define CASTLE_ENDIAN_BIG    1

/// @def CASTLE_ENDIAN_NATIVE
/// @brief Expands to the detected or user-supplied native endian tag.
/// @note Define CASTLE_ENDIAN_NATIVE before including this header to override detection.
#if defined(CASTLE_ENDIAN_NATIVE)
    #if (CASTLE_ENDIAN_NATIVE != CASTLE_ENDIAN_LITTLE) && (CASTLE_ENDIAN_NATIVE != CASTLE_ENDIAN_BIG)
        #error "CASTLE_ENDIAN_NATIVE must be 0 (little endian) or 1 (big endian)"
    #endif
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
    // Prefer compiler-provided byte-order macros when they are available.
    #if (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
        #define CASTLE_ENDIAN_NATIVE CASTLE_ENDIAN_LITTLE
    #elif (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
        #define CASTLE_ENDIAN_NATIVE CASTLE_ENDIAN_BIG
    #else
        #error "Unsupported native byte order. Define CASTLE_ENDIAN_NATIVE explicitly."
    #endif
#elif defined(__ARMEB__) || defined(__THUMBEB__) || defined(__ARM_BIG_ENDIAN)
    #define CASTLE_ENDIAN_NATIVE CASTLE_ENDIAN_BIG
#elif defined(__ARMEL__) || defined(__THUMBEL__)
    #define CASTLE_ENDIAN_NATIVE CASTLE_ENDIAN_LITTLE
#elif defined(__LITTLE_ENDIAN__) && !defined(__BIG_ENDIAN__)
    #define CASTLE_ENDIAN_NATIVE CASTLE_ENDIAN_LITTLE
#elif defined(__BIG_ENDIAN__) && !defined(__LITTLE_ENDIAN__)
    #define CASTLE_ENDIAN_NATIVE CASTLE_ENDIAN_BIG
#else
    #error "Unable to determine native endianness. Define CASTLE_ENDIAN_NATIVE as 0 or 1."
#endif

/// @def CASTLE_IS_LITTLE_ENDIAN
/// @brief Evaluates to true when the native target endianness is little-endian.
#define CASTLE_IS_LITTLE_ENDIAN (CASTLE_ENDIAN_NATIVE == CASTLE_ENDIAN_LITTLE)
/// @def CASTLE_IS_BIG_ENDIAN
/// @brief Evaluates to true when the native target endianness is big-endian.
#define CASTLE_IS_BIG_ENDIAN    (CASTLE_ENDIAN_NATIVE == CASTLE_ENDIAN_BIG)
/// @def CASTLE_HAS_CONSTEXPR_ENDIANNESS
/// @brief Indicates that Castle exposes native endianness as constexpr data.
#define CASTLE_HAS_CONSTEXPR_ENDIANNESS 1

/// @def CASTLE_PLATFORM_16BIT
/// @brief Evaluates to true when pointers are 16 bits wide.
#define CASTLE_PLATFORM_16BIT (sizeof(void*) == 2)
/// @def CASTLE_PLATFORM_32BIT
/// @brief Evaluates to true when pointers are 32 bits wide.
#define CASTLE_PLATFORM_32BIT (sizeof(void*) == 4)
/// @def CASTLE_PLATFORM_64BIT
/// @brief Evaluates to true when pointers are 64 bits wide.
#define CASTLE_PLATFORM_64BIT (sizeof(void*) == 8)

/// @def CASTLE_HAS_BUILTIN_BIT_CAST
/// @brief Evaluates to true when the active compiler provides `__builtin_bit_cast`.
#if !defined(CASTLE_HAS_BUILTIN_BIT_CAST)
    #if defined(__has_builtin)
        #if __has_builtin(__builtin_bit_cast)
            #define CASTLE_HAS_BUILTIN_BIT_CAST 1
        #endif
    #endif
    #if !defined(CASTLE_HAS_BUILTIN_BIT_CAST) && defined(__GNUC__) && !defined(__clang__) && (__GNUC__ >= 11)
        #define CASTLE_HAS_BUILTIN_BIT_CAST 1
    #endif
    #if !defined(CASTLE_HAS_BUILTIN_BIT_CAST)
        #define CASTLE_HAS_BUILTIN_BIT_CAST 0
    #endif
#endif // !defined(CASTLE_HAS_BUILTIN_BIT_CAST)

/// @def CASTLE_USING_STD_NEW
/// @brief Reports whether the standard <new> header is available.
#if !defined(CASTLE_USING_STD_NEW)
    #if defined(__has_include)
        #if __has_include(<new>)
            #define CASTLE_USING_STD_NEW 1
        #else
            #define CASTLE_USING_STD_NEW 0
        #endif
    #else
        #if defined(__STDC_HOSTED__) && (__STDC_HOSTED__ == 1)
            #define CASTLE_USING_STD_NEW 1
        #else
            #define CASTLE_USING_STD_NEW 0
        #endif
    #endif
#endif // !defined(CASTLE_USING_STD_NEW)

/// @def CASTLE_USING_STD_INITIALIZER_LIST
/// @brief Reports whether the standard <initializer_list> header is available.
#if !defined(CASTLE_USING_STD_INITIALIZER_LIST)
    #if defined(__has_include)
        #if __has_include(<initializer_list>)
            #define CASTLE_USING_STD_INITIALIZER_LIST 1
        #else
            #define CASTLE_USING_STD_INITIALIZER_LIST 0
        #endif
    #else
        #define CASTLE_USING_STD_INITIALIZER_LIST 0
    #endif
#endif // !defined(CASTLE_USING_STD_INITIALIZER_LIST)

/// @def CASTLE_USING_POSIX_APIS
/// @brief Reports whether POSIX APIs appear to be available on the target.
#if !defined(CASTLE_USING_POSIX_APIS)
    #if defined(__has_include)
        #if __has_include(<unistd.h>)
            #include <unistd.h>
            #if defined(_POSIX_VERSION)
                #define CASTLE_USING_POSIX_APIS 1
            #else
                #define CASTLE_USING_POSIX_APIS 0
            #endif
        #else
            #define CASTLE_USING_POSIX_APIS 0
        #endif
    #else
        #define CASTLE_USING_POSIX_APIS 0
    #endif
#endif // !defined(CASTLE_USING_POSIX_APIS)

/// @def CASTLE_USING_TIMEX
/// @brief Reports whether the <sys/time.h> header is available for time-related functions.
#if !defined(CASTLE_USING_TIMEX)
    #if defined(__has_include)
        #if __has_include(<sys/time.h>)
            #define CASTLE_USING_TIMEX 1
        #else
            #define CASTLE_USING_TIMEX 0
        #endif
    #else
        #define CASTLE_USING_TIMEX 0
    #endif
#endif // !defined(CASTLE_USING_TIMEX)

/// @def CASTLE_USING_PTHREAD
/// @brief Reports whether POSIX threads appear to be available on the target.
#if !defined(CASTLE_USING_PTHREAD)
    #if defined(__has_include)
        #if __has_include(<unistd.h>) && __has_include(<pthread.h>)
            #include <unistd.h>
            #if defined(_POSIX_THREADS) && (_POSIX_THREADS > 0)
                #define CASTLE_USING_PTHREAD 1
            #else
                #define CASTLE_USING_PTHREAD 0
            #endif
        #else
            #define CASTLE_USING_PTHREAD 0
        #endif
    #endif
#endif // !defined(CASTLE_USING_PTHREAD)

namespace castle
{

/// @brief Identifies the byte order used by the active target.
enum class endian : unsigned char
{
    /// @brief Little-endian byte order.
    little = CASTLE_ENDIAN_LITTLE,
    /// @brief Big-endian byte order.
    big    = CASTLE_ENDIAN_BIG,
    /// @brief The target's detected native byte order.
    native = CASTLE_ENDIAN_NATIVE
};

/// @brief Compile-time alias for the target's native endianness.
static CASTLE_CONSTEXPR endian native_endian = endian::native;
/// @brief Compile-time flag that is true on little-endian targets.
static CASTLE_CONSTEXPR bool is_little_endian = CASTLE_IS_LITTLE_ENDIAN;
/// @brief Compile-time flag that is true on big-endian targets.
static CASTLE_CONSTEXPR bool is_big_endian    = CASTLE_IS_BIG_ENDIAN;

/// @brief Compile-time flag that is true on 16-bit pointer targets.
static inline CASTLE_CONSTEXPR bool platform_16bit = CASTLE_PLATFORM_16BIT;
/// @brief Compile-time flag that is true on 32-bit pointer targets.
static inline CASTLE_CONSTEXPR bool platform_32bit = CASTLE_PLATFORM_32BIT;
/// @brief Compile-time flag that is true on 64-bit pointer targets.
static inline CASTLE_CONSTEXPR bool platform_64bit = CASTLE_PLATFORM_64BIT;

/// @brief Default number of machine words reserved for inplace function storage.
static inline CASTLE_CONSTEXPR size_type inplace_function_storage_words = 8U;
/// @brief Default byte capacity reserved for Castle's inplace storage helpers.
static inline CASTLE_CONSTEXPR size_type inplace_storage_reserved = inplace_function_storage_words * sizeof(void*);
/// @brief Default alignment used for Castle's inplace storage helpers.
static inline CASTLE_CONSTEXPR size_type inplace_alignment_default = alignof(meta::max_align_t);

} // namespace castle

#endif // CASTLE_CORE_CONFIG_HPP

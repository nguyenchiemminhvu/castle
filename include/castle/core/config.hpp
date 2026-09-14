#ifndef CASTLE_CORE_CONFIG_HPP
#define CASTLE_CORE_CONFIG_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

// -----------------------------------------------------------------------------
// Endianness constants
// -----------------------------------------------------------------------------
#define CASTLE_ENDIAN_LITTLE 0
#define CASTLE_ENDIAN_BIG    1

#if defined(CASTLE_ENDIAN_NATIVE)
    #if (CASTLE_ENDIAN_NATIVE != CASTLE_ENDIAN_LITTLE) && (CASTLE_ENDIAN_NATIVE != CASTLE_ENDIAN_BIG)
        #error "CASTLE_ENDIAN_NATIVE must be 0 (little endian) or 1 (big endian)"
    #endif
#elif defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && defined(__ORDER_BIG_ENDIAN__)
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

#define CASTLE_IS_LITTLE_ENDIAN (CASTLE_ENDIAN_NATIVE == CASTLE_ENDIAN_LITTLE)
#define CASTLE_IS_BIG_ENDIAN    (CASTLE_ENDIAN_NATIVE == CASTLE_ENDIAN_BIG)
#define CASTLE_HAS_CONSTEXPR_ENDIANNESS 1

// -----------------------------------------------------------------------------
// Platform architecture
// -----------------------------------------------------------------------------

#define CASTLE_PLATFORM_16BIT (sizeof(void*) == 2)
#define CASTLE_PLATFORM_32BIT (sizeof(void*) == 4)
#define CASTLE_PLATFORM_64BIT (sizeof(void*) == 8)

// -----------------------------------------------------------------------------
// new operator support
// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------
// POSIX threading support
// -----------------------------------------------------------------------------

#if !defined(CASTLE_USING_PTHREAD)
    #if defined(__has_include)
        #if __has_include(<pthread.h>)
            #include <unistd.h>
            // _POSIX_THREADS is defined in <unistd.h> if POSIX threads are supported
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

enum class endian : unsigned char
{
    little = CASTLE_ENDIAN_LITTLE,
    big    = CASTLE_ENDIAN_BIG,
    native = CASTLE_ENDIAN_NATIVE
};

static CASTLE_CONSTEXPR endian native_endian = endian::native;
static CASTLE_CONSTEXPR bool is_little_endian = CASTLE_IS_LITTLE_ENDIAN;
static CASTLE_CONSTEXPR bool is_big_endian    = CASTLE_IS_BIG_ENDIAN;

static inline CASTLE_CONSTEXPR bool platform_16bit = CASTLE_PLATFORM_16BIT;
static inline CASTLE_CONSTEXPR bool platform_32bit = CASTLE_PLATFORM_32BIT;
static inline CASTLE_CONSTEXPR bool platform_64bit = CASTLE_PLATFORM_64BIT;

static inline CASTLE_CONSTEXPR size_type inplace_function_storage_words = 8U;
static inline CASTLE_CONSTEXPR size_type inplace_storage_reserved = inplace_function_storage_words * sizeof(void*);
static inline CASTLE_CONSTEXPR size_type inplace_alignment_default = alignof(castle::max_align_t);

} // namespace castle

#endif // CASTLE_CORE_CONFIG_HPP

#ifndef CASTLE_UTILITY_BIT_CAST_H
#define CASTLE_UTILITY_BIT_CAST_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"

// castle::bit_cast mirrors std::bit_cast (C++20): reinterpret the object
// representation of `source` as an object of type `To`. Requires both types to
// be the same size and trivially copyable.
//
// GCC (>= 11) and Clang (>= 9) provide __builtin_bit_cast, the only way to
// obtain a truly CASTLE_CONSTEXPR, well-defined type-punning primitive. When
// available it is preferred; otherwise the implementation falls back to
// memcpy, which is legal but not CASTLE_CONSTEXPR.
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

namespace castle
{

#if CASTLE_HAS_BUILTIN_BIT_CAST

template <typename To, typename From>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
castle::meta::enable_if_t<sizeof(To) == sizeof(From)
        && castle::is_trivially_copyable<To>::value
        && castle::is_trivially_copyable<From>::value,
        To>
bit_cast(CASTLE_CONST From& source) CASTLE_NOEXCEPT
{
    return __builtin_bit_cast(To, source);
}

#else

template <typename To, typename From>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
castle::meta::enable_if_t<sizeof(To) == sizeof(From)
            && castle::is_trivially_copyable<To>::value
            && castle::is_trivially_copyable<From>::value,
        To>
bit_cast(CASTLE_CONST From& source) CASTLE_NOEXCEPT
{
    To destination;
    (void)__builtin_memcpy(&destination, &source, sizeof(To));
    return destination;
}

#endif

} // namespace castle

#endif // CASTLE_UTILITY_BIT_CAST_H
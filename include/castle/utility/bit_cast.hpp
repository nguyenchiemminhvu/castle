// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bit_cast.hpp
 * @brief Bit-preserving cast between trivially copyable same-size types.
 *
 * Use this header when a value's object representation must be reinterpreted
 * without aliasing through pointers or unions. Both types must be trivially
 * copyable and have identical size. The implementation is heap-free,
 * exception-free, and uses a compiler builtin when available so the cast can
 * remain constexpr.
 *
 * @code
 * struct word { uint32_t value; };
 * uint32_t raw = castle::bit_cast<uint32_t>(word{0x12345678U});
 * @endcode
 */
#ifndef CASTLE_UTILITY_BIT_CAST_HPP
#define CASTLE_UTILITY_BIT_CAST_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/config.hpp"
#include "castle/core/traits.hpp"

namespace castle
{

#if CASTLE_HAS_BUILTIN_BIT_CAST

/**
 * @brief Reinterprets a value by copying its raw bits into another type.
 * @tparam To Destination type.
 * @tparam From Source type.
 * @param source Source value whose object representation is copied.
 * @return A `To` object with the same bit pattern as `source`.
 * @note `To` and `From` must be trivially copyable and have equal size.
 */
template <typename To, typename From>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
castle::meta::enable_if_t<sizeof(To) == sizeof(From)
        && meta::is_trivially_copyable<To>::value
        && meta::is_trivially_copyable<From>::value,
        To>
bit_cast(CASTLE_CONST From& source) CASTLE_NOEXCEPT
{
    return __builtin_bit_cast(To, source);
}

#else

/**
 * @brief Reinterprets a value by copying its raw bits into another type.
 * @tparam To Destination type.
 * @tparam From Source type.
 * @param source Source value whose object representation is copied.
 * @return A `To` object with the same bit pattern as `source`.
 * @note `To` and `From` must be trivially copyable and have equal size.
 */
template <typename To, typename From>
CASTLE_NODISCARD CASTLE_INLINE CASTLE_CONSTEXPR
castle::meta::enable_if_t<sizeof(To) == sizeof(From)
            && meta::is_trivially_copyable<To>::value
            && meta::is_trivially_copyable<From>::value,
        To>
bit_cast(CASTLE_CONST From& source) CASTLE_NOEXCEPT
{
    To destination;
    (void)__builtin_memcpy(&destination, &source, sizeof(To)); // Copies the object representation without aliasing through pointers.
    return destination;
}

#endif

} // namespace castle

#endif // CASTLE_UTILITY_BIT_CAST_HPP

#ifndef CASTLE_BIT_BIT_ROTATE_H
#define CASTLE_BIT_BIT_ROTATE_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"

#include <stdint.h>
#include <limits.h>

namespace castle
{
namespace bit
{

// ──────────────────────────────────────────────────────────────
// rotate_left — circular left rotation of bits.
//   shift : number of bit positions to rotate.
// Bits shifted out at the MSB re-enter at the LSB.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
rotate_left(T value, uint32_t shift) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    CASTLE_CONSTEXPR uint32_t bit_count = sizeof(T) * CHAR_BIT;

    shift %= bit_count;
    if (shift == 0)
    {
        return value;
    }

    uval = (uval << shift) | (uval >> (bit_count - shift));
    return static_cast<T>(uval);
}

// ──────────────────────────────────────────────────────────────
// rotate_right — circular right rotation of bits.
//   shift : number of bit positions to rotate.
// Bits shifted out at the LSB re-enter at the MSB.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
rotate_right(T value, uint32_t shift) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    CASTLE_CONSTEXPR uint32_t bit_count = sizeof(T) * CHAR_BIT;

    shift %= bit_count;
    if (shift == 0)
    {
        return value;
    }

    uval = (uval >> shift) | (uval << (bit_count - shift));
    return static_cast<T>(uval);
}

} // namespace bit
} // namespace castle

#endif // CASTLE_BIT_BIT_ROTATE_H

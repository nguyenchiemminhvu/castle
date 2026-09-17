#ifndef CASTLE_BIT_BIT_MASK_H
#define CASTLE_BIT_BIT_MASK_H

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
// all_bits_mask — returns a value with every bit set to 1.
// Equivalent to ~T{0}.
// ──────────────────────────────────────────────────────────────

template <typename T>
struct all_bits_mask
{
    static CASTLE_CONSTEXPR T value = static_cast<T>(~T{0U});
};

// ──────────────────────────────────────────────────────────────
// single_bit_mask — returns a value with only one bit set.
//   bit_index : zero-based position of the bit to set.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
single_bit_mask(uint32_t bit_index) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(UnsignedT{1U} << bit_index);
    return static_cast<T>(uval);
}

template <size_type bit_index, typename T>
struct single_bit_mask_const
{
    static_assert(bit_index < sizeof(T) * CHAR_BIT,
                  "bit_index is out of range for the type T");
    using UnsignedT = typename meta::make_unsigned<T>::type;
    static CASTLE_CONSTEXPR T value = static_cast<T>(UnsignedT{1U} << bit_index);
};

// ──────────────────────────────────────────────────────────────
// low_bits_mask — returns a mask with the lowest N bits set.
//   bit_count : number of low-order bits to set.
// E.g. low_bits_mask<uint8_t>(3) → 0b00000111.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
low_bits_mask(uint32_t bit_count) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;

    if (bit_count >= sizeof(T) * CHAR_BIT)
    {
        return static_cast<T>(~UnsignedT{0U});
    }

    UnsignedT uval = static_cast<UnsignedT>(UnsignedT{1U} << bit_count) - UnsignedT{1U};
    return static_cast<T>(uval);
}

template <size_type bit_index, typename T>
struct low_bits_mask_const
{
    static_assert(bit_index <= sizeof(T) * CHAR_BIT,
                  "bit_index is out of range for the type T");
    using UnsignedT = typename meta::make_unsigned<T>::type;
    static CASTLE_CONSTEXPR T value = static_cast<T>(static_cast<UnsignedT>(UnsignedT{1U} << bit_index) - UnsignedT{1U});
};

// ──────────────────────────────────────────────────────────────
// high_bits_mask — returns a mask with the highest N bits set.
//   bit_count : number of high-order bits to set.
// E.g. high_bits_mask<uint8_t>(3) → 0b11100000.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
high_bits_mask(uint32_t bit_count) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;

    if (bit_count == 0)
    {
        return static_cast<T>(UnsignedT{0U});
    }

    if (bit_count >= sizeof(T) * CHAR_BIT)
    {
        return static_cast<T>(~UnsignedT{0U});
    }

    UnsignedT uval = static_cast<UnsignedT>(~UnsignedT{0U}) << (sizeof(T) * CHAR_BIT - bit_count);
    return static_cast<T>(uval);
}

template <size_type bit_count, typename T>
struct high_bits_mask_const
{
    static_assert(bit_count <= sizeof(T) * CHAR_BIT, "bit_count is out of range for the type T");
    using UnsignedT = typename meta::make_unsigned<T>::type;
    static CASTLE_CONSTEXPR T value = (bit_count == 0)
                               ? static_cast<T>(UnsignedT{0U})
                               : (bit_count >= sizeof(T) * CHAR_BIT)
                                 ? static_cast<T>(~UnsignedT{0U})
                                 : static_cast<T>(static_cast<UnsignedT>(~UnsignedT{0U}) << (sizeof(T) * CHAR_BIT - bit_count));
};

// ──────────────────────────────────────────────────────────────
// range_mask — returns a mask with a contiguous range of bits
// set, starting at start_bit_index with bit_count bits wide.
// E.g. range_mask<uint8_t>(2, 3) → 0b00011100.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
range_mask(uint32_t start_bit_index, uint32_t bit_count) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(low_bits_mask<UnsignedT>(bit_count) << start_bit_index);
    return static_cast<T>(uval);
}

template <size_type start_bit_index, size_type bit_count, typename T>
struct range_mask_const
{
    static_assert(start_bit_index + bit_count <= sizeof(T) * CHAR_BIT,
                  "start_bit_index + bit_count is out of range for the type T");
    using UnsignedT = typename meta::make_unsigned<T>::type;
    static CASTLE_CONSTEXPR T value = static_cast<T>(low_bits_mask_const<bit_count, UnsignedT>::value << start_bit_index);
};

} // namespace bit
} // namespace castle

#endif // CASTLE_BIT_BIT_MASK_H

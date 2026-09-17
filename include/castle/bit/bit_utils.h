#ifndef CASTLE_BIT_BIT_UTILS_H
#define CASTLE_BIT_BIT_UTILS_H

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
// extract_lowest_set_bit — isolates the lowest set bit in a
// value, returning a word with only that bit set.
// Uses the two's complement trick: x & (-x).
// E.g. extract_lowest_set_bit(0b10110100) → 0b00000100.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
extract_lowest_set_bit(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval & -uval);
}

// ──────────────────────────────────────────────────────────────
// extract_highest_set_bit — isolates the highest set bit in a
// value, returning a word with only that bit set.
// Uses bit-smearing followed by subtraction.
// E.g. extract_highest_set_bit(0b10110100) → 0b10000000.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
extract_highest_set_bit(T value) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (uval == 0) return 0;

    for (size_type i = 1; i < sizeof(T) * CHAR_BIT; i *= 2)
        uval |= uval >> i;

    return static_cast<T>(uval - (uval >> 1));
}

// ──────────────────────────────────────────────────────────────
// extract_field — read a contiguous bit field from a value.
//   start_bit : LSB position of the field
//   width     : number of bits in the field
// Returns the field value right-shifted to bit 0.
// This is the fundamental operation for reading hardware
// register fields in embedded systems.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
extract_field(T value, uint32_t start_bit, uint32_t width) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    if (width == 0 || start_bit >= sizeof(T) * CHAR_BIT)
        return static_cast<T>(UnsignedT{0U});

    if (width >= sizeof(T) * CHAR_BIT)
        return static_cast<T>(uval >> start_bit);

    UnsignedT mask = (UnsignedT{1U} << width) - UnsignedT{1U};
    return static_cast<T>((uval >> start_bit) & mask);
}

template <size_type start_bit, size_type width, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
extract_field(T value) CASTLE_NOEXCEPT
{
    static_assert(start_bit + width <= sizeof(T) * CHAR_BIT,
                  "extract_field: start_bit + width exceeds bit width of T");
    static_assert(width > 0, "extract_field: width must be > 0");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    CASTLE_CONSTEXPR UnsignedT mask = (UnsignedT{1U} << width) - UnsignedT{1U};
    return static_cast<T>((uval >> start_bit) & mask);
}

// ──────────────────────────────────────────────────────────────
// insert_field — write a contiguous bit field into a value.
//   dest      : the original register / word
//   field_val : the value to insert (only lowest 'width' bits used)
//   start_bit : LSB position of the field
//   width     : number of bits in the field
// Returns the modified value with the field replaced.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
insert_field(T dest, T field_val, uint32_t start_bit, uint32_t width) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT udest = static_cast<UnsignedT>(dest);
    UnsignedT ufield = static_cast<UnsignedT>(field_val);

    if (width == 0 || start_bit >= sizeof(T) * CHAR_BIT)
        return dest;

    UnsignedT mask;
    if (width >= sizeof(T) * CHAR_BIT)
        mask = ~UnsignedT{0U};
    else
        mask = (UnsignedT{1U} << width) - UnsignedT{1U};

    ufield &= mask;                            // clamp to field width
    udest &= ~(mask << start_bit);             // clear the field in dest
    udest |= (ufield << start_bit);            // insert the new value
    return static_cast<T>(udest);
}

template <size_type start_bit, size_type width, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
insert_field(T dest, T field_val) CASTLE_NOEXCEPT
{
    static_assert(start_bit + width <= sizeof(T) * CHAR_BIT,
                  "insert_field: start_bit + width exceeds bit width of T");
    static_assert(width > 0, "insert_field: width must be > 0");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT udest = static_cast<UnsignedT>(dest);
    UnsignedT ufield = static_cast<UnsignedT>(field_val);
    CASTLE_CONSTEXPR UnsignedT mask = (UnsignedT{1U} << width) - UnsignedT{1U};

    ufield &= mask;
    udest &= ~(mask << start_bit);
    udest |= (ufield << start_bit);
    return static_cast<T>(udest);
}

} // namespace bit
} // namespace castle

#endif // CASTLE_BIT_BIT_UTILS_H
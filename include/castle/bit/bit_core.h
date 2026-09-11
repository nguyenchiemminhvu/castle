#ifndef CASTLE_BIT_BIT_CORE_H
#define CASTLE_BIT_BIT_CORE_H

#include "castle/core/compiler.h"
#include "castle/core/types.h"
#include "castle/core/traits.h"

#include <stdint.h>
#include <limits.h>

namespace castle
{
namespace bit
{

// ──────────────────────────────────────────────────────────────
// test — check whether a specific bit is set (1) in a value.
//   bit_index : zero-based position of the bit to test.
// Returns true if the bit is set, false otherwise.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
test(T value, uint32_t bit_index) CASTLE_NOEXCEPT
{
    if (bit_index >= sizeof(T) * CHAR_BIT)
    {
        return false;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return ((uval >> bit_index) & UnsignedT{1U}) != UnsignedT{0U};
}

template <size_type bit_index, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
test(T value) CASTLE_NOEXCEPT
{
    static_assert(bit_index < sizeof(T) * CHAR_BIT, "bit_index is out of range for the type T");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return ((uval >> bit_index) & UnsignedT{1U}) != UnsignedT{0U};
}

// ──────────────────────────────────────────────────────────────
// set — set a specific bit to 1 in a value.
//   bit_index : zero-based position of the bit to set.
// Returns the modified value with the target bit set.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
set(T value, uint32_t bit_index) CASTLE_NOEXCEPT
{
    if (bit_index >= sizeof(T) * CHAR_BIT)
    {
        return value;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval | (UnsignedT{1U} << bit_index));
}

template <size_type bit_index, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
set(T value) CASTLE_NOEXCEPT
{
    static_assert(bit_index < sizeof(T) * CHAR_BIT, "bit_index is out of range for the type T");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval | (UnsignedT{1U} << bit_index));
}

// ──────────────────────────────────────────────────────────────
// clear — clear a specific bit to 0 in a value.
//   bit_index : zero-based position of the bit to clear.
// Returns the modified value with the target bit cleared.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
clear(T value, uint32_t bit_index) CASTLE_NOEXCEPT
{
    if (bit_index >= sizeof(T) * CHAR_BIT)
    {
        return value;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval & static_cast<UnsignedT>(~(UnsignedT{1U} << bit_index)));
}

template <size_type bit_index, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
clear(T value) CASTLE_NOEXCEPT
{
    static_assert(bit_index < sizeof(T) * CHAR_BIT, "bit_index is out of range for the type T");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval & static_cast<UnsignedT>(~(UnsignedT{1U} << bit_index)));
}

// ──────────────────────────────────────────────────────────────
// toggle — flip a specific bit in a value (0→1, 1→0).
//   bit_index : zero-based position of the bit to toggle.
// Returns the modified value with the target bit inverted.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
toggle(T value, uint32_t bit_index) CASTLE_NOEXCEPT
{
    if (bit_index >= sizeof(T) * CHAR_BIT)
    {
        return value;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval ^ (UnsignedT{1U} << bit_index));
}

template <size_type bit_index, typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
toggle(T value) CASTLE_NOEXCEPT
{
    static_assert(bit_index < sizeof(T) * CHAR_BIT, "bit_index is out of range for the type T");

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);

    return static_cast<T>(uval ^ (UnsignedT{1U} << bit_index));
}

} // namespace bit
} // namespace castle

#endif // CASTLE_BIT_BIT_CORE_H

#ifndef CASTLE_BIT_BIT_MATH_H
#define CASTLE_BIT_BIT_MATH_H

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
// is_even / is_odd — check the parity of an integer using
// the least-significant bit. Branch-free.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
is_even(T v) CASTLE_NOEXCEPT
{
    return (v & 1) == 0;
}

template <size_type N>
struct is_even_const
{
    static CASTLE_CONSTEXPR bool value = ((N & 1) == 0);
};

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
is_odd(T v) CASTLE_NOEXCEPT
{
    return (v & 1) != 0;
}

template <size_type N>
struct is_odd_const
{
    static CASTLE_CONSTEXPR bool value = ((N & 1) != 0);
};

// ──────────────────────────────────────────────────────────────
// is_power_of_two — returns true if the value is an exact
// power of two (has exactly one bit set). Returns false for
// zero and negative values.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
is_power_of_two(T v) CASTLE_NOEXCEPT
{
    if (v < 0)
        return false; // Negative numbers are not powers of two

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(v);
    return uval != 0 && (uval & (uval - 1)) == 0;
}

template <size_type N>
struct is_power_of_two_const
{
    static CASTLE_CONSTEXPR bool value = (N != 0 && (N & (N - 1)) == 0);
};

// ──────────────────────────────────────────────────────────────
// next_power_of_two — returns the smallest power of two that
// is greater than or equal to the input value.
// Returns 1 for input 0.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
next_power_of_two(T v) CASTLE_NOEXCEPT
{
    if (v == 0)
    {
        return 1;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(v);

    --uval; // handle edge case where v is already a power of two
    for (size_type i = 1; i < sizeof(T) * CHAR_BIT; i *= 2)
    {
        uval |= uval >> i;
    }

    return static_cast<T>(uval + 1);
}

template <size_type N>
CASTLE_CONSTEXPR size_type next_power_of_two() CASTLE_NOEXCEPT
{
    if (N == 0)
    {
        return 1;
    }

    size_type v = N - 1;
    for (size_type i = 1; i < sizeof(size_type) * CHAR_BIT; i *= 2)
    {
        v |= v >> i;
    }

    return v + 1;
}

template <size_type N>
struct next_power_of_two_const
{
    static CASTLE_CONSTEXPR size_type value = next_power_of_two<N>();
};

// ──────────────────────────────────────────────────────────────
// previous_power_of_two — returns the largest power of two
// that is less than or equal to the input value.
// Returns 0 for input 0.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
previous_power_of_two(T v) CASTLE_NOEXCEPT
{
    if (v == 0)
    {
        return 0;
    }

    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(v);

    for (size_type i = 1; i < sizeof(T) * CHAR_BIT; i *= 2)
    {
        uval |= uval >> i;
    }

    return static_cast<T>(uval - (uval >> 1));
}

template <size_type N>
CASTLE_CONSTEXPR size_type previous_power_of_two() CASTLE_NOEXCEPT
{
    if (N == 0)
    {
        return 0;
    }

    size_type v = N;
    for (size_type i = 1; i < sizeof(size_type) * CHAR_BIT; i *= 2)
    {
        v |= v >> i;
    }

    return v - (v >> 1);
}

template <size_type N>
struct previous_power_of_two_const
{
    static CASTLE_CONSTEXPR size_type value = previous_power_of_two<N>();
};

// ──────────────────────────────────────────────────────────────
// align_up / align_down — round an address or size to the
// nearest multiple of a power-of-two alignment.
// Ubiquitous in DMA buffer setup, memory-mapped I/O, and
// linker-script math in embedded systems.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
align_up(T value, T alignment) CASTLE_NOEXCEPT
{
    // alignment must be a power of two; caller's responsibility.
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    UnsignedT mask = static_cast<UnsignedT>(alignment) - UnsignedT{1U};
    return static_cast<T>((uval + mask) & ~mask);
}

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
align_down(T value, T alignment) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(value);
    UnsignedT mask = static_cast<UnsignedT>(alignment) - UnsignedT{1U};
    return static_cast<T>(uval & ~mask);
}

// ──────────────────────────────────────────────────────────────
// is_aligned — returns true if the value is a multiple of the
// given power-of-two alignment.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, bool>
is_aligned(T value, T alignment) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT mask = static_cast<UnsignedT>(alignment) - UnsignedT{1U};
    return (static_cast<UnsignedT>(value) & mask) == UnsignedT{0U};
}

// ──────────────────────────────────────────────────────────────
// sign — returns -1, 0, or +1 indicating the sign of a value.
// Branch-free implementation using comparison operators.
// ──────────────────────────────────────────────────────────────

template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, int>
sign(T v) CASTLE_NOEXCEPT
{
    return (v > 0) - (v < 0);
}

template <size_type N>
CASTLE_CONSTEXPR int sign() CASTLE_NOEXCEPT
{
    return (N > 0) - (N < 0);
}

} // namespace bit
} // namespace castle

#endif // CASTLE_BIT_BIT_MATH_H

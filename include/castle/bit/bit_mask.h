#pragma once

#include "castle/types/traits.h"

#include <cstdint>
#include <climits>

using namespace castle::types;

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
    static constexpr T value = static_cast<T>(~T{0U});
};

template <typename T>
inline static constexpr T all_bits_mask_v = all_bits_mask<T>::value;

// ──────────────────────────────────────────────────────────────
// single_bit_mask — returns a value with only one bit set.
//   bit_index : zero-based position of the bit to set.
// ──────────────────────────────────────────────────────────────

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
single_bit_mask(std::uint32_t bit_index) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(UnsignedT{1U} << bit_index);
    return static_cast<T>(uval);
}

template <std::size_t bit_index, typename T>
struct single_bit_mask_const
{
    static_assert(bit_index < sizeof(T) * CHAR_BIT, "bit_index is out of range for the type T");
    using UnsignedT = typename std::make_unsigned<T>::type;
    static constexpr T value = static_cast<T>(UnsignedT{1U} << bit_index);
};

template <std::size_t bit_index, typename T>
inline static constexpr T single_bit_mask_v = single_bit_mask_const<bit_index, T>::value;

// ──────────────────────────────────────────────────────────────
// low_bits_mask — returns a mask with the lowest N bits set.
//   bit_count : number of low-order bits to set.
// E.g. low_bits_mask<uint8_t>(3) → 0b00000111.
// ──────────────────────────────────────────────────────────────

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
low_bits_mask(std::uint32_t bit_count) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;

    if (bit_count >= sizeof(T) * CHAR_BIT)
        return static_cast<T>(~UnsignedT{0U});

    UnsignedT uval = static_cast<UnsignedT>(UnsignedT{1U} << bit_count) - UnsignedT{1U};
    return static_cast<T>(uval);
}

template <std::size_t bit_index, typename T>
struct low_bits_mask_const
{
    static_assert(bit_index <= sizeof(T) * CHAR_BIT, "bit_index is out of range for the type T");
    using UnsignedT = typename std::make_unsigned<T>::type;
    static constexpr T value = static_cast<T>(static_cast<UnsignedT>(UnsignedT{1U} << bit_index) - UnsignedT{1U});
};

template <std::size_t bit_index, typename T>
inline static constexpr T low_bits_mask_v = low_bits_mask_const<bit_index, T>::value;

// ──────────────────────────────────────────────────────────────
// high_bits_mask — returns a mask with the highest N bits set.
//   bit_count : number of high-order bits to set.
// E.g. high_bits_mask<uint8_t>(3) → 0b11100000.
// ──────────────────────────────────────────────────────────────

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
high_bits_mask(std::uint32_t bit_count) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;

    if (bit_count == 0)
        return static_cast<T>(UnsignedT{0U});

    if (bit_count >= sizeof(T) * CHAR_BIT)
        return static_cast<T>(~UnsignedT{0U});

    UnsignedT uval = static_cast<UnsignedT>(~UnsignedT{0U}) << (sizeof(T) * CHAR_BIT - bit_count);
    return static_cast<T>(uval);
}

template <std::size_t bit_count, typename T>
struct high_bits_mask_const
{
    static_assert(bit_count <= sizeof(T) * CHAR_BIT, "bit_count is out of range for the type T");
    using UnsignedT = typename std::make_unsigned<T>::type;
    static constexpr T value = (bit_count == 0)
                               ? static_cast<T>(UnsignedT{0U})
                               : (bit_count >= sizeof(T) * CHAR_BIT)
                                 ? static_cast<T>(~UnsignedT{0U})
                                 : static_cast<T>(static_cast<UnsignedT>(~UnsignedT{0U}) << (sizeof(T) * CHAR_BIT - bit_count));
};

template <std::size_t bit_count, typename T>
inline static constexpr T high_bits_mask_v = high_bits_mask_const<bit_count, T>::value;

// ──────────────────────────────────────────────────────────────
// range_mask — returns a mask with a contiguous range of bits
// set, starting at start_bit_index with bit_count bits wide.
// E.g. range_mask<uint8_t>(2, 3) → 0b00011100.
// ──────────────────────────────────────────────────────────────

template <typename T>
constexpr
typename std::enable_if_t<is_valid_integer_v<T>, T>
range_mask(std::uint32_t start_bit_index, std::uint32_t bit_count) noexcept
{
    using UnsignedT = typename std::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(low_bits_mask<UnsignedT>(bit_count) << start_bit_index);
    return static_cast<T>(uval);
}

template <std::size_t start_bit_index, std::size_t bit_count, typename T>
struct range_mask_const
{
    static_assert(start_bit_index + bit_count <= sizeof(T) * CHAR_BIT, "start_bit_index + bit_count is out of range for the type T");
    using UnsignedT = typename std::make_unsigned<T>::type;
    static constexpr T value = static_cast<T>(low_bits_mask_v<bit_count, UnsignedT> << start_bit_index);
};

template <std::size_t start_bit_index, std::size_t bit_count, typename T>
inline static constexpr T range_mask_v = range_mask_const<start_bit_index, bit_count, T>::value;

} // namespace bit
} // namespace castle
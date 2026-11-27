// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file bit_mask.hpp
 * @brief Mask builders for single bits and contiguous bit ranges.
 *
 * Use this header when register definitions or packed-field code needs
 * reusable masks for one bit, low bits, high bits, or a contiguous span.
 *
 * Key constraints:
 * - Accepts Castle integer types that satisfy `castle::meta::is_valid_integer` where gated.
 * - Some runtime builders perform no bounds checking before shifting; keep indexes and ranges valid.
 * - No allocation, exceptions, RTTI, virtual dispatch, or STL facilities are used.
 */
#ifndef CASTLE_BIT_BIT_MASK_HPP
#define CASTLE_BIT_BIT_MASK_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

#include <stdint.h>
#include <limits.h>

namespace castle
{
namespace bit
{

/**
 * @brief Compile-time mask whose bits are all set.
 * @tparam T Integral storage type for the mask.
 */
template <typename T>
struct all_bits_mask
{
    static CASTLE_CONSTEXPR T value = static_cast<T>(~T{0U});
};

/**
 * @brief Builds a mask with one bit set.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param bit_index Zero-based bit position to set.
 * @return A value whose selected bit is 1 and whose other bits are 0.
 * @warning This overload does not check `bit_index` before shifting.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
single_bit_mask(uint32_t bit_index) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(UnsignedT{1U} << bit_index);
    return static_cast<T>(uval);
}

/**
 * @brief Compile-time mask with one selected bit set.
 * @tparam bit_index Zero-based bit position to set.
 * @tparam T Integral storage type for the mask.
 */
template <size_type bit_index, typename T>
struct single_bit_mask_const
{
    static_assert(bit_index < sizeof(T) * CHAR_BIT,
                  "bit_index is out of range for the type T");
    using UnsignedT = typename meta::make_unsigned<T>::type;
    static CASTLE_CONSTEXPR T value = static_cast<T>(UnsignedT{1U} << bit_index);
};

/**
 * @brief Builds a mask with the lowest `bit_count` bits set.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param bit_count Number of low-order bits to set.
 * @return A low-bit mask. When `bit_count` is greater than or equal to the bit
 *         width of `T`, the result is an all-ones value for `T`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
low_bits_mask(uint32_t bit_count) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;

    if (bit_count >= sizeof(T) * CHAR_BIT)
    {
        return static_cast<T>(~UnsignedT{0U}); // Avoid shifting by the storage width.
    }

    UnsignedT uval = static_cast<UnsignedT>(UnsignedT{1U} << bit_count) - UnsignedT{1U};
    return static_cast<T>(uval);
}

/**
 * @brief Compile-time mask with the lowest `bit_index` bits set.
 * @tparam bit_index Number of low-order bits to set.
 * @tparam T Integral storage type for the mask.
 * @warning This form computes the mask with a left shift and does not mirror
 *          the runtime overload's full-width special case.
 */
template <size_type bit_index, typename T>
struct low_bits_mask_const
{
    static_assert(bit_index <= sizeof(T) * CHAR_BIT,
                  "bit_index is out of range for the type T");
    using UnsignedT = typename meta::make_unsigned<T>::type;
    static CASTLE_CONSTEXPR T value = static_cast<T>(static_cast<UnsignedT>(UnsignedT{1U} << bit_index) - UnsignedT{1U});
};

/**
 * @brief Builds a mask with the highest `bit_count` bits set.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param bit_count Number of high-order bits to set.
 * @return A high-bit mask. Returns 0 when `bit_count` is 0 and an all-ones
 *         value when `bit_count` is greater than or equal to the bit width of `T`.
 */
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

/**
 * @brief Compile-time mask with the highest `bit_count` bits set.
 * @tparam bit_count Number of high-order bits to set.
 * @tparam T Integral storage type for the mask.
 */
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

/**
 * @brief Builds a mask for a contiguous bit range.
 * @tparam T Integer type accepted by `castle::meta::is_valid_integer`.
 * @param start_bit_index Zero-based starting bit position.
 * @param bit_count Number of bits in the range.
 * @return A value whose selected range is set and whose other bits are clear.
 * @warning This overload does not validate that the shifted range stays within the bit width of `T`.
 */
template <typename T>
CASTLE_CONSTEXPR
typename meta::enable_if_t<meta::is_valid_integer<T>::value, T>
range_mask(uint32_t start_bit_index, uint32_t bit_count) CASTLE_NOEXCEPT
{
    using UnsignedT = typename meta::make_unsigned<T>::type;
    UnsignedT uval = static_cast<UnsignedT>(low_bits_mask<UnsignedT>(bit_count) << start_bit_index);
    return static_cast<T>(uval);
}

/**
 * @brief Compile-time mask for a contiguous bit range.
 * @tparam start_bit_index Zero-based starting bit position.
 * @tparam bit_count Number of bits in the range.
 * @tparam T Integral storage type for the mask.
 */
template <size_type start_bit_index, size_type bit_count, typename T>
struct range_mask_const
{
    static_assert(start_bit_index + bit_count <= sizeof(T) * CHAR_BIT,
                  "start_bit_index + bit_count is out of range for the type T");
    using UnsignedT = typename meta::make_unsigned<T>::type;
    static CASTLE_CONSTEXPR T value = static_cast<T>(low_bits_mask_const<bit_count, UnsignedT>::value << start_bit_index);
};

}
}

#endif

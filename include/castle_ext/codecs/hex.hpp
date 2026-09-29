// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file hex.hpp
 * @brief Fixed-buffer hexadecimal encoder and decoder.
 */
#ifndef CASTLE_EXT_CODECS_HEX_HPP
#define CASTLE_EXT_CODECS_HEX_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace codecs
{

/**
 * @brief Returns the encoded length for a hexadecimal input.
 * @param input_size Number of source bytes.
 * @return Two characters per byte.
 */
CASTLE_CONSTEXPR size_type hex_encoded_size(size_type input_size) CASTLE_NOEXCEPT
{
    return input_size * 2U;
}

namespace detail
{
/** @brief Maps a hex digit (either case) to its value, or 0xFF if invalid. */
CASTLE_CONSTEXPR uint8_t hex_value(uint8_t c) CASTLE_NOEXCEPT
{
    if (c >= static_cast<uint8_t>('0') && c <= static_cast<uint8_t>('9')) // LCOV_EXCL_BR_LINE
    {
        return static_cast<uint8_t>(c - '0');
    }
    if (c >= static_cast<uint8_t>('A') && c <= static_cast<uint8_t>('F')) // LCOV_EXCL_BR_LINE
    {
        return static_cast<uint8_t>(c - 'A') + 10U;
    }
    if (c >= static_cast<uint8_t>('a') && c <= static_cast<uint8_t>('f')) // LCOV_EXCL_BR_LINE
    {
        return static_cast<uint8_t>(c - 'a') + 10U;
    }
    return 0xFFU;
}
}

/**
 * @brief Encodes bytes to hexadecimal text without a terminating null byte.
 *
 * @param input           Source bytes; may be null only when `input_size == 0`.
 * @param input_size      Source byte count.
 * @param output          Destination character buffer.
 * @param output_capacity Destination capacity in characters.
 * @param[out] output_size Number of characters written, zero on failure.
 * @param uppercase       When true, `A`-`F` is used instead of `a`-`f`.
 * @return `status::ok`, `status::full`, or `status::invalid_argument`.
 */
inline status hex_encode(CASTLE_CONST uint8_t* input,
                         size_type input_size,
                         char* output,
                         size_type output_capacity,
                         size_type& output_size,
                         bool uppercase = false) CASTLE_NOEXCEPT
{
    output_size = 0U;
    if (input == 0 && input_size != 0U)
    {
        return status::invalid_argument;
    }
    // LCOV_EXCL_START
    if (output == 0 && output_capacity != 0U)
    {
        return status::invalid_argument;
    }
    CASTLE_CONST size_type required = hex_encoded_size(input_size);
    if (required > output_capacity)
    {
        return status::full;
    }
    if (required != 0U && output == 0)
    {
        return status::invalid_argument;
    }
    // LCOV_EXCL_STOP

    CASTLE_CONST char* alphabet = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    for (size_type i = 0U; i < input_size; ++i)
    {
        output[2U * i] = alphabet[(input[i] >> 4U) & 0x0FU];
        output[2U * i + 1U] = alphabet[input[i] & 0x0FU];
    }
    output_size = required;
    return status::ok;
}

/**
 * @brief Decodes hexadecimal text into a caller-owned byte buffer.
 *
 * @param input           Source characters; either case is accepted.
 * @param input_size      Source character count; must be even.
 * @param output          Destination byte buffer.
 * @param output_capacity Destination capacity in bytes.
 * @param[out] output_size Number of bytes written, zero on failure.
 * @return `status::ok`, `status::full`, or `status::invalid_argument`
 *         (odd length, non-hex character or null pointer).
 */
inline status hex_decode(CASTLE_CONST char* input,
                         size_type input_size,
                         uint8_t* output,
                         size_type output_capacity,
                         size_type& output_size) CASTLE_NOEXCEPT
{
    output_size = 0U;
    // LCOV_EXCL_START
    if (input == 0 && input_size != 0U)
    {
        return status::invalid_argument;
    }
    if (output == 0 && output_capacity != 0U)
    {
        return status::invalid_argument;
    }
    if ((input_size & 1U) != 0U)
    {
        return status::invalid_argument;
    }
    CASTLE_CONST size_type required = input_size / 2U;
    if (required > output_capacity)
    {
        return status::full;
    }
    if (required != 0U && output == 0)
    {
        return status::invalid_argument;
    }
    // LCOV_EXCL_STOP

    for (size_type i = 0U; i < required; ++i)
    {
        CASTLE_CONST uint8_t high = detail::hex_value(static_cast<uint8_t>(input[2U * i]));
        CASTLE_CONST uint8_t low = detail::hex_value(static_cast<uint8_t>(input[2U * i + 1U]));
        if (high == 0xFFU || low == 0xFFU) // LCOV_EXCL_BR_LINE
        {
            return status::invalid_argument;
        }
        output[i] = static_cast<uint8_t>((high << 4U) | low);
    }
    output_size = required;
    return status::ok;
}

} // namespace codecs
} // namespace castle

#endif // CASTLE_EXT_CODECS_HEX_HPP

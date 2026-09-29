// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file base32.hpp
 * @brief Fixed-buffer RFC 4648 Base32 encoder and decoder.
 */
#ifndef CASTLE_EXT_CODECS_BASE32_HPP
#define CASTLE_EXT_CODECS_BASE32_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace codecs
{

/**
 * @brief Returns the exact encoded length for a Base32 input.
 * @param input_size Number of source bytes.
 * @param padding    True when `=` padding completes the final 8-character group.
 * @return Number of characters `base32_encode()` produces.
 */
CASTLE_CONSTEXPR size_type base32_encoded_size(size_type input_size, bool padding = true) CASTLE_NOEXCEPT
{
    CASTLE_CONST size_type full_groups = input_size / 5U;
    CASTLE_CONST size_type remainder = input_size % 5U;
    if (remainder == 0U)
    {
        return full_groups * 8U;
    }

    if (!padding)
    {
        return full_groups * 8U + (remainder == 1U ? 2U : remainder == 2U ? 4U : remainder == 3U ? 5U : 7U); // LCOV_EXCL_BR_LINE
    }
    return full_groups * 8U + 8U;
}

/**
 * @brief Returns the decoded byte count for a Base32 data-character count.
 * @param input_size Number of data characters, excluding `=` padding.
 * @return Number of bytes `base32_decode()` produces for valid input.
 */
CASTLE_CONSTEXPR size_type base32_decoded_size(size_type input_size) CASTLE_NOEXCEPT
{
    CASTLE_CONST size_type full_groups = input_size / 8U;
    CASTLE_CONST size_type remainder = input_size % 8U;
    size_type extra = 0U;
    if (remainder >= 2U) extra = 1U; // LCOV_EXCL_BR_LINE
    if (remainder >= 4U) extra = 2U; // LCOV_EXCL_BR_LINE
    if (remainder >= 5U) extra = 3U; // LCOV_EXCL_BR_LINE
    if (remainder >= 7U) extra = 4U; // LCOV_EXCL_BR_LINE
    return full_groups * 5U + extra;
}

namespace detail
{
/** @brief Maps a Base32 character (either case) to its 5-bit value, or -1 if invalid. */
CASTLE_CONSTEXPR int base32_value(uint8_t c) CASTLE_NOEXCEPT
{
    // LCOV_EXCL_START
    if (c >= static_cast<uint8_t>('A') && c <= static_cast<uint8_t>('Z'))
    {
        return static_cast<int>(c - 'A');
    }
    if (c >= static_cast<uint8_t>('a') && c <= static_cast<uint8_t>('z'))
    {
        return static_cast<int>(c - 'a');
    }
    if (c >= static_cast<uint8_t>('2') && c <= static_cast<uint8_t>('7'))
    {
        return static_cast<int>(c - '2') + 26;
    }
    // LCOV_EXCL_STOP
    return -1;
}

/** @brief Maps a 5-bit value (0..31) to its uppercase RFC 4648 Base32 character. */
CASTLE_CONSTEXPR uint8_t base32_char(uint8_t value) CASTLE_NOEXCEPT
{
    return value < 26U ? static_cast<uint8_t>('A' + value)
                       : static_cast<uint8_t>('2' + (value - 26U));
}

} // namespace detail

/**
 * @brief Encodes bytes into RFC 4648 Base32 text.
 *
 * @param input           Source bytes; may be null only when `input_size == 0`.
 * @param input_size      Source byte count.
 * @param output          Destination character buffer; no terminating null is written.
 * @param output_capacity Destination capacity in characters.
 * @param[out] output_size Number of characters written, zero on failure.
 * @param padding         When true, `=` padding is emitted.
 * @return `status::ok`, `status::full`, or `status::invalid_argument`.
 */
inline status base32_encode(CASTLE_CONST uint8_t* input,
                            size_type input_size,
                            char* output,
                            size_type output_capacity,
                            size_type& output_size,
                            bool padding = true) CASTLE_NOEXCEPT
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
    CASTLE_CONST size_type required = base32_encoded_size(input_size, padding);
    if (required > output_capacity)
    {
        return status::full;
    }
    if (required != 0U && output == 0)
    {
        return status::invalid_argument;
    }
    // LCOV_EXCL_STOP

    size_type in = 0U;
    size_type out = 0U;
    while (in + 5U <= input_size)
    {
        uint64_t block = static_cast<uint64_t>(input[in]) << 32U |
                         static_cast<uint64_t>(input[in + 1U]) << 24U |
                         static_cast<uint64_t>(input[in + 2U]) << 16U |
                         static_cast<uint64_t>(input[in + 3U]) << 8U |
                         static_cast<uint64_t>(input[in + 4U]);
        for (int shift = 35; shift >= 0; shift -= 5)
        {
            output[out++] = static_cast<char>(detail::base32_char(static_cast<uint8_t>((block >> shift) & 0x1FU)));
        }
        in += 5U;
    }

    CASTLE_CONST size_type rem = input_size - in;
    if (rem != 0U)
    {
        uint64_t block = 0U;
        for (size_type i = 0U; i < rem; ++i)
        {
            block |= static_cast<uint64_t>(input[in + i]) << (32U - 8U * i);
        }
        CASTLE_CONST size_type chars = rem == 1U ? 2U : rem == 2U ? 4U : rem == 3U ? 5U : 7U; // LCOV_EXCL_BR_LINE
        for (size_type i = 0U; i < chars; ++i)
        {
            output[out++] = static_cast<char>(detail::base32_char(static_cast<uint8_t>((block >> (35U - 5U * i)) & 0x1FU)));
        }
        if (padding)
        {
            while (out < required) output[out++] = '=';
        }
    }

    output_size = out;
    return status::ok;
}

/**
 * @brief Decodes RFC 4648 Base32 text; lowercase is accepted and is normalized.
 *
 * Invalid characters, misplaced or miscounted padding, impossible group
 * lengths and non-zero unused trailing bits are rejected.
 *
 * @param input           Source characters; may be null only when `input_size == 0`.
 * @param input_size      Source character count, including any padding.
 * @param output          Destination byte buffer.
 * @param output_capacity Destination capacity in bytes.
 * @param[out] output_size Number of bytes written, zero on failure.
 * @param allow_unpadded  When false, a final partial group must carry `=` padding.
 * @return `status::ok`, `status::full`, or `status::invalid_argument`.
 */
inline status base32_decode(CASTLE_CONST char* input,
                            size_type input_size,
                            uint8_t* output,
                            size_type output_capacity,
                            size_type& output_size,
                            bool allow_unpadded = true) CASTLE_NOEXCEPT
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
    if (input_size == 0U)
    {
        return status::ok;
    }
    // LCOV_EXCL_STOP

    size_type padding_count = 0U;
    while (padding_count < input_size && input[input_size - 1U - padding_count] == '=')
    {
        ++padding_count;
    }
    for (size_type i = 0U; i + padding_count < input_size; ++i)
    {
        if (input[i] == '=')
        {
            return status::invalid_argument;
        }
    }
    if (padding_count > 6U)
    {
        return status::invalid_argument;
    }
    if (padding_count != 0U && (input_size % 8U) != 0U)
    {
        return status::invalid_argument;
    }

    CASTLE_CONST size_type data_chars = input_size - padding_count;
    CASTLE_CONST size_type rem = data_chars % 8U;
    // LCOV_EXCL_START
    if (padding_count == 0U && !allow_unpadded && rem != 0U)
    {
        return status::invalid_argument;
    }
    if (rem == 1U || rem == 3U || rem == 6U)
    {
        return status::invalid_argument;
    }
    // LCOV_EXCL_STOP

    size_type expected_padding = 0U;
    if (rem != 0U)
    {
        expected_padding = 8U - rem;
    }
    if (padding_count != 0U && padding_count != expected_padding)
    {
        return status::invalid_argument;
    }

    // LCOV_EXCL_START
    CASTLE_CONST size_type required = (data_chars / 8U) * 5U +
                                      (rem == 0U ? 0U : rem == 2U ? 1U : rem == 4U ? 2U : rem == 5U ? 3U : 4U);
    if (required > output_capacity)
    {
        return status::full;
    }
    if (required != 0U && output == 0)
    {
        return status::invalid_argument;
    }
    // LCOV_EXCL_STOP

    size_type in = 0U;
    size_type out = 0U;
    while (in < data_chars)
    {
        CASTLE_CONST size_type chunk = (data_chars - in >= 8U) ? 8U : data_chars - in;
        uint64_t buffer = 0U;
        for (size_type i = 0U; i < chunk; ++i)
        {
            int value = detail::base32_value(static_cast<uint8_t>(input[in + i]));
            if (value < 0)
            {
                return status::invalid_argument;
            }
            buffer = (buffer << 5U) | static_cast<uint32_t>(value);
        }
        CASTLE_CONST size_type bytes = chunk == 8U ? 5U : // LCOV_EXCL_BR_LINE
                                       chunk == 2U ? 1U :
                                       chunk == 4U ? 2U :
                                       chunk == 5U ? 3U : 4U;
        if (chunk < 8U)
        {
            CASTLE_CONST size_type unused_bits = chunk * 5U - bytes * 8U;
            if (unused_bits != 0U && (buffer & ((static_cast<uint64_t>(1U) << unused_bits) - 1U)) != 0U) // LCOV_EXCL_BR_LINE
            {
                return status::invalid_argument;
            }
            buffer <<= (8U - chunk) * 5U;
        }
        for (size_type i = 0U; i < bytes; ++i)
        {
            output[out++] = static_cast<uint8_t>((buffer >> (32U - 8U * i)) & 0xFFU);
        }
        in += chunk;
    }

    output_size = out;
    return status::ok;
}

} // namespace codecs
} // namespace castle

#endif // CASTLE_EXT_CODECS_BASE32_HPP

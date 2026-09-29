// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file base64.hpp
 * @brief Fixed-buffer Base64 codec using the RFC 4648 alphabet by default.
 */
#ifndef CASTLE_EXT_CODECS_BASE64_HPP
#define CASTLE_EXT_CODECS_BASE64_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace codecs
{

/**
 * @brief Returns the exact encoded length for a Base64 input.
 * @param input_size Number of source bytes.
 * @param padding    True when `=` padding completes the final 4-character group.
 */
CASTLE_CONSTEXPR size_type base64_encoded_size(size_type input_size, bool padding = true) CASTLE_NOEXCEPT
{
    CASTLE_CONST size_type groups = input_size / 3U;
    CASTLE_CONST size_type remainder = input_size % 3U;
    if (remainder == 0U)
    {
        return groups * 4U;
    }
    return groups * 4U + (padding ? 4U : (remainder + 1U));
}

/**
 * @brief Returns the maximum decoded byte count for a Base64 character count.
 * @param input_size Number of encoded characters; padding is not subtracted.
 */
CASTLE_CONSTEXPR size_type base64_decoded_size(size_type input_size) CASTLE_NOEXCEPT
{
    return (input_size / 4U) * 3U + ((input_size % 4U) == 0U ? 0U : (input_size % 4U) - 1U);
}

namespace detail
{

/** @brief Maps a Base64 character to its 6-bit value, or -1 if invalid for the chosen alphabet. */
CASTLE_CONSTEXPR int base64_value(uint8_t c, bool url_safe) CASTLE_NOEXCEPT
{
    // LCOV_EXCL_START
    if (c >= static_cast<uint8_t>('A') && c <= static_cast<uint8_t>('Z'))
    {
        return static_cast<int>(c - 'A');
    }
    if (c >= static_cast<uint8_t>('a') && c <= static_cast<uint8_t>('z'))
    {
        return static_cast<int>(c - 'a') + 26;
    }
    if (c >= static_cast<uint8_t>('0') && c <= static_cast<uint8_t>('9'))
    {
        return static_cast<int>(c - '0') + 52;
    }
    if (c == static_cast<uint8_t>('+'))
    {
        return url_safe ? -1 : 62;
    }
    if (c == static_cast<uint8_t>('-'))
    {
        return url_safe ? 62 : -1;
    }
    if (c == static_cast<uint8_t>('/'))
    {
        return url_safe ? -1 : 63;
    }
    if (c == static_cast<uint8_t>('_'))
    {
        return url_safe ? 63 : -1;
    }
    return -1;
    // LCOV_EXCL_STOP
}

/** @brief Maps a 6-bit value (0..63) to its Base64 character in the chosen alphabet. */
CASTLE_CONSTEXPR uint8_t base64_char(uint8_t value, bool url_safe) CASTLE_NOEXCEPT
{
    if (value < 26U)
    {
        return static_cast<uint8_t>('A' + value);
    }
    if (value < 52U)
    {
        return static_cast<uint8_t>('a' + (value - 26U));
    }
    if (value < 62U)
    {
        return static_cast<uint8_t>('0' + (value - 52U));
    }
    if (value == 62U)
    {
        return url_safe ? static_cast<uint8_t>('-') : static_cast<uint8_t>('+'); // LCOV_EXCL_BR_LINE
    }
    {
        return url_safe ? static_cast<uint8_t>('_') : static_cast<uint8_t>('/'); // LCOV_EXCL_BR_LINE
    }
}

} // namespace detail

/**
 * @brief Encodes bytes as Base64 into a caller-owned character buffer.
 *
 * @param input Source byte range; may be null only when `input_size == 0`.
 * @param input_size Source byte count.
 * @param output Destination character buffer.
 * @param output_capacity Destination capacity including all encoded characters.
 * @param output_size Receives the number of encoded characters; no terminator is written.
 * @param padding When true, RFC 4648 `=` padding is emitted.
 * @param url_safe When true, `-` and `_` replace `+` and `/`.
 * @return `status::ok`, `status::full`, or `status::invalid_argument`.
 */
inline status base64_encode(CASTLE_CONST uint8_t* input,
                            size_type input_size,
                            char* output,
                            size_type output_capacity,
                            size_type& output_size,
                            bool padding = true,
                            bool url_safe = false) CASTLE_NOEXCEPT
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
    // LCOV_EXCL_STOP

    CASTLE_CONST size_type required = base64_encoded_size(input_size, padding);
    // LCOV_EXCL_START
    if (required > output_capacity)
    {
        return status::full;
    }
    if (required != 0U && output == 0)
    {
        return status::invalid_argument;
    }
    // LCOV_EXCL_STOP

    size_type input_index = 0U;
    size_type output_index = 0U;
    while (input_index + 3U <= input_size)
    {
        CASTLE_CONST uint32_t block = (static_cast<uint32_t>(input[input_index]) << 16U) |
                                      (static_cast<uint32_t>(input[input_index + 1U]) << 8U) |
                                      static_cast<uint32_t>(input[input_index + 2U]);
        output[output_index++] = static_cast<char>(detail::base64_char(static_cast<uint8_t>((block >> 18U) & 0x3FU), url_safe));
        output[output_index++] = static_cast<char>(detail::base64_char(static_cast<uint8_t>((block >> 12U) & 0x3FU), url_safe));
        output[output_index++] = static_cast<char>(detail::base64_char(static_cast<uint8_t>((block >> 6U) & 0x3FU), url_safe));
        output[output_index++] = static_cast<char>(detail::base64_char(static_cast<uint8_t>(block & 0x3FU), url_safe));
        input_index += 3U;
    }

    CASTLE_CONST size_type remaining = input_size - input_index;
    if (remaining != 0U)
    {
        CASTLE_CONST uint8_t b0 = input[input_index];
        CASTLE_CONST uint8_t b1 = (remaining == 2U) ? input[input_index + 1U] : 0U;
        CASTLE_CONST uint8_t c0 = static_cast<uint8_t>(b0 >> 2U);
        CASTLE_CONST uint8_t c1 = static_cast<uint8_t>(((b0 & 0x03U) << 4U) | (b1 >> 4U));
        output[output_index++] = static_cast<char>(detail::base64_char(c0, url_safe));
        output[output_index++] = static_cast<char>(detail::base64_char(c1, url_safe));
        if (remaining == 2U)
        {
            output[output_index++] = static_cast<char>(detail::base64_char(static_cast<uint8_t>((b1 & 0x0FU) << 2U), url_safe));
            if (padding)
            {
                output[output_index++] = '=';
            }
        }
        else if (padding)
        {
            output[output_index++] = '=';
            output[output_index++] = '=';
        }
        else
        {
            // For an unpadded one-byte tail, only the first two characters are emitted.
        }
    }

    output_size = output_index;
    return status::ok;
}

/**
 * @brief Decodes Base64 from a caller-provided character range.
 *
 * The decoder accepts canonical padded input and, by default, valid unpadded
 * final groups. Non-zero unused bits are rejected so malformed encodings do
 * not silently normalize to another byte sequence.
 *
 * @param input           Source characters; may be null only when `input_size == 0`.
 * @param input_size      Source character count, including any padding.
 * @param output          Destination byte buffer.
 * @param output_capacity Destination capacity in bytes.
 * @param[out] output_size Number of bytes written, zero on failure.
 * @param url_safe        When true, `-` and `_` are used instead of `+` and `/`.
 * @param allow_unpadded  When false, a final partial group must carry `=` padding.
 * @return `status::ok`, `status::full`, or `status::invalid_argument`.
 */
inline status base64_decode(CASTLE_CONST char* input,
                            size_type input_size,
                            uint8_t* output,
                            size_type output_capacity,
                            size_type& output_size,
                            bool url_safe = false,
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

    size_type groups = input_size / 4U;
    size_type remainder = input_size % 4U;
    size_type padded_groups = groups;
    bool has_partial = remainder != 0U;
    if (has_partial)
    {
        if (!allow_unpadded || remainder == 1U)
        {
            return status::invalid_argument;
        }
        ++padded_groups;
    }

    size_type decoded_capacity_needed = base64_decoded_size(input_size);
    // LCOV_EXCL_START
    if (input_size >= 2U && input[input_size - 1U] == '=' && input[input_size - 2U] == '=')
    {
        decoded_capacity_needed = (input_size / 4U) * 3U - 2U;
    }
    else if (input_size >= 1U && input[input_size - 1U] == '=')
    {
        decoded_capacity_needed = (input_size / 4U) * 3U - 1U;
    }
    if (decoded_capacity_needed > output_capacity)
    {
        return status::full;
    }
    if (decoded_capacity_needed != 0U && output == 0)
    {
        return status::invalid_argument;
    }
    // LCOV_EXCL_STOP

    size_type input_index = 0U;
    size_type output_index = 0U;
    for (size_type group = 0U; group < padded_groups; ++group)
    {
        CASTLE_CONST size_type remaining = input_size - input_index;
        uint8_t c0 = 0U;
        uint8_t c1 = 0U;
        uint8_t c2 = 0U;
        uint8_t c3 = 0U;
        bool p2 = false;
        bool p3 = false;

        if (remaining < 2U)
        {
            return status::invalid_argument;
        }
        c0 = static_cast<uint8_t>(input[input_index++]);
        c1 = static_cast<uint8_t>(input[input_index++]);
        int v0 = detail::base64_value(c0, url_safe);
        int v1 = detail::base64_value(c1, url_safe);
        if (v0 < 0 || v1 < 0)
        {
            return status::invalid_argument;
        }

        if (remaining > 2U)
        {
            c2 = static_cast<uint8_t>(input[input_index++]);
            p2 = c2 == static_cast<uint8_t>('=');
            if (!p2)
            {
                int v2 = detail::base64_value(c2, url_safe);
                if (v2 < 0)
                {
                    return status::invalid_argument;
                }

                if (remaining > 3U)
                {
                    c3 = static_cast<uint8_t>(input[input_index++]);
                    p3 = c3 == static_cast<uint8_t>('=');
                    if (!p3)
                    {
                        int v3 = detail::base64_value(c3, url_safe);
                        if (v3 < 0)
                        {
                            return status::invalid_argument;
                        }
                        if (output_index + 3U > output_capacity)
                        {
                            return status::full;
                        }
                        output[output_index++] = static_cast<uint8_t>((v0 << 2) | (v1 >> 4));
                        output[output_index++] = static_cast<uint8_t>((v1 << 4) | (v2 >> 2));
                        output[output_index++] = static_cast<uint8_t>((v2 << 6) | v3);
                        continue;
                    }
                }

                // LCOV_EXCL_START
                if (p3 || remaining == 3U)
                {
                    if ((v2 & 0x03) != 0)
                    {
                        return status::invalid_argument;
                    }
                    if (output_index + 2U > output_capacity)
                    {
                        return status::full;
                    }
                    output[output_index++] = static_cast<uint8_t>((v0 << 2) | (v1 >> 4));
                    output[output_index++] = static_cast<uint8_t>((v1 << 4) | (v2 >> 2));
                    if (p3)
                    {
                        if (input_index < input_size)
                        {
                            return status::invalid_argument;
                        }
                    }
                    continue;
                }
                // LCOV_EXCL_STOP
            }
            else
            {
                if (remaining != 4U || input_index >= input_size || input[input_index++] != '=') // LCOV_EXCL_BR_LINE
                {
                    return status::invalid_argument;
                }
                if ((v1 & 0x0F) != 0)
                {
                    return status::invalid_argument;
                }
                if (output_index + 1U > output_capacity)
                {
                    return status::full;
                }
                output[output_index++] = static_cast<uint8_t>((v0 << 2) | (v1 >> 4));
                continue;
            }
        }

        // LCOV_EXCL_START
        // Only possible for an unpadded final group of two characters.
        if (remaining != 2U || group + 1U != padded_groups || !has_partial || !allow_unpadded)
        {
            return status::invalid_argument;
        }
        if ((v1 & 0x0F) != 0)
        {
            return status::invalid_argument;
        }
        if (output_index + 1U > output_capacity)
        {
            return status::full;
        }
        output[output_index++] = static_cast<uint8_t>((v0 << 2) | (v1 >> 4));
        // LCOV_EXCL_STOP
    }

    output_size = output_index;
    return status::ok;
}

} // namespace codecs
} // namespace castle

#endif // CASTLE_EXT_CODECS_BASE64_HPP

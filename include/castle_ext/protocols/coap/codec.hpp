// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file codec.hpp
 * @brief RFC 7252 CoAP message encoding and decoding with bounded option/payload storage.
 */
#ifndef CASTLE_EXT_PROTOCOLS_COAP_CODEC_HPP
#define CASTLE_EXT_PROTOCOLS_COAP_CODEC_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/protocols/coap/types.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace coap
{

enum class decode_error : uint8_t
{
    none = 0U,
    malformed_header,
    unsupported_version,
    invalid_token_length,
    malformed_option,
    unknown_critical_option,
    capacity_exceeded
};

CASTLE_INLINE uint16_t read_u16(uint8_t CASTLE_CONST* p) CASTLE_NOEXCEPT
{
    return static_cast<uint16_t>((static_cast<uint16_t>(p[0U]) << 8U) | p[1U]);
}

CASTLE_INLINE void write_u16(uint8_t* p, uint16_t value) CASTLE_NOEXCEPT
{
    p[0U] = static_cast<uint8_t>(value >> 8U);
    p[1U] = static_cast<uint8_t>(value);
}

namespace detail
{
CASTLE_INLINE castle::status write_byte(uint8_t* output, size_type capacity, size_type& used,
                                        uint8_t value) CASTLE_NOEXCEPT
{
    if (used >= capacity)
    {
        return castle::status::full;
    }
    output[used++] = value;
    return castle::status::ok;
}

CASTLE_INLINE castle::status write_extended(uint8_t* output, size_type capacity, size_type& used,
                                            uint8_t nibble, uint32_t value) CASTLE_NOEXCEPT
{
    if (nibble < 13U)
    {
        return castle::status::ok;
    }
    if (nibble == 13U)
    {
        return write_byte(output, capacity, used, static_cast<uint8_t>(value - 13U));
    }
    if (nibble == 14U)
    {
        if (capacity - used < 2U)
        {
            return castle::status::full;
        }
        CASTLE_CONST uint16_t extended = static_cast<uint16_t>(value - 269U);
        write_u16(output + used, extended);
        used += 2U;
        return castle::status::ok;
    }
    return castle::status::invalid_argument;
}

CASTLE_INLINE castle::status decode_nibble(uint8_t nibble, uint8_t CASTLE_CONST* input, size_type size,
                                           size_type& used, uint32_t& value) CASTLE_NOEXCEPT
{
    if (nibble < 13U)
    {
        value = nibble;
        return castle::status::ok;
    }
    if (nibble == 13U)
    {
        if (used >= size)
        {
            return castle::status::invalid_argument;
        }
        value = 13U + input[used++];
        return castle::status::ok;
    }
    if (nibble == 14U)
    {
        if (size - used < 2U)
        {
            return castle::status::invalid_argument;
        }
        value = 269U + read_u16(input + used);
        used += 2U;
        return castle::status::ok;
    }
    return castle::status::invalid_argument;
}

CASTLE_INLINE uint8_t encode_nibble(uint32_t value) CASTLE_NOEXCEPT
{
    return value < 13U ? static_cast<uint8_t>(value) : (value < 269U ? 13U : 14U);
}
} // namespace detail

/** @brief Checks the RFC 7252 value-length constraints for implemented options. */
CASTLE_INLINE bool valid_option_length(uint16_t number, size_type length) CASTLE_NOEXCEPT
{
    switch (number)
    {
    case 1U: return length <= 8U;                         // If-Match
    case 3U: return length >= 1U && length <= 255U;      // Uri-Host
    case 4U: return length >= 1U && length <= 8U;        // ETag
    case 5U: return length == 0U;                        // If-None-Match
    case 6U: return length <= 3U;                        // Observe
    case 7U: return length <= 2U;                        // Uri-Port
    case 8U: case 11U: case 15U: case 20U:               // Location-Path/Uri-Path/Uri-Query/Location-Query
        return length <= 255U;
    case 12U: case 17U: return length <= 2U;             // Content-Format/Accept
    case 14U: return length <= 4U;                       // Max-Age
    case 23U: case 27U: return length <= 3U;             // Block2/Block1
    case 28U: case 60U: return length <= 4U;             // Size2/Size1
    case 35U: return length >= 1U && length <= 1034U;    // Proxy-Uri
    case 39U: return length >= 1U && length <= 255U;     // Proxy-Scheme
    default: return false;
    }
}

/** @brief Returns true for options whose RFC definitions permit multiple occurrences. */
CASTLE_INLINE bool is_repeatable_option(uint16_t number) CASTLE_NOEXCEPT
{
    return number == 1U || number == 8U || number == 11U || number == 15U || number == 20U;
}

/** @brief Encodes one complete message; `written` is zero on failure. */
template <typename Message>
castle::status encode_message(Message CASTLE_CONST& input, uint8_t* output, size_type capacity,
                              size_type& written) CASTLE_NOEXCEPT
{
    written = 0U;
    CASTLE_CONST uint8_t message_type_value = static_cast<uint8_t>(input.type);
    CASTLE_CONST bool empty_code = input.code_value == static_cast<uint8_t>(code::empty);
    if (output == nullptr || input.version != protocol_version || input.token_length > maximum_token_length ||
        input.option_count > Message::max_options || input.payload_size > Message::payload_capacity ||
        message_type_value > static_cast<uint8_t>(message_type::reset) ||
        (empty_code && (input.token_length != 0U || input.option_count != 0U || input.payload_size != 0U ||
                        (input.type != message_type::acknowledgement && input.type != message_type::reset))) ||
        (input.type == message_type::reset && !empty_code) ||
        (input.type == message_type::acknowledgement && (input.code_value >> 5U) == 0U && !empty_code))
    {
        return castle::status::invalid_argument;
    }
    if (capacity < 4U + input.token_length)
    {
        return castle::status::full;
    }
    output[written++] = static_cast<uint8_t>((protocol_version << 6U) |
        (static_cast<uint8_t>(input.type) << 4U) | static_cast<uint8_t>(input.token_length));
    output[written++] = input.code_value;
    write_u16(output + written, input.message_id);
    written += 2U;
    for (size_type i = 0U; i < input.token_length; ++i)
    {
        output[written++] = input.token[i];
    }

    uint16_t previous = 0U;
    for (size_type i = 0U; i < input.option_count; ++i)
    {
        CASTLE_CONST auto& option_value = input.options[i];
        if (option_value.number == 0U || option_value.number < previous || static_cast<uint32_t>(option_value.number - previous) > 65804U ||
            option_value.length > Message::option_value_capacity ||
            (is_supported_option(option_value.number) && !valid_option_length(option_value.number, option_value.length)))
        {
            written = 0U;
            return castle::status::invalid_argument;
        }
        CASTLE_CONST uint32_t delta = static_cast<uint32_t>(option_value.number - previous);
        CASTLE_CONST uint32_t length = static_cast<uint32_t>(option_value.length);
        if (length > 65804U)
        {
            written = 0U;
            return castle::status::invalid_argument;
        }
        CASTLE_CONST uint8_t delta_nibble = detail::encode_nibble(delta);
        CASTLE_CONST uint8_t length_nibble = detail::encode_nibble(length);
        castle::status result = detail::write_byte(output, capacity, written,
            static_cast<uint8_t>((delta_nibble << 4U) | length_nibble));
        if (castle::succeeded(result))
        {
            result = detail::write_extended(output, capacity, written, delta_nibble, delta);
        }
        if (castle::succeeded(result))
        {
            result = detail::write_extended(output, capacity, written, length_nibble, length);
        }
        if (!castle::succeeded(result) || length > capacity - written)
        {
            written = 0U;
            return castle::status::full;
        }
        for (size_type j = 0U; j < option_value.length; ++j)
        {
            output[written++] = option_value.value[j];
        }
        previous = option_value.number;
    }
    if (input.payload_size != 0U)
    {
        if (capacity - written < input.payload_size + 1U)
        {
            written = 0U;
            return castle::status::full;
        }
        output[written++] = 0xFFU;
        for (size_type i = 0U; i < input.payload_size; ++i)
        {
            output[written++] = input.payload[i];
        }
    }
    return castle::status::ok;
}

/** @brief Decodes one datagram. Unknown elective options are ignored; unknown critical options fail. */
template <typename Message>
castle::status decode_message(uint8_t CASTLE_CONST* input, size_type size, Message& output,
                              decode_error* error = nullptr) CASTLE_NOEXCEPT
{
    if (error != nullptr)
    {
        *error = decode_error::none;
    }
    output.clear();
    if (input == nullptr || size < 4U)
    {
        if (error != nullptr)
        {
            *error = decode_error::malformed_header;
        }
        return castle::status::invalid_argument;
    }
    CASTLE_CONST uint8_t first = input[0U];
    CASTLE_CONST uint8_t version = static_cast<uint8_t>(first >> 6U);
    CASTLE_CONST uint8_t token_length = static_cast<uint8_t>(first & 0x0FU);
    if (version != protocol_version)
    {
        if (error != nullptr)
        {
            *error = decode_error::unsupported_version;
        }
        return castle::status::invalid_argument;
    }
    if (token_length > maximum_token_length || size < 4U + token_length)
    {
        if (error != nullptr)
        {
            *error = decode_error::invalid_token_length;
        }
        return castle::status::invalid_argument;
    }
    output.version = version;
    output.type = static_cast<message_type>((first >> 4U) & 0x03U);
    output.code_value = input[1U];
    output.message_id = read_u16(input + 2U);
    output.token_length = token_length;
    for (size_type i = 0U; i < token_length; ++i)
    {
        output.token[i] = input[4U + i];
    }
    if (output.code_value == 0U)
    {
        if (token_length != 0U || size != 4U ||
            (output.type != message_type::acknowledgement && output.type != message_type::reset))
        {
            if (error != nullptr)
            {
                *error = decode_error::malformed_header;
            }
            return castle::status::invalid_argument;
        }
        return castle::status::ok;
    }
    if (output.type == message_type::reset ||
        (output.type == message_type::acknowledgement && (output.code_value >> 5U) == 0U))
    {
        if (error != nullptr)
        {
            *error = decode_error::malformed_header;
        }
        return castle::status::invalid_argument;
    }

    size_type position = 4U + token_length;
    uint32_t option_number_value = 0U;
    while (position < size)
    {
        if (input[position] == 0xFFU)
        {
            ++position;
            if (position == size)
            {
                if (error != nullptr)
                {
                    *error = decode_error::malformed_option;
                }
                return castle::status::invalid_argument;
            }
            CASTLE_CONST size_type payload_size = size - position;
            CASTLE_CONST castle::status copied = output.set_payload(input + position, payload_size);
            if (!castle::succeeded(copied))
            {
                if (error != nullptr)
                {
                    *error = decode_error::capacity_exceeded;
                }
                return copied;
            }
            position = size;
            break;
        }
        CASTLE_CONST uint8_t header = input[position++];
        CASTLE_CONST uint8_t delta_nibble = static_cast<uint8_t>(header >> 4U);
        CASTLE_CONST uint8_t length_nibble = static_cast<uint8_t>(header & 0x0FU);
        if (delta_nibble == 15U || length_nibble == 15U)
        {
            if (error != nullptr)
            {
                *error = decode_error::malformed_option;
            }
            return castle::status::invalid_argument;
        }
        uint32_t delta = 0U, length = 0U;
        castle::status result = detail::decode_nibble(delta_nibble, input, size, position, delta);
        if (castle::succeeded(result))
        {
            result = detail::decode_nibble(length_nibble, input, size, position, length);
        }
        if (!castle::succeeded(result) || delta > 65535U - option_number_value || length > size - position)
        {
            if (error != nullptr)
            {
                *error = decode_error::malformed_option;
            }
            return castle::status::invalid_argument;
        }
        option_number_value += delta;
        if (option_number_value == 0U)
        {
            if (error != nullptr)
            {
                *error = decode_error::malformed_option;
            }
            return castle::status::invalid_argument;
        }
        CASTLE_CONST uint16_t current_option = static_cast<uint16_t>(option_number_value);
        bool recognized = is_supported_option(current_option) && valid_option_length(current_option, length);
        if (recognized && !is_repeatable_option(current_option) && output.find_option(current_option) != nullptr)
        {
            recognized = false; // excess occurrence is treated as an unrecognized option by RFC 7252
        }
        if (!recognized)
        {
            if (is_critical_option(current_option))
            {
                if (error != nullptr)
                {
                    *error = decode_error::unknown_critical_option;
                }
                return castle::status::not_configured;
            }
            position += length; // unknown/elective or invalid-length elective option: safely skip it
            continue;
        }
        result = output.append_option(current_option, length == 0U ? nullptr : input + position, length);
        if (!castle::succeeded(result))
        {
            if (error != nullptr)
            {
                *error = decode_error::capacity_exceeded;
            }
            return result;
        }
        position += length;
    }
    return castle::status::ok;
}

/** @brief Decodes a zero-to-four-byte unsigned integer option value. */
CASTLE_INLINE castle::status decode_uint_option(uint8_t CASTLE_CONST* value, size_type length,
                                               uint32_t& output) CASTLE_NOEXCEPT
{
    output = 0U;
    if (length > 4U || (value == nullptr && length != 0U))
    {
        return castle::status::invalid_argument;
    }
    for (size_type i = 0U; i < length; ++i)
    {
        output = (output << 8U) | value[i];
    }
    return castle::status::ok;
}

} // namespace coap
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_COAP_CODEC_HPP

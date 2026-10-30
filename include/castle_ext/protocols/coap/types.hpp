// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file types.hpp
 * @brief Fixed-capacity CoAP message, code, option and URI-path types.
 */
#ifndef CASTLE_EXT_PROTOCOLS_COAP_TYPES_HPP
#define CASTLE_EXT_PROTOCOLS_COAP_TYPES_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace coap
{

static CASTLE_CONSTEXPR uint8_t protocol_version = 1U;
static CASTLE_CONSTEXPR uint16_t default_port = 5683U;
static CASTLE_CONSTEXPR size_type maximum_token_length = 8U;

/** @brief CoAP message type values from RFC 7252 section 3. */
enum class message_type : uint8_t
{
    confirmable = 0U,
    non_confirmable = 1U,
    acknowledgement = 2U,
    reset = 3U
};

/** @brief Request and response code octets from RFC 7252. */
enum class code : uint8_t
{
    empty = 0x00U,
    get = 0x01U,
    post = 0x02U,
    put = 0x03U,
    delete_ = 0x04U,
    created = 0x41U,
    deleted = 0x42U,
    valid = 0x43U,
    changed = 0x44U,
    content = 0x45U,
    bad_request = 0x80U,
    unauthorized = 0x81U,
    bad_option = 0x82U,
    forbidden = 0x83U,
    not_found = 0x84U,
    method_not_allowed = 0x85U,
    not_acceptable = 0x86U,
    precondition_failed = 0x8CU,
    request_entity_too_large = 0x8DU,
    unsupported_content_format = 0x8FU,
    internal_server_error = 0xA0U,
    not_implemented = 0xA1U,
    bad_gateway = 0xA2U,
    service_unavailable = 0xA3U,
    gateway_timeout = 0xA4U,
    proxying_not_supported = 0xA5U
};

/** @brief IANA CoAP option numbers supported by the message model. */
enum class option_number : uint16_t
{
    if_match = 1U,
    uri_host = 3U,
    etag = 4U,
    if_none_match = 5U,
    observe = 6U,
    uri_port = 7U,
    location_path = 8U,
    uri_path = 11U,
    content_format = 12U,
    max_age = 14U,
    uri_query = 15U,
    accept = 17U,
    location_query = 20U,
    block2 = 23U,
    block1 = 27U,
    size2 = 28U,
    proxy_uri = 35U,
    proxy_scheme = 39U,
    size1 = 60U
};

/** @brief Returns true when the code octet is one of the four core request methods. */
CASTLE_INLINE bool is_request_code(uint8_t value) CASTLE_NOEXCEPT
{
    return value >= static_cast<uint8_t>(code::get) && value <= static_cast<uint8_t>(code::delete_);
}

CASTLE_INLINE bool is_request_code(code value) CASTLE_NOEXCEPT
{
    return is_request_code(static_cast<uint8_t>(value));
}

/** @brief Returns true for a response code class (2.xx through 5.xx). */
CASTLE_INLINE bool is_response_code(uint8_t value) CASTLE_NOEXCEPT
{
    CASTLE_CONST uint8_t klass = static_cast<uint8_t>(value >> 5U);
    return klass >= 2U && klass <= 5U;
}

/** @brief Returns true when an option is critical (option number bit zero is set). */
CASTLE_INLINE bool is_critical_option(uint16_t number) CASTLE_NOEXCEPT
{
    return (number & 0x01U) != 0U;
}

/** @brief Returns true when an option is unsafe to forward (option number bit one is set). */
CASTLE_INLINE bool is_unsafe_option(uint16_t number) CASTLE_NOEXCEPT
{
    return (number & 0x02U) != 0U;
}

/** @brief RFC 7252 NoCacheKey property, using the option-number mask from Figure 11. */
CASTLE_INLINE bool is_no_cache_key_option(uint16_t number) CASTLE_NOEXCEPT
{
    return (number & 0x1EU) == 0x1CU;
}

/** @brief Tests whether an option number is understood by this implementation. */
CASTLE_INLINE bool is_supported_option(uint16_t number) CASTLE_NOEXCEPT
{
    switch (number)
    {
    case 1U: case 3U: case 4U: case 5U: case 6U: case 7U: case 8U:
    case 11U: case 12U: case 14U: case 15U: case 17U: case 20U:
    case 23U: case 27U: case 28U: case 35U: case 39U: case 60U:
        return true;
    default:
        return false;
    }
}

/** @brief One CoAP option value stored inline; value bytes are opaque to the codec. */
template <size_type ValueCapacity = 64U>
struct option
{
    static_assert(ValueCapacity > 0U, "CoAP option value capacity must be non-zero");
    uint16_t number = 0U;
    size_type length = 0U;
    uint8_t value[ValueCapacity]{};
};

/**
 * @brief Complete CoAP message stored entirely in fixed-capacity arrays.
 * @tparam MaxOptions Maximum decoded/encoded options.
 * @tparam OptionValueCapacity Maximum bytes in one option value.
 * @tparam PayloadCapacity Maximum payload bytes.
 *
 * `append_option()` inserts options in ascending option-number order, preserving the
 * relative order of duplicate option numbers as required by RFC 7252.
 */
template <size_type MaxOptions = 16U,
          size_type OptionValueCapacity = 64U,
          size_type PayloadCapacity = 1024U>
struct message
{
    static_assert(MaxOptions > 0U, "CoAP option capacity must be non-zero");
    static_assert(OptionValueCapacity > 0U, "CoAP option value capacity must be non-zero");
    static_assert(PayloadCapacity > 0U, "CoAP payload capacity must be non-zero");

    using option_type = option<OptionValueCapacity>;
    static CASTLE_CONSTEXPR size_type max_options = MaxOptions;
    static CASTLE_CONSTEXPR size_type option_value_capacity = OptionValueCapacity;
    static CASTLE_CONSTEXPR size_type payload_capacity = PayloadCapacity;

    uint8_t version = protocol_version;
    message_type type = message_type::confirmable;
    uint8_t code_value = static_cast<uint8_t>(code::empty);
    uint16_t message_id = 0U;
    uint8_t token[maximum_token_length]{};
    size_type token_length = 0U;
    option_type options[MaxOptions]{};
    size_type option_count = 0U;
    uint8_t payload[PayloadCapacity]{};
    size_type payload_size = 0U;

    void clear() CASTLE_NOEXCEPT
    {
        version = protocol_version;
        type = message_type::confirmable;
        code_value = static_cast<uint8_t>(code::empty);
        message_id = 0U;
        token_length = 0U;
        option_count = 0U;
        payload_size = 0U;
    }

    castle::status set_token(uint8_t CASTLE_CONST* bytes, size_type count) CASTLE_NOEXCEPT
    {
        if ((bytes == nullptr && count != 0U) || count > maximum_token_length)
        {
            return castle::status::invalid_argument;
        }
        for (size_type i = 0U; i < count; ++i)
        {
            token[i] = bytes[i];
        }
        token_length = count;
        return castle::status::ok;
    }

    castle::status set_payload(uint8_t CASTLE_CONST* bytes, size_type count) CASTLE_NOEXCEPT
    {
        if ((bytes == nullptr && count != 0U))
        {
            return castle::status::invalid_argument;
        }
        if (count > PayloadCapacity)
        {
            return castle::status::full;
        }
        for (size_type i = 0U; i < count; ++i)
        {
            payload[i] = bytes[i];
        }
        payload_size = count;
        return castle::status::ok;
    }

    castle::status append_option(uint16_t number, uint8_t CASTLE_CONST* bytes, size_type count) CASTLE_NOEXCEPT
    {
        if (number == 0U || (bytes == nullptr && count != 0U))
        {
            return castle::status::invalid_argument;
        }
        if (count > OptionValueCapacity)
        {
            return castle::status::out_of_range;
        }
        if (option_count >= MaxOptions)
        {
            return castle::status::full;
        }
        size_type position = option_count;
        while (position > 0U && options[position - 1U].number > number)
        {
            options[position] = options[position - 1U];
            --position;
        }
        options[position].number = number;
        options[position].length = count;
        for (size_type i = 0U; i < count; ++i)
        {
            options[position].value[i] = bytes[i];
        }
        ++option_count;
        return castle::status::ok;
    }

    castle::status append_uint_option(uint16_t number, uint32_t value) CASTLE_NOEXCEPT
    {
        uint8_t encoded[4U]{};
        size_type count = 0U;
        if (value != 0U)
        {
            uint32_t remaining = value;
            while (remaining != 0U && count < 4U)
            {
                encoded[3U - count] = static_cast<uint8_t>(remaining);
                remaining >>= 8U;
                ++count;
            }
        }
        return append_option(number, count == 0U ? nullptr : encoded + 4U - count, count);
    }

    castle::status append_string_option(uint16_t number, char CASTLE_CONST* text) CASTLE_NOEXCEPT
    {
        if (text == nullptr)
        {
            return castle::status::invalid_argument;
        }
        size_type count = 0U;
        while (text[count] != '\0' && count <= OptionValueCapacity)
        {
            ++count;
        }
        if (count > OptionValueCapacity)
        {
            return castle::status::out_of_range;
        }
        return append_option(number, reinterpret_cast<uint8_t CASTLE_CONST*>(text), count);
    }

    option_type CASTLE_CONST* find_option(uint16_t number) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < option_count; ++i)
        {
            if (options[i].number == number)
            {
                return &options[i];
            }
            if (options[i].number > number)
            {
                break;
            }
        }
        return nullptr;
    }

    option_type* find_option(uint16_t number) CASTLE_NOEXCEPT
    {
        return const_cast<option_type*>(static_cast<message CASTLE_CONST&>(*this).find_option(number));
    }
};

/** @brief Adds every slash-delimited URI path segment as a separate Uri-Path option. */
template <typename Message>
castle::status append_uri_path(Message& target, char CASTLE_CONST* path) CASTLE_NOEXCEPT
{
    if (path == nullptr)
    {
        return castle::status::invalid_argument;
    }
    size_type index = 0U;
    while (path[index] == '/')
    {
        ++index;
    }
    while (path[index] != '\0')
    {
        CASTLE_CONST size_type start = index;
        while (path[index] != '\0' && path[index] != '/')
        {
            ++index;
        }
        if (index != start)
        {
            castle::status result = target.append_option(static_cast<uint16_t>(option_number::uri_path),
                reinterpret_cast<uint8_t CASTLE_CONST*>(path + start), index - start);
            if (!castle::succeeded(result))
            {
                return result;
            }
        }
        while (path[index] == '/')
        {
            ++index;
        }
    }
    return castle::status::ok;
}

/** @brief Adds a URI query component as a Uri-Query option (without a leading '?'). */
template <typename Message>
castle::status append_uri_query(Message& target, char CASTLE_CONST* query) CASTLE_NOEXCEPT
{
    if (query == nullptr)
    {
        return castle::status::invalid_argument;
    }
    size_type start = 0U;
    size_type i = 0U;
    while (true)
    {
        if (query[i] == '&' || query[i] == '\0')
        {
            if (i != start)
            {
                castle::status result = target.append_option(static_cast<uint16_t>(option_number::uri_query),
                    reinterpret_cast<uint8_t CASTLE_CONST*>(query + start), i - start);
                if (!castle::succeeded(result))
                {
                    return result;
                }
            }
            if (query[i] == '\0')
            {
                break;
            }
            start = i + 1U;
        }
        ++i;
    }
    return castle::status::ok;
}

/** @brief Matches all Uri-Path options against a slash-delimited registered path. */
template <typename Message>
bool uri_path_matches(Message CASTLE_CONST& request, char CASTLE_CONST* path) CASTLE_NOEXCEPT
{
    if (path == nullptr)
    {
        return false;
    }
    size_type p = 0U;
    while (path[p] == '/')
    {
        ++p;
    }
    size_type option_index = 0U;
    for (size_type i = 0U; i < request.option_count; ++i)
    {
        if (request.options[i].number != static_cast<uint16_t>(option_number::uri_path))
        {
            continue;
        }
        while (path[p] == '/')
        {
            ++p;
        }
        size_type end = p;
        while (path[end] != '\0' && path[end] != '/')
        {
            ++end;
        }
        if (end - p != request.options[i].length)
        {
            return false;
        }
        for (size_type j = 0U; j < end - p; ++j)
        {
            if (static_cast<uint8_t>(path[p + j]) != request.options[i].value[j])
            {
                return false;
            }
        }
        p = end;
        ++option_index;
    }
    while (path[p] == '/')
    {
        ++p;
    }
    return path[p] == '\0' && option_index != 0U;
}

} // namespace coap
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_COAP_TYPES_HPP

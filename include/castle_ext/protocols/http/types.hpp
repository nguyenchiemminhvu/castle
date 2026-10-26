// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file types.hpp
 * @brief Shared HTTP vocabulary: versions, methods, error codes and non-owning views.
 *
 * Everything here is allocation-free and exception-free. `error_code` carries the protocol-level cause of
 * a failure, while `to_status()` folds it into CASTLE's common `castle::status`.
 *
 * @code
 * using namespace castle::protocols::http;
 *
 * string_view token = method_token(method::get);                        // "GET"
 * bool same = ascii_iequal(string_view("Content-Type"), string_view("content-type")); // true
 * castle::status result = to_status(error_code::body_limit);            // status::out_of_range
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_HTTP_TYPES_HPP
#define CASTLE_EXT_PROTOCOLS_HTTP_TYPES_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/container/string_view.hpp"
#include "castle/error/status.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace http
{

/** @brief Non-owning text view used by every HTTP interface. */
using string_view = castle::container::string_view;

/** @brief HTTP protocol version of a request or response. */
enum class version : uint8_t
{
    http_1_0 = 0U,
    http_1_1 = 1U,
    http_2 = 2U
};

/** @brief Standard request methods; `custom` takes its token from a caller-supplied string. */
enum class method : uint8_t
{
    get = 0U,
    post,
    put,
    delete_,
    head,
    options,
    patch,
    custom
};

/** @brief Protocol-level failure causes; `to_status()` maps them to `castle::status`. */
enum class error_code : uint8_t
{
    ok = 0U,
    malformed_message,
    malformed_status_line,
    invalid_header,
    header_limit,
    header_line_limit,
    body_limit,
    chunked_not_supported,
    invalid_chunk,
    invalid_frame,
    frame_size_limit,
    compression_error,
    dynamic_table_limit,
    stream_limit,
    stream_closed,
    flow_control_error,
    protocol_error,
    connection_closed,
    output_full,
    request_in_progress,
    unsupported_feature,
    goaway_received,
    stream_reset
};

/** @brief Non-owning name/value pair of one HTTP field. */
struct header_view
{
    string_view name;
    string_view value;
};

/** @brief Non-owning view of a completed response; its storage belongs to the protocol client. */
struct response_view
{
    uint16_t status_code = 0U;
    version protocol = version::http_1_1;
    header_view const* headers = nullptr;
    size_type header_count = 0U;
    string_view body;
    uint32_t stream_id = 0U;
    bool keep_alive = false;
    bool body_streamed = false;
};

/** @brief Compares two texts for equality, ignoring ASCII case. */
CASTLE_INLINE bool ascii_iequal(string_view lhs, string_view rhs) CASTLE_NOEXCEPT
{
    if (lhs.size() != rhs.size())
    {
        return false;
    }
    for (size_type i = 0U; i < lhs.size(); ++i)
    {
        char a = lhs[i];
        char b = rhs[i];
        if (a >= 'A' && a <= 'Z')
        {
            a = static_cast<char>(a + ('a' - 'A'));
        }
        if (b >= 'A' && b <= 'Z')
        {
            b = static_cast<char>(b + ('a' - 'A'));
        }
        if (a != b)
        {
            return false;
        }
    }
    return true;
}

/** @brief Tests for a visible text without uppercase ASCII letters (the HTTP/2 field-name form). */
CASTLE_INLINE bool ascii_lowercase(string_view value) CASTLE_NOEXCEPT
{
    for (size_type i = 0U; i < value.size(); ++i)
    {
        char c = value[i];
        if (c >= 'A' && c <= 'Z')
        {
            return false;
        }
        if (c <= 32 || c >= 127)
        {
            return false;
        }
    }
    return true;
}

namespace detail
{
/** @brief RFC 9110 `tchar`: the characters allowed in methods and field names. */
CASTLE_INLINE bool token_char(char c) CASTLE_NOEXCEPT
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*' ||
           c == '+' || c == '-' || c == '.' || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

/** @brief True for a non-empty text made only of `tchar` characters. */
CASTLE_INLINE bool is_token(string_view value) CASTLE_NOEXCEPT
{
    if (value.empty())
    {
        return false;
    }
    for (size_type i = 0U; i < value.size(); ++i)
    {
        if (!token_char(value[i]))
        {
            return false;
        }
    }
    return true;
}

/** @brief True for a non-empty text without whitespace or control characters (request target, authority). */
CASTLE_INLINE bool is_visible_text(string_view value) CASTLE_NOEXCEPT
{
    if (value.empty())
    {
        return false;
    }
    for (size_type i = 0U; i < value.size(); ++i)
    {
        unsigned char c = static_cast<unsigned char>(value[i]);
        if (c <= 32U || c == 127U)
        {
            return false;
        }
    }
    return true;
}

/** @brief Parses an unsigned decimal; rejects empty text, non-digits and `uint64_t` overflow. */
CASTLE_INLINE bool parse_decimal(string_view value, uint64_t& out) CASTLE_NOEXCEPT
{
    if (value.empty())
    {
        return false;
    }
    uint64_t result = 0U;
    for (size_type i = 0U; i < value.size(); ++i)
    {
        if (value[i] < '0' || value[i] > '9')
        {
            return false;
        }
        uint8_t digit = static_cast<uint8_t>(value[i] - '0');
        if (result > (static_cast<uint64_t>(-1) - digit) / 10U)
        {
            return false;
        }
        result = result * 10U + digit;
    }
    out = result;
    return true;
}
} // namespace detail

/** @brief Returns the wire token of a standard method, or an empty view for `method::custom`. */
CASTLE_INLINE string_view method_token(method value) CASTLE_NOEXCEPT
{
    switch (value)
    {
        case method::get: return string_view("GET");
        case method::post: return string_view("POST");
        case method::put: return string_view("PUT");
        case method::delete_: return string_view("DELETE");
        case method::head: return string_view("HEAD");
        case method::options: return string_view("OPTIONS");
        case method::patch: return string_view("PATCH");
        default: return string_view();
    }
}

/** @brief Maps protocol-specific errors to CASTLE's common, exception-free status type. */
CASTLE_INLINE castle::status to_status(error_code value) CASTLE_NOEXCEPT
{
    switch (value)
    {
        case error_code::ok: return castle::status::ok;
        case error_code::header_limit:
        case error_code::header_line_limit:
        case error_code::body_limit:
        case error_code::frame_size_limit:
        case error_code::dynamic_table_limit:
        case error_code::stream_limit:
        case error_code::output_full: return castle::status::out_of_range;
        case error_code::connection_closed: return castle::status::data_loss;
        case error_code::request_in_progress: return castle::status::invalid_config;
        case error_code::unsupported_feature:
        case error_code::chunked_not_supported: return castle::status::invalid_config;
        case error_code::goaway_received:
        case error_code::stream_reset:
        case error_code::stream_closed: return castle::status::data_loss;
        default: return castle::status::invalid_argument;
    }
}

} // namespace http
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_HTTP_TYPES_HPP

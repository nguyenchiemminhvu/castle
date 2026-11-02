// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file server.hpp
 * @brief Bounded HTTP/1.1 server, request model, response model and fixed-capacity router.
 *
 * The server is a synchronous, polled adapter over `castle::net::tcp_server`.
 * Every route, connection, request buffer, header, path parameter and response
 * body is stored inline. It supports HTTP/1.0 and HTTP/1.1, Content-Length and
 * chunked request bodies, sequential HTTP/1.1 pipelining, persistent connections,
 * exact/prefix/path-parameter routing, and fixed-buffer responses.
 *
 * HTTP/2, TLS, asynchronous/deferred handlers, streaming response bodies, and
 * filesystem access are deliberately not implemented by this header. `set_file`
 * copies content supplied by the application into the bounded response buffer.
 *
 * @code
 * #include "castle_ext/protocols/http/server.hpp"
 *
 * using namespace castle::protocols::http;
 *
 * int main()
 * {
 *     HttpServer server;
 *     server.register_handler(HttpMethod::GET, "/health",
 *         [](HttpRequest const&, HttpResponse& response)
 *         {
 *             response.set_body("ok");
 *         });
 *     if (server.start(castle::net::endpoint::loopback_ipv4(8080U)) != castle::status::ok)
 *     {
 *         return 1;
 *     }
 *     while (true)
 *     {
 *         server.poll(10);
 *     }
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_HTTP_SERVER_HPP
#define CASTLE_EXT_PROTOCOLS_HTTP_SERVER_HPP

#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/net/endpoint.hpp"
#include "castle_ext/net/server.hpp"
#include "castle_ext/protocols/http/headers.hpp"
#include "castle_ext/protocols/http/types.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace http
{

enum class HttpMethod : uint8_t
{
    GET = 0U, POST, PUT, DELETE, HEAD, OPTIONS, PATCH
};

CASTLE_INLINE method to_method(HttpMethod value) CASTLE_NOEXCEPT
{
    switch (value)
    {
        case HttpMethod::GET:
        {
            return method::get;
        }
        case HttpMethod::POST:
        {
            return method::post;
        }
        case HttpMethod::PUT:
        {
            return method::put;
        }
        case HttpMethod::DELETE:
        {
            return method::delete_;
        }
        case HttpMethod::HEAD:
        {
            return method::head;
        }
        case HttpMethod::OPTIONS:
        {
            return method::options;
        }
        case HttpMethod::PATCH:
        {
            return method::patch;
        }
        default:
        {
            return method::custom;
        }
    }
}

/** @brief Common HTTP response status codes understood by the response reason-phrase helper. */
enum class status_code : uint16_t
{
    continue_ = 100U,
    switching_protocols = 101U,
    ok = 200U,
    created = 201U,
    accepted = 202U,
    no_content = 204U,
    moved_permanently = 301U,
    found = 302U,
    not_modified = 304U,
    bad_request = 400U,
    unauthorized = 401U,
    forbidden = 403U,
    not_found = 404U,
    method_not_allowed = 405U,
    request_timeout = 408U,
    payload_too_large = 413U,
    uri_too_long = 414U,
    unsupported_media_type = 415U,
    expectation_failed = 417U,
    too_many_requests = 429U,
    request_header_fields_too_large = 431U,
    internal_server_error = 500U,
    not_implemented = 501U,
    bad_gateway = 502U,
    service_unavailable = 503U,
    gateway_timeout = 504U,
    http_version_not_supported = 505U
};

namespace detail
{

/** @brief Copies at most `capacity - 1` characters and NUL-terminates the destination. */
CASTLE_INLINE bool http_copy(char* destination, size_type capacity, string_view source) CASTLE_NOEXCEPT
{
    if (destination == nullptr || capacity == 0U || source.size() >= capacity)
    {
        return false;
    }
    for (size_type i = 0U; i < source.size(); ++i)
    {
        destination[i] = source[i];
    }
    destination[source.size()] = '\0';
    return true;
}

/** @brief Copies a byte range into a fixed byte buffer. */
CASTLE_INLINE void http_copy_bytes(uint8_t* destination, CASTLE_CONST uint8_t* source, size_type size) CASTLE_NOEXCEPT
{
    for (size_type i = 0U; i < size; ++i)
    {
        destination[i] = source[i];
    }
}

/** @brief Removes optional whitespace from both ends of a field value. */
CASTLE_INLINE string_view server_trim_ows(string_view value) CASTLE_NOEXCEPT
{
    size_type first = 0U;
    size_type last = value.size();
    while (first < last && (value[first] == ' ' || value[first] == '\t'))
    {
        ++first;
    }
    while (last > first && (value[last - 1U] == ' ' || value[last - 1U] == '\t'))
    {
        --last;
    }
    return value.substr(first, last - first);
}

/** @brief Tests whether a comma-separated field contains `token`, case-insensitively. */
CASTLE_INLINE bool server_contains_token(string_view value, string_view token) CASTLE_NOEXCEPT
{
    size_type offset = 0U;
    while (offset <= value.size())
    {
        size_type end = offset;
        while (end < value.size() && value[end] != ',')
        {
            ++end;
        }
        if (ascii_iequal(server_trim_ows(value.substr(offset, end - offset)), token))
        {
            return true;
        }
        if (end == value.size())
        {
            break;
        }
        offset = end + 1U;
    }
    return false;
}

/** @brief Maps a request method token to CASTLE's method enumeration. */
CASTLE_INLINE method parse_method(string_view value) CASTLE_NOEXCEPT
{
    if (value == string_view("GET"))
    {
        return method::get;
    }
    if (value == string_view("POST"))
    {
        return method::post;
    }
    if (value == string_view("PUT"))
    {
        return method::put;
    }
    if (value == string_view("DELETE"))
    {
        return method::delete_;
    }
    if (value == string_view("HEAD"))
    {
        return method::head;
    }
    if (value == string_view("OPTIONS"))
    {
        return method::options;
    }
    if (value == string_view("PATCH"))
    {
        return method::patch;
    }
    return method::custom;
}

/** @brief Parses one hexadecimal chunk size, stopping before an optional extension. */
CASTLE_INLINE bool server_parse_hex(string_view value, uint64_t& out) CASTLE_NOEXCEPT
{
    size_type end = 0U;
    while (end < value.size() && value[end] != ';')
    {
        ++end;
    }
    if (end == 0U)
    {
        return false;
    }
    uint64_t result = 0U;
    for (size_type i = 0U; i < end; ++i)
    {
        uint8_t digit = 0U;
        char c = value[i];
        if (c >= '0' && c <= '9')
        {
            digit = static_cast<uint8_t>(c - '0');
        }
        else if (c >= 'a' && c <= 'f')
        {
            digit = static_cast<uint8_t>(c - 'a' + 10);
        }
        else if (c >= 'A' && c <= 'F')
        {
            digit = static_cast<uint8_t>(c - 'A' + 10);
        }
        else
        {
            return false;
        }
        if (result > (static_cast<uint64_t>(-1) - digit) / 16U)
        {
            return false;
        }
        result = result * 16U + digit;
    }
    out = result;
    return true;
}

/** @brief Returns the standard reason phrase, or `Unknown` for an unregistered code. */
CASTLE_INLINE string_view reason_phrase(uint16_t code) CASTLE_NOEXCEPT
{
    switch (code)
    {
        case 100U: return string_view("Continue");
        case 101U: return string_view("Switching Protocols");
        case 200U: return string_view("OK");
        case 201U: return string_view("Created");
        case 202U: return string_view("Accepted");
        case 204U: return string_view("No Content");
        case 301U: return string_view("Moved Permanently");
        case 302U: return string_view("Found");
        case 304U: return string_view("Not Modified");
        case 400U: return string_view("Bad Request");
        case 401U: return string_view("Unauthorized");
        case 403U: return string_view("Forbidden");
        case 404U: return string_view("Not Found");
        case 405U: return string_view("Method Not Allowed");
        case 408U: return string_view("Request Timeout");
        case 413U: return string_view("Payload Too Large");
        case 414U: return string_view("URI Too Long");
        case 415U: return string_view("Unsupported Media Type");
        case 417U: return string_view("Expectation Failed");
        case 429U: return string_view("Too Many Requests");
        case 431U: return string_view("Request Header Fields Too Large");
        case 500U: return string_view("Internal Server Error");
        case 501U: return string_view("Not Implemented");
        case 502U: return string_view("Bad Gateway");
        case 503U: return string_view("Service Unavailable");
        case 504U: return string_view("Gateway Timeout");
        case 505U: return string_view("HTTP Version Not Supported");
        default: return string_view("Unknown");
    }
}

/** @brief Formats an unsigned integer in decimal without a temporary dynamic string. */
CASTLE_INLINE size_type format_uint(uint32_t value, char* output, size_type capacity) CASTLE_NOEXCEPT
{
    char reversed[10]{};
    size_type digits = 0U;
    do
    {
        reversed[digits++] = static_cast<char>('0' + (value % 10U));
        value /= 10U;
    }
    while (value != 0U && digits < sizeof(reversed));
    if (digits > capacity)
    {
        return 0U;
    }
    for (size_type i = 0U; i < digits; ++i)
    {
        output[i] = reversed[digits - i - 1U];
    }
    return digits;
}

/** @brief Routes a server protocol error through CASTLE's configured error callback when enabled. */
CASTLE_INLINE void report_http_error(CASTLE_CONST char* reason) CASTLE_NOEXCEPT
{
#if defined(CASTLE_LOG_ERRORS) || defined(CASTLE_USE_ASSERT_FUNCTION)
    castle::error_handler::error(CASTLE_ERROR_GENERIC(reason));
#else
    (void)reason;
#endif
}

} // namespace detail

/**
 * @brief Owning, fixed-capacity parsed HTTP request passed to route handlers.
 * @tparam MaxHeaders Maximum request header count.
 * @tparam MaxHeaderNameSize Maximum header-name length.
 * @tparam MaxHeaderValueSize Maximum header-value length.
 * @tparam MaxUriSize Maximum request-target and normalized-path length.
 * @tparam MaxBodySize Maximum decoded request body size.
 * @tparam MaxPathSegments Maximum captured `{parameter}` path segments.
 *
 * All request accessors return views into this object. A request and its views
 * are valid only during the synchronous handler invocation; copy data needed
 * after the callback returns.
 */
template <size_type MaxHeaders = 16U,
          size_type MaxHeaderNameSize = 64U,
          size_type MaxHeaderValueSize = 256U,
          size_type MaxUriSize = 256U,
          size_type MaxBodySize = 1024U,
          size_type MaxPathSegments = 8U>
class basic_http_request
{
public:
    using headers_type = basic_headers<MaxHeaders, MaxHeaderNameSize, MaxHeaderValueSize>;
    using method_type = castle::protocols::http::method;

    /** @brief Constructs an empty request. */
    basic_http_request() CASTLE_DEFAULT;

    /** @brief Clears all parsed fields and captured parameters. */
    void clear() CASTLE_NOEXCEPT
    {
        method_ = method::custom;
        version_ = version::http_1_1;
        method_size_ = 0U;
        target_size_ = 0U;
        path_size_ = 0U;
        query_offset_ = 0U;
        body_size_ = 0U;
        parameter_count_ = 0U;
        method_text_[0] = '\0';
        target_[0] = '\0';
        path_[0] = '\0';
        headers_.clear();
    }

    /** @brief Returns the recognized method enum. Unknown tokens map to `method::custom`. */
    method_type method() CASTLE_CONST CASTLE_NOEXCEPT { return method_; }
    /** @brief Alias for `method()`. */
    method_type request_method() CASTLE_CONST CASTLE_NOEXCEPT { return method_; }
    /** @brief Alias for `method()`. */
    method_type method_value() CASTLE_CONST CASTLE_NOEXCEPT { return method_; }
    /** @brief Returns the original method token, including an unknown extension method. */
    string_view method_text() CASTLE_CONST CASTLE_NOEXCEPT { return string_view(method_text_, method_size_); }
    /** @brief Returns HTTP/1.0 or HTTP/1.1. */
    version protocol_version() CASTLE_CONST CASTLE_NOEXCEPT { return version_; }
    /** @brief Returns the original request target, including any query string. */
    string_view target() CASTLE_CONST CASTLE_NOEXCEPT { return string_view(target_, target_size_); }
    /** @brief Returns the normalized path without the query string. */
    string_view path() CASTLE_CONST CASTLE_NOEXCEPT { return string_view(path_, path_size_); }
    /** @brief Returns the query string without the leading `?`, or an empty view. */
    string_view query() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return query_offset_ == 0U ? string_view() : string_view(target_ + query_offset_, target_size_ - query_offset_);
    }
    /** @brief Provides read-only access to parsed headers. */
    headers_type CASTLE_CONST& headers() CASTLE_CONST CASTLE_NOEXCEPT { return headers_; }
    /** @brief Returns a header value using case-insensitive name matching. */
    string_view header(string_view name) CASTLE_CONST CASTLE_NOEXCEPT { return headers_.get(name); }
    /** @brief Returns the decoded request body as a byte-preserving view. */
    string_view body() CASTLE_CONST CASTLE_NOEXCEPT { return string_view(reinterpret_cast<CASTLE_CONST char*>(body_), body_size_); }
    /** @brief Returns a pointer to the decoded request body. */
    CASTLE_CONST uint8_t* body_data() CASTLE_CONST CASTLE_NOEXCEPT { return body_; }
    /** @brief Returns the decoded body length. */
    size_type body_size() CASTLE_CONST CASTLE_NOEXCEPT { return body_size_; }

    /** @brief Looks up a decoded path-segment parameter captured from a `{name}` route segment. */
    string_view path_param(string_view name) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < parameter_count_; ++i)
        {
            if (ascii_iequal(string_view(parameters_[i].name, parameters_[i].name_size), name))
            {
                return string_view(parameters_[i].value, parameters_[i].value_size);
            }
        }
        return string_view();
    }

    /** @brief Finds a query parameter; percent-encoded octets are returned without decoding. */
    string_view query_param(string_view name) CASTLE_CONST CASTLE_NOEXCEPT
    {
        string_view values = query();
        size_type offset = 0U;
        while (offset <= values.size() && !values.empty())
        {
            size_type end = offset;
            while (end < values.size() && values[end] != '&' && values[end] != '#')
            {
                ++end;
            }
            string_view item = values.substr(offset, end - offset);
            size_type equal = item.find('=');
            string_view key = equal == string_view::npos ? item : item.substr(0U, equal);
            if (key == name)
            {
                return equal == string_view::npos ? string_view() : item.substr(equal + 1U);
            }
            if (end == values.size() || values[end] == '#')
            {
                break;
            }
            offset = end + 1U;
        }
        return string_view();
    }

private:
    template <size_type, size_type, size_type, size_type, size_type, size_type, size_type,
              size_type, size_type, size_type, size_type, size_type, size_type, size_type>
    friend class basic_http_server;

    struct parameter
    {
        char name[MaxUriSize + 1U]{};
        char value[MaxUriSize + 1U]{};
        size_type name_size = 0U;
        size_type value_size = 0U;
    };

    bool assign_method(string_view value) CASTLE_NOEXCEPT
    {
        if (value.size() >= sizeof(method_text_))
        {
            return false;
        }
        detail::http_copy(method_text_, sizeof(method_text_), value);
        method_size_ = value.size();
        method_ = detail::parse_method(value);
        return true;
    }

    bool assign_target(string_view value) CASTLE_NOEXCEPT
    {
        if (value.empty() || value.size() > MaxUriSize || !detail::http_copy(target_, sizeof(target_), value))
        {
            return false;
        }
        target_size_ = value.size();
        string_view target_value(target_, target_size_);
        if (target_value == string_view("*"))
        {
            detail::http_copy(path_, sizeof(path_), target_value);
            path_size_ = 1U;
            query_offset_ = 0U;
            return true;
        }
        string_view normalized = target_value;
        bool absolute_http = target_value.size() >= 7U &&
                             ascii_iequal(target_value.substr(0U, 7U), string_view("http://"));
        bool absolute_https = target_value.size() >= 8U &&
                              ascii_iequal(target_value.substr(0U, 8U), string_view("https://"));
        if (absolute_http || absolute_https)
        {
            size_type scheme = absolute_https ? 8U : 7U;
            size_type authority_end = scheme;
            while (authority_end < target_value.size() && target_value[authority_end] != '/' &&
                   target_value[authority_end] != '?')
            {
                ++authority_end;
            }
            if (authority_end == scheme)
            {
                return false; // An absolute URI must contain a non-empty authority.
            }
            if (authority_end == target_value.size())
            {
                normalized = string_view("/");
            }
            else if (target_value[authority_end] == '?')
            {
                path_[0] = '/';
                path_[1] = '\0';
                path_size_ = 1U;
                query_offset_ = authority_end + 1U;
                return true;
            }
            else
            {
                normalized = target_value.substr(authority_end);
            }
        }
        if (normalized.empty() || normalized[0U] != '/')
        {
            return false;
        }
        size_type query = normalized.find('?');
        size_type path_length = query == string_view::npos ? normalized.size() : query;
        if (!detail::http_copy(path_, sizeof(path_), normalized.substr(0U, path_length)))
        {
            return false;
        }
        path_size_ = path_length;
        query_offset_ = query == string_view::npos ? 0U : target_size_ - normalized.size() + query + 1U;
        return true;
    }

    bool add_parameter(string_view name, string_view value) CASTLE_NOEXCEPT
    {
        if (parameter_count_ >= MaxPathSegments || name.empty() || name.size() > MaxUriSize ||
            value.size() > MaxUriSize)
        {
            return false;
        }
        parameter& item = parameters_[parameter_count_];
        detail::http_copy(item.name, sizeof(item.name), name);
        detail::http_copy(item.value, sizeof(item.value), value);
        item.name_size = name.size();
        item.value_size = value.size();
        ++parameter_count_;
        return true;
    }

    method_type method_ = castle::protocols::http::method::custom;
    version version_ = version::http_1_1;
    char method_text_[16U]{};
    char target_[MaxUriSize + 1U]{};
    char path_[MaxUriSize + 1U]{};
    size_type method_size_ = 0U;
    size_type target_size_ = 0U;
    size_type path_size_ = 0U;
    size_type query_offset_ = 0U;
    headers_type headers_;
    uint8_t body_[MaxBodySize == 0U ? 1U : MaxBodySize]{};
    size_type body_size_ = 0U;
    parameter parameters_[MaxPathSegments == 0U ? 1U : MaxPathSegments]{};
    size_type parameter_count_ = 0U;
};

/** @brief Default request type for applications that use the default server capacities. */
using HttpRequest = basic_http_request<>;

/**
 * @brief Fixed-capacity HTTP response built by a synchronous request handler.
 * @tparam MaxHeaders Maximum response header count.
 * @tparam MaxHeaderNameSize Maximum response header-name length.
 * @tparam MaxHeaderValueSize Maximum response header-value length.
 * @tparam MaxBodySize Maximum copied response body size.
 */
template <size_type MaxHeaders = 16U,
          size_type MaxHeaderNameSize = 64U,
          size_type MaxHeaderValueSize = 256U,
          size_type MaxBodySize = 1024U>
class basic_http_response
{
public:
    using headers_type = basic_headers<MaxHeaders, MaxHeaderNameSize, MaxHeaderValueSize>;

    /** @brief Constructs a 200 OK response with an empty body. */
    basic_http_response() CASTLE_DEFAULT;

    /** @brief Resets status, headers and body to the initial 200 OK state. */
    void clear() CASTLE_NOEXCEPT
    {
        status_ = 200U;
        body_size_ = 0U;
        headers_.clear();
    }

    /** @brief Sets any three-digit HTTP status, including application-defined extension codes. */
    castle::status set_status(uint16_t value) CASTLE_NOEXCEPT
    {
        if (value < 100U || value > 999U)
        {
            return castle::status::invalid_argument;
        }
        status_ = value;
        return castle::status::ok;
    }

    /** @brief Sets a predefined status code. */
    castle::status set_status(status_code value) CASTLE_NOEXCEPT
    {
        return set_status(static_cast<uint16_t>(value));
    }

    /** @brief Returns the response status code. */
    uint16_t status() CASTLE_CONST CASTLE_NOEXCEPT { return status_; }
    /** @brief Provides mutable response-header access. */
    headers_type& headers() CASTLE_NOEXCEPT { return headers_; }
    /** @brief Provides read-only response-header access. */
    headers_type CASTLE_CONST& headers() CASTLE_CONST CASTLE_NOEXCEPT { return headers_; }
    /** @brief Adds or replaces a validated response header. */
    castle::status set_header(string_view name, string_view value) CASTLE_NOEXCEPT
    {
        return headers_.set(name, value);
    }
    /** @brief Copies a body into the fixed response buffer. */
    castle::status set_body(string_view value) CASTLE_NOEXCEPT
    {
        if (value.size() > MaxBodySize)
        {
            return castle::status::out_of_range;
        }
        for (size_type i = 0U; i < value.size(); ++i)
        {
            body_[i] = static_cast<uint8_t>(value[i]);
        }
        body_size_ = value.size();
        return castle::status::ok;
    }
    /** @brief Sets an application/json response body and media type. */
    castle::status set_json(string_view value) CASTLE_NOEXCEPT
    {
        castle::status result = headers_.set(string_view("Content-Type"), string_view("application/json"));
        return castle::succeeded(result) ? set_body(value) : result;
    }
    /** @brief Copies static resource bytes and sets their media type; no filesystem is accessed. */
    castle::status set_file(string_view media_type, string_view content) CASTLE_NOEXCEPT
    {
        castle::status result = headers_.set(string_view("Content-Type"), media_type);
        return castle::succeeded(result) ? set_body(content) : result;
    }
    /** @brief Returns the response body as a byte-preserving view. */
    string_view body() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return string_view(reinterpret_cast<CASTLE_CONST char*>(body_), body_size_);
    }
    /** @brief Returns the response body byte pointer. */
    CASTLE_CONST uint8_t* body_data() CASTLE_CONST CASTLE_NOEXCEPT { return body_; }
    /** @brief Returns the current response body length. */
    size_type body_size() CASTLE_CONST CASTLE_NOEXCEPT { return body_size_; }

private:
    template <size_type, size_type, size_type, size_type, size_type, size_type, size_type,
              size_type, size_type, size_type, size_type, size_type, size_type, size_type>
    friend class basic_http_server;

    uint16_t status_ = 200U;
    headers_type headers_;
    uint8_t body_[MaxBodySize == 0U ? 1U : MaxBodySize]{};
    size_type body_size_ = 0U;
};

/** @brief Default response type for applications that use default server capacities. */
using HttpResponse = basic_http_response<>;

/**
 * @brief Zero-allocation, polled HTTP/1.1 server over CASTLE's fixed-capacity TCP server.
 *
 * @tparam MaxConnections Maximum simultaneously accepted TCP clients.
 * @tparam ReceiveBufferSize Maximum bytes received in one TCP operation.
 * @tparam MaxRequestsPerConnection Maximum requests served before a persistent connection is closed.
 * @tparam MaxRoutes Maximum registered exact/prefix routes.
 * @tparam MaxHeaders Maximum request/response header count.
 * @tparam MaxHeaderNameSize Maximum header-name length.
 * @tparam MaxHeaderValueSize Maximum header-value length.
 * @tparam MaxUriSize Maximum request-target and normalized path length.
 * @tparam MaxBodySize Maximum decoded request and copied response body size.
 * @tparam MaxRequestSize Maximum buffered raw HTTP request bytes, including headers and chunk framing.
 * @tparam MaxHeaderBytes Maximum raw request header section size.
 * @tparam MaxRoutePathSize Maximum stored route-pattern length.
 * @tparam MaxPathSegments Maximum captured path parameters per request.
 * @tparam HandlerStorageSize Inline storage reserved for each handler.
 *
 * The object is not internally synchronized. Call `start()`, `poll()` and
 * `stop()` from one thread; register routes before `start()`. Handlers run in
 * that same thread and complete synchronously. Each connection has one active
 * request parser; pipelined requests are processed sequentially in wire order.
 */
template <size_type MaxConnections = 4U,
          size_type ReceiveBufferSize = 512U,
          size_type MaxRequestsPerConnection = 16U,
          size_type MaxRoutes = 16U,
          size_type MaxHeaders = 16U,
          size_type MaxHeaderNameSize = 64U,
          size_type MaxHeaderValueSize = 256U,
          size_type MaxUriSize = 256U,
          size_type MaxBodySize = 1024U,
          size_type MaxRequestSize = 4096U,
          size_type MaxHeaderBytes = 2048U,
          size_type MaxRoutePathSize = 128U,
          size_type MaxPathSegments = 8U,
          size_type HandlerStorageSize = castle::inplace_storage_reserved>
class basic_http_server
{
    static_assert(MaxConnections > 0U, "HTTP server requires at least one connection slot");
    static_assert(MaxRequestsPerConnection > 0U, "HTTP keep-alive request limit must be non-zero");
    static_assert(MaxRoutes > 0U, "HTTP server requires at least one route slot");
    static_assert(MaxRequestSize > 0U, "HTTP request buffer must be non-zero");
    static_assert(MaxBodySize > 0U, "HTTP body capacity must be non-zero");

public:
    using request_type = basic_http_request<MaxHeaders, MaxHeaderNameSize, MaxHeaderValueSize,
                                            MaxUriSize, MaxBodySize, MaxPathSegments>;
    using response_type = basic_http_response<MaxHeaders, MaxHeaderNameSize, MaxHeaderValueSize, MaxBodySize>;
    using headers_type = basic_headers<MaxHeaders, MaxHeaderNameSize, MaxHeaderValueSize>;
    using transport_type = castle::net::tcp_server<MaxConnections, ReceiveBufferSize, HandlerStorageSize>;
    using client_handle = typename transport_type::client_handle;
    using handler_type = castle::callbacks::function<void(request_type CASTLE_CONST&, response_type&), HandlerStorageSize>;

    /** @brief Constructs an unopened server with no routes or callbacks. */
    basic_http_server() CASTLE_DEFAULT;
    basic_http_server(CASTLE_CONST basic_http_server&) CASTLE_DELETE;
    basic_http_server& operator=(CASTLE_CONST basic_http_server&) CASTLE_DELETE;

    /** @brief Destroys the server and closes all owned TCP connections. */
    ~basic_http_server() CASTLE_DEFAULT;

    /** @brief Registers a route that accepts any recognized HTTP method. */
    template <typename Handler>
    basic_http_server& register_handler(string_view path, Handler&& handler)
    {
        return add_route(false, method::custom, false, path, CASTLE_FORWARD<Handler>(handler));
    }

    /** @brief Registers a method-specific exact/path-parameter route. */
    template <typename Handler>
    basic_http_server& register_handler(method verb, string_view path, Handler&& handler)
    {
        return add_route(true, verb, false, path, CASTLE_FORWARD<Handler>(handler));
    }

    /** @brief Registers a method-specific route using the Drogon-style `HttpMethod` spelling. */
    template <typename Handler>
    basic_http_server& register_handler(HttpMethod verb, string_view path, Handler&& handler)
    {
        return register_handler(to_method(verb), path, CASTLE_FORWARD<Handler>(handler));
    }

    /** @brief Registers a prefix route that matches the path itself and descendant paths. */
    template <typename Handler>
    basic_http_server& register_prefix_handler(string_view prefix, Handler&& handler)
    {
        return add_route(false, method::custom, true, prefix, CASTLE_FORWARD<Handler>(handler));
    }

    /** @brief Registers a method-specific prefix route. */
    template <typename Handler>
    basic_http_server& register_prefix_handler(method verb, string_view prefix, Handler&& handler)
    {
        return add_route(true, verb, true, prefix, CASTLE_FORWARD<Handler>(handler));
    }

    /** @brief Registers a method-specific prefix route using `HttpMethod`. */
    template <typename Handler>
    basic_http_server& register_prefix_handler(HttpMethod verb, string_view prefix, Handler&& handler)
    {
        return register_prefix_handler(to_method(verb), prefix, CASTLE_FORWARD<Handler>(handler));
    }

    /** @brief Sets an optional fallback handler used when no route matches. */
    template <typename Handler>
    basic_http_server& set_not_found_handler(Handler&& handler)
    {
        handler_type next(CASTLE_FORWARD<Handler>(handler));
        registration_status_ = next ? castle::status::ok : castle::status::invalid_callback;
        if (castle::succeeded(registration_status_))
        {
            not_found_handler_ = CASTLE_MOVE(next);
        }
        return *this;
    }

    /** @brief Clears every route and the optional fallback handler. Register routes before start. */
    void clear_routes() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxRoutes; ++i)
        {
            routes_[i].handler = nullptr;
            routes_[i].used = false;
        }
        route_count_ = 0U;
        not_found_handler_ = nullptr;
    }

    /** @brief Returns the result of the most recent route/fallback registration. */
    castle::status last_registration_status() CASTLE_CONST CASTLE_NOEXCEPT { return registration_status_; }
    /** @brief Returns the current number of registered routes. */
    size_type route_count() CASTLE_CONST CASTLE_NOEXCEPT { return route_count_; }
    /** @brief Returns true while the TCP listener is open. */
    bool is_running() CASTLE_CONST CASTLE_NOEXCEPT { return started_; }
    /** @brief Returns the number of active accepted client connections. */
    size_type connection_count() CASTLE_CONST CASTLE_NOEXCEPT { return transport_.client_count(); }
    /** @brief Returns the bound local endpoint, including the chosen ephemeral port. */
    castle::status local_endpoint(castle::net::endpoint& out, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (!started_)
        {
            return castle::status::not_configured;
        }
        return transport_.socket().local_endpoint(out, system_error);
    }

    /**
     * @brief Opens the TCP listener.
     * @param local Local numeric endpoint; defaults to 0.0.0.0:80.
     * @param backlog Native TCP accept backlog.
     * @return `ok`, configuration failure, or a native socket error.
     * @note Accepted sockets are blocking so fixed-buffer response writes can use `send_all()`.
     */
    castle::status start(castle::net::endpoint CASTLE_CONST& local = castle::net::endpoint::any_ipv4(80U),
                         int backlog = 8, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (started_)
        {
            return castle::status::invalid_config;
        }
        castle::status result = transport_.set_default_callback(
            [this](client_handle handle, castle::net::endpoint CASTLE_CONST&, uint8_t CASTLE_CONST* data, size_type size)
            {
                on_receive(handle, data, size);
            });
        if (!castle::succeeded(result))
        {
            return result;
        }
        result = transport_.open(local, backlog, false, system_error);
        if (castle::succeeded(result))
        {
            started_ = true;
            for (size_type i = 0U; i < MaxConnections; ++i)
            {
                reset_connection(connections_[i]);
            }
        }
        else
        {
            transport_.clear_default_callback();
        }
        return result;
    }

    /** @brief Closes the listener and every accepted peer. */
    castle::status stop(int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (!started_)
        {
            return castle::status::not_configured;
        }
        started_ = false;
        castle::status result = transport_.close(system_error);
        transport_.clear_default_callback();
        for (size_type i = 0U; i < MaxConnections; ++i)
        {
            reset_connection(connections_[i]);
        }
        return result;
    }

    /**
     * @brief Accepts pending clients and polls each active connection once.
     * @param timeout_ms Maximum wait for a new connection when no clients are active; zero polls.
     * @return `ok` when accept/read work made progress, `empty` when nothing was ready, or an underlying error.
     * @note When any client is active, the accept wait is forced to zero so its data is not starved by a listener wait.
     */
    castle::status poll(int timeout_ms = 0, int* system_error = nullptr) CASTLE_NOEXCEPT
    {
        if (!started_)
        {
            return castle::status::not_configured;
        }
        bool progressed = false;
        client_handle accepted;
        CASTLE_CONST int accept_timeout = transport_.client_count() == 0U ? timeout_ms : 0;
        castle::status result = transport_.accept_client(accepted, accept_timeout, system_error);
        if (castle::succeeded(result))
        {
            connection& item = connections_[accepted.index];
            reset_connection(item);
            item.used = true;
            item.handle = accepted;
            progressed = true;
            // Drain already queued accepts without waiting.
            for (size_type i = 1U; i < MaxConnections; ++i)
            {
                client_handle more;
                castle::status extra = transport_.accept_client(more, 0, system_error);
                if (!castle::succeeded(extra))
                {
                    break;
                }
                connection& slot = connections_[more.index];
                reset_connection(slot);
                slot.used = true;
                slot.handle = more;
                progressed = true;
            }
        }
        else if (result != castle::status::empty && result != castle::status::full && result != castle::status::time_out)
        {
            return result;
        }

        for (size_type i = 0U; i < MaxConnections; ++i)
        {
            client_handle handle = transport_.client_at(i);
            if (!handle.is_valid())
            {
                if (connections_[i].used)
                {
                    reset_connection(connections_[i]);
                }
                continue;
            }
            if (!connections_[i].used || connections_[i].handle.generation != handle.generation)
            {
                reset_connection(connections_[i]);
                connections_[i].used = true;
                connections_[i].handle = handle;
            }
            result = transport_.process_client(handle, 0, system_error);
            if (castle::succeeded(result))
            {
                progressed = true;
            }
            else if (result != castle::status::empty && result != castle::status::time_out &&
                     result != castle::status::not_configured && result != castle::status::not_found)
            {
                detail::report_http_error("HTTP TCP receive failure");
                if (transport_.client_at(i).is_valid())
                {
                    static_cast<void>(transport_.close_client(handle, nullptr));
                }
            }
        }
        return progressed ? castle::status::ok : castle::status::empty;
    }

    /** @brief Returns the underlying fixed-capacity TCP server for advanced socket integration. */
    transport_type& transport() CASTLE_NOEXCEPT { return transport_; }

private:
    enum class parse_state : uint8_t
    {
        incomplete,
        complete,
        malformed,
        too_large
    };

    struct route_record
    {
        char path[MaxRoutePathSize + 1U]{};
        size_type path_size = 0U;
        method verb = method::custom;
        bool method_filtered = false;
        bool prefix = false;
        bool used = false;
        handler_type handler;
    };

    struct connection
    {
        client_handle handle;
        request_type request;
        char input[MaxRequestSize]{};
        size_type input_size = 0U;
        size_type body_offset = 0U;
        size_type content_length = 0U;
        size_type request_count = 0U;
        bool used = false;
        bool headers_parsed = false;
        bool chunked = false;
        bool keep_alive = true;
    };

    template <typename Handler>
    basic_http_server& add_route(bool method_filtered, method verb, bool prefix,
                                 string_view path, Handler&& handler)
    {
        handler_type next(CASTLE_FORWARD<Handler>(handler));
        if (!next)
        {
            registration_status_ = castle::status::invalid_callback;
            return *this;
        }
        if (path.empty() || path.size() > MaxRoutePathSize || path.size() > MaxUriSize ||
            path[0U] != '/' || !valid_route_pattern(path))
        {
            registration_status_ = castle::status::invalid_argument;
            return *this;
        }
        if (route_count_ == MaxRoutes)
        {
            registration_status_ = castle::status::full;
            return *this;
        }
        route_record& route = routes_[route_count_];
        static_cast<void>(detail::http_copy(route.path, sizeof(route.path), path));
        route.path_size = path.size();
        route.verb = verb;
        route.method_filtered = method_filtered;
        route.prefix = prefix;
        route.used = true;
        route.handler = CASTLE_MOVE(next);
        ++route_count_;
        registration_status_ = castle::status::ok;
        return *this;
    }

    static bool valid_route_pattern(string_view path) CASTLE_NOEXCEPT
    {
        size_type parameter_count = 0U;
        for (size_type i = 0U; i < path.size(); ++i)
        {
            if (path[i] == '{')
            {
                size_type close = path.find('}', i + 1U);
                string_view name = close == string_view::npos || close <= i + 1U ? string_view() :
                                   path.substr(i + 1U, close - i - 1U);
                if (name.empty() || !detail::is_token(name) || ++parameter_count > MaxPathSegments)
                {
                    return false;
                }
                i = close;
            }
            else if (path[i] == '}')
            {
                return false;
            }
        }
        return true;
    }

    static void reset_connection(connection& item) CASTLE_NOEXCEPT
    {
        item.request.clear();
        item.input_size = 0U;
        item.body_offset = 0U;
        item.content_length = 0U;
        item.request_count = 0U;
        item.headers_parsed = false;
        item.chunked = false;
        item.keep_alive = true;
        item.used = false;
        item.handle = client_handle();
    }

    connection* find_connection(client_handle handle) CASTLE_NOEXCEPT
    {
        if (!handle.is_valid() || handle.index >= MaxConnections)
        {
            return nullptr;
        }
        connection& item = connections_[handle.index];
        return item.used && item.handle.generation == handle.generation ? &item : nullptr;
    }

    void on_receive(client_handle handle, uint8_t CASTLE_CONST* data, size_type size) CASTLE_NOEXCEPT
    {
        connection* item = find_connection(handle);
        if (item == nullptr || data == nullptr || size == 0U)
        {
            return;
        }
        if (size > MaxRequestSize - item->input_size)
        {
            send_error(handle, 413U, "Request buffer limit exceeded");
            return;
        }
        for (size_type i = 0U; i < size; ++i)
        {
            item->input[item->input_size + i] = static_cast<char>(data[i]);
        }
        item->input_size += size;
        process_buffer(handle, *item);
    }

    void process_buffer(client_handle handle, connection& item) CASTLE_NOEXCEPT
    {
        while (item.used)
        {
            if (!item.headers_parsed)
            {
                size_type marker = find_header_end(item.input, item.input_size);
                if (marker == string_view::npos)
                {
                    size_type first_line = find_crlf(item.input, item.input_size, 0U);
                    if ((first_line == string_view::npos && item.input_size > MaxUriSize + 32U) ||
                        item.input_size > MaxHeaderBytes)
                    {
                        send_error(handle, first_line == string_view::npos ? 414U : 431U,
                                   "Request line or header section exceeds configured capacity");
                    }
                    return;
                }
                if (marker + 4U > MaxHeaderBytes)
                {
                    send_error(handle, 431U, "Request header section exceeds configured capacity");
                    return;
                }
                uint16_t error_status = 400U;
                parse_state parsed = parse_headers(item, marker, error_status);
                if (parsed == parse_state::too_large)
                {
                    send_error(handle, error_status, "Request header or URI exceeds configured capacity");
                    return;
                }
                if (parsed != parse_state::complete)
                {
                    send_error(handle, error_status, "Malformed HTTP request headers");
                    return;
                }
                item.headers_parsed = true;
                item.body_offset = marker + 4U;
                if (ascii_iequal(item.request.header(string_view("Expect")), string_view("100-continue")))
                {
                    CASTLE_CONST char interim[] = "HTTP/1.1 100 Continue\r\n\r\n";
                    size_type sent = 0U;
                    castle::status interim_status = transport_.send_all(handle, interim, sizeof(interim) - 1U, sent, nullptr);
                    if (!castle::succeeded(interim_status))
                    {
                        close_connection(handle);
                        return;
                    }
                }
            }

            size_type consumed = item.body_offset;
            if (item.chunked)
            {
                uint16_t body_error_status = 413U;
                parse_state body_state = decode_chunked(item, consumed, body_error_status);
                if (body_state == parse_state::incomplete)
                {
                    return;
                }
                if (body_state == parse_state::too_large)
                {
                    send_error(handle, body_error_status, "Chunked body or trailer section exceeds configured capacity");
                    return;
                }
                if (body_state != parse_state::complete)
                {
                    send_error(handle, 400U, "Malformed chunked request body");
                    return;
                }
            }
            else
            {
                if (item.content_length > MaxBodySize)
                {
                    send_error(handle, 413U, "Content-Length exceeds configured body capacity");
                    return;
                }
                if (item.content_length > item.input_size - item.body_offset)
                {
                    return;
                }
                item.request.body_size_ = item.content_length;
                detail::http_copy_bytes(item.request.body_,
                    reinterpret_cast<uint8_t CASTLE_CONST*>(item.input + item.body_offset), item.content_length);
                consumed = item.body_offset + item.content_length;
            }

            response_scratch_.clear();
            dispatch_request(item.request, response_scratch_);
            ++item.request_count;
            bool keep_alive = item.keep_alive && item.request_count < MaxRequestsPerConnection;
            if (!keep_alive)
            {
                static_cast<void>(response_scratch_.set_header(string_view("Connection"), string_view("close")));
            }
            else
            {
                static_cast<void>(response_scratch_.set_header(string_view("Connection"), string_view("keep-alive")));
            }
            castle::status sent = send_response(handle, item.request, response_scratch_, keep_alive);
            if (!castle::succeeded(sent))
            {
                detail::report_http_error("HTTP response write failed");
                close_connection(handle);
                return;
            }
            if (!keep_alive)
            {
                close_connection(handle);
                return;
            }
            if (consumed > item.input_size)
            {
                send_error(handle, 400U, "Invalid request framing");
                return;
            }
            remove_prefix(item, consumed);
            item.request.clear();
            item.body_offset = 0U;
            item.content_length = 0U;
            item.headers_parsed = false;
            item.chunked = false;
            item.keep_alive = true;
            if (item.input_size == 0U)
            {
                return;
            }
        }
    }

    static size_type find_crlf(CASTLE_CONST char* data, size_type size, size_type start) CASTLE_NOEXCEPT
    {
        if (size < 2U || start >= size - 1U)
        {
            return string_view::npos;
        }
        for (size_type i = start; i + 1U < size; ++i)
        {
            if (data[i] == '\r' && data[i + 1U] == '\n')
            {
                return i;
            }
        }
        return string_view::npos;
    }

    static size_type find_header_end(CASTLE_CONST char* data, size_type size) CASTLE_NOEXCEPT
    {
        if (size < 4U)
        {
            return string_view::npos;
        }
        for (size_type i = 0U; i + 3U < size; ++i)
        {
            if (data[i] == '\r' && data[i + 1U] == '\n' && data[i + 2U] == '\r' && data[i + 3U] == '\n')
            {
                return i;
            }
        }
        return string_view::npos;
    }

    parse_state parse_headers(connection& item, size_type marker, uint16_t& error_status) CASTLE_NOEXCEPT
    {
        item.request.clear();
        string_view block(item.input, marker);
        size_type line_end = block.find(string_view("\r\n"));
        if (line_end == string_view::npos)
        {
            line_end = block.size();
        }
        string_view request_line = block.substr(0U, line_end);
        size_type first_space = request_line.find(' ');
        size_type second_space = first_space == string_view::npos ? string_view::npos : request_line.find(' ', first_space + 1U);
        if (first_space == string_view::npos || second_space == string_view::npos || first_space == 0U ||
            second_space <= first_space + 1U || request_line.find(' ', second_space + 1U) != string_view::npos)
        {
            return parse_state::malformed;
        }
        string_view verb = request_line.substr(0U, first_space);
        string_view target = request_line.substr(first_space + 1U, second_space - first_space - 1U);
        string_view protocol = request_line.substr(second_space + 1U);
        if (!detail::is_token(verb) || !detail::is_visible_text(target))
        {
            return parse_state::malformed;
        }
        if (target.size() > MaxUriSize)
        {
            error_status = 414U;
            return parse_state::too_large;
        }
        if (protocol == string_view("HTTP/1.1"))
        {
            item.request.version_ = version::http_1_1;
        }
        else if (protocol == string_view("HTTP/1.0"))
        {
            item.request.version_ = version::http_1_0;
        }
        else
        {
            error_status = 505U;
            return parse_state::malformed;
        }
        if (target.find('#') != string_view::npos || !item.request.assign_method(verb) || !item.request.assign_target(target))
        {
            return parse_state::malformed;
        }

        bool has_content_length = false;
        bool has_transfer_encoding = false;
        bool has_host = false;
        size_type cursor = line_end + 2U;
        size_type header_count = 0U;
        while (cursor < block.size())
        {
            size_type end = block.find(string_view("\r\n"), cursor);
            if (end == string_view::npos)
            {
                end = block.size();
            }
            string_view line = block.substr(cursor, end - cursor);
            if (line.empty() || line[0U] == ' ' || line[0U] == '\t')
            {
                return parse_state::malformed;
            }
            size_type colon = line.find(':');
            if (colon == string_view::npos || colon == 0U)
            {
                return parse_state::malformed;
            }
            string_view name = line.substr(0U, colon);
            string_view value = detail::server_trim_ows(line.substr(colon + 1U));
            if (name.size() > MaxHeaderNameSize || value.size() > MaxHeaderValueSize)
            {
                error_status = 431U;
                return parse_state::too_large;
            }
            if (++header_count > MaxHeaders)
            {
                error_status = 431U;
                return parse_state::too_large;
            }
            if (ascii_iequal(name, string_view("Content-Length")))
            {
                if (has_content_length)
                {
                    return parse_state::malformed;
                }
                has_content_length = true;
                uint64_t length = 0U;
                if (!detail::parse_decimal(value, length))
                {
                    return parse_state::malformed;
                }
                if (length > MaxBodySize)
                {
                    error_status = 413U;
                    return parse_state::too_large;
                }
                item.content_length = static_cast<size_type>(length);
            }
            else if (ascii_iequal(name, string_view("Transfer-Encoding")))
            {
                if (has_transfer_encoding)
                {
                    return parse_state::malformed;
                }
                has_transfer_encoding = true;
                if (!ascii_iequal(value, string_view("chunked")))
                {
                    error_status = 501U;
                    return parse_state::malformed;
                }
                item.chunked = true;
            }
            else if (ascii_iequal(name, string_view("Host")))
            {
                if (has_host)
                {
                    return parse_state::malformed;
                }
                if (value.empty())
                {
                    return parse_state::malformed;
                }
                has_host = true;
            }
            castle::status added = item.request.headers_.add(name, value);
            if (!castle::succeeded(added))
            {
                error_status = added == castle::status::full || added == castle::status::out_of_range ? 431U : 400U;
                return (error_status == 431U) ? parse_state::too_large : parse_state::malformed;
            }
            cursor = end + 2U;
        }
        if (item.request.version_ == version::http_1_1 && !has_host)
        {
            return parse_state::malformed;
        }
        if (has_content_length && has_transfer_encoding)
        {
            return parse_state::malformed;
        }
        if (ascii_iequal(item.request.header(string_view("Expect")), string_view("100-continue")) == false &&
            !item.request.header(string_view("Expect")).empty())
        {
            error_status = 417U;
            return parse_state::malformed;
        }
        bool connection_close = false;
        bool connection_keep_alive = false;
        for (size_type i = 0U; i < item.request.headers_.size(); ++i)
        {
            header_view field = item.request.headers_[i];
            if (ascii_iequal(field.name, string_view("Connection")))
            {
                connection_close = connection_close ||
                    detail::server_contains_token(field.value, string_view("close"));
                connection_keep_alive = connection_keep_alive ||
                    detail::server_contains_token(field.value, string_view("keep-alive"));
            }
        }
        if (item.request.version_ == version::http_1_1)
        {
            item.keep_alive = !connection_close;
        }
        else
        {
            item.keep_alive = connection_keep_alive && !connection_close;
        }
        return parse_state::complete;
    }

    parse_state decode_chunked(connection& item, size_type& consumed,
                               uint16_t& error_status) CASTLE_NOEXCEPT
    {
        size_type cursor = item.body_offset;
        size_type decoded = 0U;
        size_type trailer_start = 0U;
        while (true)
        {
            size_type line_end = find_crlf(item.input, item.input_size, cursor);
            if (line_end == string_view::npos)
            {
                return parse_state::incomplete;
            }
            uint64_t chunk_size = 0U;
            if (!detail::server_parse_hex(string_view(item.input + cursor, line_end - cursor), chunk_size))
            {
                return parse_state::malformed;
            }
            cursor = line_end + 2U;
            if (chunk_size == 0U)
            {
                size_type trailer_count = 0U;
                trailer_start = cursor;
                while (true)
                {
                    size_type trailer_end = find_crlf(item.input, item.input_size, cursor);
                    if (trailer_end == string_view::npos)
                    {
                        if (item.input_size - trailer_start > MaxHeaderBytes)
                        {
                            error_status = 431U;
                            return parse_state::too_large;
                        }
                        return parse_state::incomplete;
                    }
                    if (trailer_end + 2U - trailer_start > MaxHeaderBytes)
                    {
                        error_status = 431U;
                        return parse_state::too_large;
                    }
                    if (trailer_end == cursor)
                    {
                        consumed = cursor + 2U;
                        item.request.body_size_ = decoded;
                        return parse_state::complete;
                    }
                    string_view trailer(item.input + cursor, trailer_end - cursor);
                    size_type colon = trailer.find(':');
                    if (trailer.empty() || trailer[0U] == ' ' || trailer[0U] == '\t' || colon == string_view::npos ||
                        colon == 0U || !detail::is_token(trailer.substr(0U, colon)))
                    {
                        return parse_state::malformed;
                    }
                    if (trailer.substr(0U, colon).size() > MaxHeaderNameSize ||
                        detail::server_trim_ows(trailer.substr(colon + 1U)).size() > MaxHeaderValueSize ||
                        ++trailer_count > MaxHeaders)
                    {
                        error_status = 431U;
                        return parse_state::too_large;
                    }
                    if (ascii_iequal(trailer.substr(0U, colon), string_view("Content-Length")) ||
                        ascii_iequal(trailer.substr(0U, colon), string_view("Transfer-Encoding")) ||
                        ascii_iequal(trailer.substr(0U, colon), string_view("Host")))
                    {
                        return parse_state::malformed;
                    }
                    cursor = trailer_end + 2U;
                }
            }
            if (chunk_size > MaxBodySize - decoded)
            {
                return parse_state::too_large;
            }
            if (chunk_size > static_cast<uint64_t>(item.input_size - cursor) ||
                chunk_size + 2U > static_cast<uint64_t>(item.input_size - cursor))
            {
                return parse_state::incomplete;
            }
            size_type count = static_cast<size_type>(chunk_size);
            detail::http_copy_bytes(item.request.body_ + decoded,
                reinterpret_cast<uint8_t CASTLE_CONST*>(item.input + cursor), count);
            decoded += count;
            cursor += count;
            if (item.input[cursor] != '\r' || item.input[cursor + 1U] != '\n')
            {
                return parse_state::malformed;
            }
            cursor += 2U;
        }
    }

    void dispatch_request(request_type& request, response_type& response) CASTLE_NOEXCEPT
    {
        if (request.method_ == method::options && request.path() == string_view("*"))
        {
            static_cast<void>(response.set_status(200U));
            static_cast<void>(response.set_header(string_view("Allow"),
                string_view("GET, POST, PUT, DELETE, HEAD, OPTIONS, PATCH")));
            return;
        }
        if (request.method_ == method::custom)
        {
            static_cast<void>(response.set_status(501U));
            static_cast<void>(response.set_body(string_view("Method Not Implemented")));
            return;
        }
        bool path_matched = false;
        for (size_type pass = 0U; pass < 2U; ++pass)
        {
            for (size_type i = 0U; i < route_count_; ++i)
            {
                route_record& route = routes_[i];
                if (!route.used || route.prefix != (pass == 1U))
                {
                    continue;
                }
                request.parameter_count_ = 0U;
                if (!match_path(string_view(route.path, route.path_size), request.path(), route.prefix, request))
                {
                    continue;
                }
                path_matched = true;
                if (route.method_filtered && route.verb != request.method_)
                {
                    continue;
                }
                if (route.handler)
                {
                    route.handler(request, response);
                    return;
                }
            }
        }
        if (path_matched)
        {
            static_cast<void>(response.set_status(405U));
            static_cast<void>(response.set_body(string_view("Method Not Allowed")));
            return;
        }
        if (not_found_handler_)
        {
            not_found_handler_(request, response);
            return;
        }
        static_cast<void>(response.set_status(404U));
        static_cast<void>(response.set_body(string_view("Not Found")));
    }

    static bool match_path(string_view pattern, string_view path, bool prefix, request_type& request) CASTLE_NOEXCEPT
    {
        size_type pattern_offset = 0U;
        size_type path_offset = 0U;
        bool pattern_trailing_slash = pattern.size() > 1U && pattern[pattern.size() - 1U] == '/';
        size_type pattern_limit = pattern_trailing_slash ? pattern.size() - 1U : pattern.size();
        if (pattern == string_view("/"))
        {
            return prefix || path == string_view("/");
        }
        while (pattern_offset < pattern_limit)
        {
            if (pattern[pattern_offset] == '/')
            {
                ++pattern_offset;
            }
            if (pattern_offset >= pattern_limit)
            {
                break;
            }
            size_type pattern_end = pattern_offset;
            while (pattern_end < pattern_limit && pattern[pattern_end] != '/')
            {
                ++pattern_end;
            }
            if (path_offset < path.size() && path[path_offset] == '/')
            {
                ++path_offset;
            }
            size_type path_end = path_offset;
            while (path_end < path.size() && path[path_end] != '/')
            {
                ++path_end;
            }
            if (path_offset > path.size() || (path_end == path_offset && path_offset == path.size()))
            {
                return false;
            }
            string_view pattern_segment = pattern.substr(pattern_offset, pattern_end - pattern_offset);
            string_view path_segment = path.substr(path_offset, path_end - path_offset);
            if (pattern_segment.size() >= 2U && pattern_segment[0U] == '{' && pattern_segment.back() == '}')
            {
                string_view parameter_name = pattern_segment.substr(1U, pattern_segment.size() - 2U);
                if (!request.add_parameter(parameter_name, path_segment))
                {
                    return false;
                }
            }
            else if (pattern_segment != path_segment)
            {
                return false;
            }
            pattern_offset = pattern_end;
            path_offset = path_end;
            if (pattern_offset == pattern_limit)
            {
                break;
            }
        }
        if (pattern_trailing_slash)
        {
            if (prefix)
            {
                return path_offset < path.size() && path[path_offset] == '/';
            }
            if (path_offset < path.size() && path[path_offset] == '/')
            {
                ++path_offset;
            }
            else
            {
                return false;
            }
        }
        if (prefix)
        {
            return path_offset == path.size() || (path_offset < path.size() && path[path_offset] == '/');
        }
        return path_offset == path.size();
    }

    castle::status send_response(client_handle handle, request_type CASTLE_CONST& request,
                                 response_type& response, bool keep_alive) CASTLE_NOEXCEPT
    {
        size_type used = 0U;
        string_view protocol = request.protocol_version() == version::http_1_0 ? string_view("HTTP/1.0 ") : string_view("HTTP/1.1 ");
        if (!append_text(response_output_, max_response_size_, used, protocol) ||
            !append_uint(response_output_, max_response_size_, used, response.status_) ||
            !append_text(response_output_, max_response_size_, used, string_view(" ")) ||
            !append_text(response_output_, max_response_size_, used, detail::reason_phrase(response.status_)) ||
            !append_text(response_output_, max_response_size_, used, string_view("\r\n")))
        {
            return castle::status::out_of_range;
        }
        bool no_body = request.method_ == method::head || response.status_ == 204U || response.status_ == 304U ||
                       (response.status_ >= 100U && response.status_ < 200U);
        response.headers_.remove(string_view("Transfer-Encoding"));
        if (no_body)
        {
            response.headers_.remove(string_view("Content-Length"));
        }
        else
        {
            char length[10U]{};
            size_type length_size = detail::format_uint(static_cast<uint32_t>(response.body_size_), length, sizeof(length));
            if (length_size == 0U || !castle::succeeded(response.headers_.set(string_view("Content-Length"),
                                                                                string_view(length, length_size))))
            {
                return castle::status::out_of_range;
            }
        }
        if (!keep_alive)
        {
            static_cast<void>(response.headers_.set(string_view("Connection"), string_view("close")));
        }
        else
        {
            static_cast<void>(response.headers_.set(string_view("Connection"), string_view("keep-alive")));
        }
        for (size_type i = 0U; i < response.headers_.size(); ++i)
        {
            header_view header = response.headers_[i];
            if (!append_text(response_output_, max_response_size_, used, header.name) ||
                !append_text(response_output_, max_response_size_, used, string_view(": ")) ||
                !append_text(response_output_, max_response_size_, used, header.value) ||
                !append_text(response_output_, max_response_size_, used, string_view("\r\n")))
            {
                return castle::status::out_of_range;
            }
        }
        if (!append_text(response_output_, max_response_size_, used, string_view("\r\n")))
        {
            return castle::status::out_of_range;
        }
        if (!no_body)
        {
            if (response.body_size_ > max_response_size_ - used)
            {
                return castle::status::out_of_range;
            }
            for (size_type i = 0U; i < response.body_size_; ++i)
            {
                response_output_[used++] = static_cast<char>(response.body_[i]);
            }
        }
        size_type sent = 0U;
        return transport_.send_all(handle, response_output_, used, sent, nullptr);
    }

    static bool append_text(char* output, size_type capacity, size_type& used, string_view text) CASTLE_NOEXCEPT
    {
        if (text.size() > capacity - used)
        {
            return false;
        }
        for (size_type i = 0U; i < text.size(); ++i)
        {
            output[used++] = text[i];
        }
        return true;
    }

    static bool append_uint(char* output, size_type capacity, size_type& used, uint16_t value) CASTLE_NOEXCEPT
    {
        char digits[5U]{};
        size_type count = detail::format_uint(value, digits, sizeof(digits));
        return count != 0U && append_text(output, capacity, used, string_view(digits, count));
    }

    static void remove_prefix(connection& item, size_type count) CASTLE_NOEXCEPT
    {
        if (count > item.input_size)
        {
            item.input_size = 0U;
            return;
        }
        size_type remaining = item.input_size - count;
        for (size_type i = 0U; i < remaining; ++i)
        {
            item.input[i] = item.input[count + i];
        }
        item.input_size = remaining;
    }

    void send_error(client_handle handle, uint16_t code, CASTLE_CONST char* message) CASTLE_NOEXCEPT
    {
        detail::report_http_error(message);
        response_scratch_.clear();
        static_cast<void>(response_scratch_.set_status(code));
        static_cast<void>(response_scratch_.set_header(string_view("Content-Type"), string_view("text/plain; charset=utf-8")));
        static_cast<void>(response_scratch_.set_body(string_view(message)));
        request_type request;
        request.version_ = version::http_1_1;
        static_cast<void>(send_response(handle, request, response_scratch_, false));
        close_connection(handle);
    }

    void close_connection(client_handle handle) CASTLE_NOEXCEPT
    {
        connection* item = find_connection(handle);
        if (item != nullptr)
        {
            item->used = false;
        }
        static_cast<void>(transport_.close_client(handle, nullptr));
    }

    static constexpr size_type max_response_size_ = MaxBodySize +
        MaxHeaders * (MaxHeaderNameSize + MaxHeaderValueSize + 4U) + 160U;

    castle::status registration_status_ = castle::status::ok;
    bool started_ = false;
    transport_type transport_;
    response_type response_scratch_;
    char response_output_[max_response_size_]{};
    route_record routes_[MaxRoutes]{};
    size_type route_count_ = 0U;
    handler_type not_found_handler_;
    connection connections_[MaxConnections]{};
};

/** @brief Default HTTP/1.1 server type with four connections and fixed request/response buffers. */
using HttpServer = basic_http_server<>;
using http_server = basic_http_server<>;

} // namespace http
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_HTTP_SERVER_HPP

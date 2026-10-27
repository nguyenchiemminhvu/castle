// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file http1.hpp
 * @brief HTTP/1.1 request serializer and incremental response parser with fixed storage.
 *
 * `http1_request` renders a request into a caller-sized buffer. `http1_response_parser` is a byte-driven
 * state machine: it consumes every byte once, never backtracks, and reports how many bytes it used so a
 * keep-alive transport can keep any following data. Chunked bodies are rejected unless `EnableChunked` is set.
 *
 * @code
 * using namespace castle::protocols::http;
 *
 * basic_headers<> headers;
 * http1_request<512U> request;
 * request.build(method::get, "/status", "device.local:80", headers);   // request.data() is ready to send
 *
 * http1_response_parser<> parser;
 * castle::size_type consumed = 0U;
 * parser.feed(bytes, size, false, &consumed);
 * if (parser.complete())
 * {
 *     response_view response = parser.response();
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_PROTOCOLS_HTTP_HTTP1_HPP
#define CASTLE_EXT_PROTOCOLS_HTTP_HTTP1_HPP

#include "castle/algorithm/algorithm.hpp"
#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/protocols/http/headers.hpp"

#include <stdint.h>

namespace castle
{
namespace protocols
{
namespace http
{

namespace detail
{
/** @brief Strips leading and trailing spaces and tabs. */
CASTLE_INLINE string_view trim_ows(string_view value) CASTLE_NOEXCEPT
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

/** @brief Splits `name: value` into a token name and an OWS-trimmed value. */
CASTLE_INLINE bool split_field(string_view line, string_view& name, string_view& value) CASTLE_NOEXCEPT
{
    size_type colon = line.find(':');
    if (colon == string_view::npos || !is_token(line.substr(0U, colon)))
    {
        return false;
    }
    name = line.substr(0U, colon);
    value = trim_ows(line.substr(colon + 1U));
    return true;
}

/** @brief Parses an unsigned hexadecimal number (chunk sizes); rejects empty text and overflow. */
CASTLE_INLINE bool parse_hex(string_view value, uint64_t& out) CASTLE_NOEXCEPT
{
    if (value.empty())
    {
        return false;
    }
    uint64_t result = 0U;
    for (size_type i = 0U; i < value.size(); ++i)
    {
        char c = value[i];
        uint8_t digit = (c >= '0' && c <= '9') ? static_cast<uint8_t>(c - '0') :
                        (c >= 'a' && c <= 'f') ? static_cast<uint8_t>(c - 'a' + 10) :
                        (c >= 'A' && c <= 'F') ? static_cast<uint8_t>(c - 'A' + 10) : 255U;
        if (digit > 15U || result > (static_cast<uint64_t>(-1) - digit) / 16U)
        {
            return false;
        }
        result = result * 16U + digit;
    }
    out = result;
    return true;
}

/** @brief Searches a comma-separated list for `token`, ignoring ASCII case. */
CASTLE_INLINE bool contains_token(string_view list, string_view token) CASTLE_NOEXCEPT
{
    size_type offset = 0U;
    while (offset < list.size())
    {
        size_type end = list.find(',', offset);
        if (end == string_view::npos)
        {
            end = list.size();
        }
        if (ascii_iequal(trim_ows(list.substr(offset, end - offset)), token))
        {
            return true;
        }
        offset = end + 1U;
    }
    return false;
}
} // namespace detail

/** @brief Serializes an HTTP/1.1 request into caller-independent fixed storage. */
template <size_type Capacity,
          size_type MaxHeaders = 16U,
          size_type MaxNameSize = 64U,
          size_type MaxValueSize = 256U>
class http1_request
{
    static_assert(Capacity >= 64U, "HTTP/1.1 request buffer is too small");
public:
    using headers_type = basic_headers<MaxHeaders, MaxNameSize, MaxValueSize>;

    /**
     * @brief Renders a request into the internal buffer, adding Host, Content-Length and Connection when absent.
     *
     * @param verb Request method; `method::custom` uses `custom_method`.
     * @param path Request target without whitespace.
     * @param authority Value for the Host header when `headers` has none.
     * @return `ok`, `invalid_argument` for invalid input, or `full` when the buffer is too small.
     */
    castle::status build(method verb, string_view path, string_view authority,
                         headers_type CASTLE_CONST& headers,
                         string_view body = string_view(), string_view custom_method = string_view(),
                         bool keep_alive = true) CASTLE_NOEXCEPT
    {
        size_ = 0U;
        string_view token = (verb == method::custom) ? custom_method : method_token(verb);
        if (!detail::is_token(token) || !detail::is_visible_text(path) || !detail::is_visible_text(authority))
        {
            return castle::status::invalid_argument;
        }

        if (!append(token) || !append(" ") || !append(path) || !append(" HTTP/1.1\r\n"))
        {
            return castle::status::full;
        }
        bool has_host = false;
        bool has_length = false;
        bool has_connection = false;
        for (size_type i = 0U; i < headers.size(); ++i)
        {
            header_view item = headers[i];
            // Only Content-Length framing is emitted, so a caller-supplied Transfer-Encoding would desynchronize it.
            if (item.name[0U] == ':' ||
                ascii_iequal(item.name, string_view("transfer-encoding")))
            {
                return castle::status::invalid_argument;
            }
            if (ascii_iequal(item.name, string_view("host")))
            {
                has_host = true;
            }
            if (ascii_iequal(item.name, string_view("connection")))
            {
                has_connection = true;
            }
            if (ascii_iequal(item.name, string_view("content-length")))
            {
                uint64_t explicit_length = 0U;
                if (!detail::parse_decimal(item.value, explicit_length) || explicit_length != body.size())
                {
                    return castle::status::invalid_argument;
                }
                has_length = true;
            }
            if (!append(item.name) || !append(": ") || !append(item.value) || !append("\r\n"))
            {
                return castle::status::full;
            }
        }
        if (!has_host && (!append("Host: ") || !append(authority) || !append("\r\n")))
        {
            return castle::status::full;
        }
        if (!has_length && (body.size() != 0U || verb == method::post || verb == method::put || verb == method::patch))
        {
            if (!append("Content-Length: ") || !append_decimal(body.size()) || !append("\r\n"))
            {
                return castle::status::full;
            }
        }
        if (!has_connection && (!append(keep_alive ? "Connection: keep-alive\r\n" : "Connection: close\r\n")))
        {
            return castle::status::full;
        }
        if (!append("\r\n") || !append(body))
        {
            return castle::status::full;
        }
        return castle::status::ok;
    }

    /** @brief Returns the serialized request. */
    string_view data() CASTLE_CONST CASTLE_NOEXCEPT { return string_view(buffer_, size_); }

    /** @brief Returns the serialized request size in bytes. */
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

private:
    /** @brief Appends text; false when the buffer would overflow. */
    bool append(string_view value) CASTLE_NOEXCEPT
    {
        if (value.size() > Capacity - size_)
        {
            return false;
        }
        for (size_type i = 0U; i < value.size(); ++i)
        {
            buffer_[size_++] = value[i];
        }
        return true;
    }

    /** @brief Appends a NUL-terminated string; false when the buffer would overflow. */
    bool append(char CASTLE_CONST* value) CASTLE_NOEXCEPT { return append(string_view(value)); }

    /** @brief Appends `value` in decimal; false when the buffer would overflow. */
    bool append_decimal(size_type value) CASTLE_NOEXCEPT
    {
        char digits[3U * sizeof(size_type) + 1U];
        size_type count = 0U;
        do
        {
            digits[count++] = static_cast<char>('0' + (value % 10U));
            value /= 10U;
        } while (value != 0U);
        if (count > Capacity - size_)
        {
            return false;
        }
        while (count != 0U)
        {
            buffer_[size_++] = digits[--count];
        }
        return true;
    }

    char buffer_[Capacity]{};
    size_type size_ = 0U;
};

/**
 * @brief Incremental, deterministic HTTP/1.1 response parser.
 * @tparam EnableChunked Enables bounded chunk decoding; disabled by default.
 * @note Feed consumes each byte at most once and reports the number consumed. Bytes following a completed
 *       response are left to the caller, allowing a keep-alive transport to retain pipelined bytes.
 */
template <size_type MaxBodySize = 1024U,
          size_type MaxHeaders = 16U,
          size_type MaxHeaderNameSize = 64U,
          size_type MaxHeaderValueSize = 256U,
          size_type MaxLineSize = 384U,
          bool EnableChunked = false,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
class http1_response_parser
{
    static_assert(MaxBodySize > 0U, "HTTP response body buffer must be non-zero");
    static_assert(MaxLineSize >= 16U, "HTTP line buffer is too small");
public:
    using headers_type = basic_headers<MaxHeaders, MaxHeaderNameSize, MaxHeaderValueSize>;
    using body_callback = castle::callbacks::function<void(string_view), CallbackStorageSize>;

    /** @brief Constructs a parser ready for the first response. */
    http1_response_parser() CASTLE_DEFAULT;

    /**
     * @brief Installs a callback that receives every body chunk, including bytes beyond `MaxBodySize`.
     *
     * @return `ok`, or `invalid_callback` for an empty callable.
     */
    template <typename Callback>
    castle::status set_body_callback(Callback&& callback)
    {
        body_callback next(CASTLE_FORWARD<Callback>(callback));
        if (!next)
        {
            return castle::status::invalid_callback;
        }
        body_callback_ = CASTLE_MOVE(next);
        return castle::status::ok;
    }

    /** @brief Removes the body callback. */
    void clear_body_callback() CASTLE_NOEXCEPT { body_callback_.reset(); }

    /**
     * @brief Prepares the parser for the next response.
     *
     * @param head_response True when answering a HEAD request, so no body is expected.
     */
    void reset(bool head_response = false) CASTLE_NOEXCEPT
    {
        state_ = state::status_line;
        error_ = error_code::ok;
        headers_.clear();
        header_count_ = 0U;
        line_size_ = 0U;
        body_size_ = 0U;
        body_received_ = 0U;
        chunk_remaining_ = 0U;
        content_length_ = 0U;
        content_length_seen_ = false;
        chunked_seen_ = false;
        head_response_ = head_response;
        keep_alive_ = true;
        connection_close_seen_ = false;
        streamed_ = false;
        status_code_ = 0U;
        response_version_ = version::http_1_1;
    }

    /**
     * @brief Consumes response bytes; each byte is examined once.
     *
     * @param peer_closed True when the transport reached end of stream.
     * @param consumed Receives the number of bytes used; the rest belongs to the next response.
     * @return `ok`, or the failure whose cause is `last_error()`.
     */
    castle::status feed(void CASTLE_CONST* data, size_type size, bool peer_closed = false,
                        size_type* consumed = nullptr) CASTLE_NOEXCEPT
    {
        size_type offset = 0U;
        castle::status result = feed_bytes(data, size, peer_closed, offset);
        if (consumed != nullptr)
        {
            *consumed = offset;
        }
        return result;
    }

    /** @brief Returns true once a full response was parsed. */
    bool complete() CASTLE_CONST CASTLE_NOEXCEPT { return state_ == state::complete; }

    /** @brief Returns true after a protocol or limit violation. */
    bool failed() CASTLE_CONST CASTLE_NOEXCEPT { return state_ == state::failed; }

    /** @brief Returns the cause of the last failure. */
    error_code last_error() CASTLE_CONST CASTLE_NOEXCEPT { return error_; }

    /** @brief Returns the body bytes seen so far, including streamed ones. */
    uint64_t body_received() CASTLE_CONST CASTLE_NOEXCEPT { return body_received_; }

    /** @brief Returns a view of the response; it stays valid until the next `reset()` or `feed()`. */
    response_view response() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return response_view{status_code_, response_version_, views_, header_count_,
                             string_view(body_, body_size_), 0U, keep_alive_, streamed_};
    }

private:
    /** @brief Parser states; `complete` and `failed` are terminal. */
    enum class state : uint8_t
    {
        status_line, header_line, fixed_body, close_body,
        chunk_size, chunk_data, chunk_data_cr, chunk_data_lf, trailer_line,
        complete, failed
    };

    /** @brief Parses `size` bytes from `data`, advancing `offset` past every byte it uses. */
    castle::status feed_bytes(void CASTLE_CONST* data, size_type size, bool peer_closed, size_type& offset) CASTLE_NOEXCEPT
    {
        if (state_ == state::failed)
        {
            return to_status(error_);
        }
        if (state_ == state::complete)
        {
            return size == 0U ? castle::status::ok : castle::status::invalid_config;
        }
        if (data == nullptr && size != 0U)
        {
            return castle::status::invalid_argument;
        }
        char CASTLE_CONST* bytes = static_cast<char CASTLE_CONST*>(data);
        while (offset < size && state_ != state::complete)
        {
            if (state_ == state::status_line || state_ == state::header_line ||
                state_ == state::chunk_size || state_ == state::trailer_line)
            {
                char c = bytes[offset++];
                if (line_size_ >= MaxLineSize)
                {
                    return fail(error_code::header_line_limit);
                }
                line_[line_size_++] = c;
                if (c == '\n')
                {
                    size_type line_length = line_size_;
                    if (line_length < 2U || line_[line_length - 2U] != '\r')
                    {
                        return fail(error_code::malformed_message);
                    }
                    line_length -= 2U;
                    string_view line(line_, line_length);
                    state current = state_;
                    line_size_ = 0U;
                    castle::status result = consume_line(current, line);
                    if (!castle::succeeded(result))
                    {
                        return result;
                    }
                }
                continue;
            }
            if (state_ == state::fixed_body || state_ == state::close_body || state_ == state::chunk_data)
            {
                size_type take = size - offset;
                if (state_ == state::fixed_body)
                {
                    take = castle::min(take, static_cast<size_type>(content_length_ - body_received_));
                }
                if (state_ == state::chunk_data)
                {
                    take = castle::min(take, chunk_remaining_);
                }
                if (take == 0U) // LCOV_EXCL_BR_LINE
                {
                    return fail(error_code::malformed_message);
                }
                castle::status result = append_body(bytes + offset, take);
                if (!castle::succeeded(result))
                {
                    return result;
                }
                offset += take;
                if (state_ == state::fixed_body && body_received_ == content_length_)
                {
                    mark_complete();
                }
                else if (state_ == state::chunk_data && chunk_remaining_ == 0U)
                {
                    state_ = state::chunk_data_cr;
                }
                continue;
            }
            if (state_ == state::chunk_data_cr)
            {
                if (bytes[offset++] != '\r')
                {
                    return fail(error_code::invalid_chunk);
                }
                state_ = state::chunk_data_lf;
                continue;
            }
            // chunk_data_lf is the only state left: complete and failed end the loop.
            if (bytes[offset++] != '\n')
            {
                return fail(error_code::invalid_chunk);
            }
            state_ = state::chunk_size;
        }
        if (peer_closed && state_ != state::complete)
        {
            if (state_ == state::close_body)
            {
                mark_complete();
            }
            else
            {
                return fail(error_code::connection_closed);
            }
        }
        return castle::status::ok;
    }

    /** @brief Enters the failed state and returns the matching status. */
    castle::status fail(error_code error) CASTLE_NOEXCEPT
    {
        error_ = error;
        state_ = state::failed;
        return to_status(error_);
    }

    /** @brief Handles one CRLF-terminated line of the status line, headers, chunk sizes or trailers. */
    castle::status consume_line(state current, string_view line) CASTLE_NOEXCEPT
    {
        if (current == state::status_line)
        {
            return parse_status(line);
        }
        if (current == state::header_line)
        {
            if (line.empty())
            {
                return headers_done();
            }
            string_view name;
            string_view value;
            if (!detail::split_field(line, name, value))
            {
                return fail(error_code::invalid_header);
            }
            castle::status added = headers_.add(name, value);
            if (!castle::succeeded(added))
            {
                return fail(added == castle::status::full || added == castle::status::out_of_range ? error_code::header_limit : error_code::invalid_header);
            }
            if (ascii_iequal(name, string_view("content-length")))
            {
                uint64_t parsed = 0U;
                if (!detail::parse_decimal(value, parsed) || (content_length_seen_ && parsed != content_length_))
                {
                    return fail(error_code::malformed_message);
                }
                content_length_ = parsed;
                content_length_seen_ = true;
            }
            else if (ascii_iequal(name, string_view("transfer-encoding")))
            {
                if (ascii_iequal(detail::trim_ows(value), string_view("chunked")))
                {
                    chunked_seen_ = true;
                    if (!EnableChunked)
                    {
                        return fail(error_code::chunked_not_supported);
                    }
                }
                else
                {
                    return fail(error_code::unsupported_feature);
                }
            }
            else if (ascii_iequal(name, string_view("connection")))
            {
                if (detail::contains_token(value, string_view("close")))
                {
                    connection_close_seen_ = true;
                    keep_alive_ = false;
                }
                else if (!connection_close_seen_ && detail::contains_token(value, string_view("keep-alive")))
                {
                    keep_alive_ = true;
                }
            }
            return castle::status::ok;
        }
        if (current == state::chunk_size)
        {
            size_type extension = line.find(';');
            string_view digits = line.substr(0U, extension);
            uint64_t chunk = 0U;
            if (!detail::parse_hex(digits, chunk))
            {
                return fail(error_code::invalid_chunk);
            }
            // size_type is 64-bit on supported hosts, so a parsed chunk size always fits.
            if (chunk > static_cast<uint64_t>(static_cast<size_type>(-1))) // LCOV_EXCL_BR_LINE
            {
                return fail(error_code::invalid_chunk);
            }
            chunk_remaining_ = static_cast<size_type>(chunk);
            if (chunk_remaining_ == 0U)
            {
                state_ = state::trailer_line;
            }
            else
            {
                state_ = state::chunk_data;
            }
            return castle::status::ok;
        }
        // consume_line() only receives line states, and trailer_line is the last of them.
        if (current == state::trailer_line) // LCOV_EXCL_BR_LINE
        {
            if (line.empty())
            {
                mark_complete();
                return castle::status::ok;
            }
            string_view name;
            string_view value;
            if (!detail::split_field(line, name, value))
            {
                return fail(error_code::invalid_chunk);
            }
            if (ascii_iequal(name, string_view("content-length")) ||
                ascii_iequal(name, string_view("transfer-encoding")) ||
                ascii_iequal(name, string_view("host")) ||
                ascii_iequal(name, string_view("connection")) ||
                ascii_iequal(name, string_view("trailer")))
            {
                return fail(error_code::invalid_chunk);
            }
            castle::status added = headers_.add(name, value);
            if (!castle::succeeded(added))
            {
                return fail(added == castle::status::full || added == castle::status::out_of_range
                    ? error_code::header_limit : error_code::invalid_chunk);
            }
            header_count_ = headers_.size();
            views_[header_count_ - 1U] = headers_[header_count_ - 1U];
            return castle::status::ok;
        }
        return fail(error_code::malformed_message);
    }

    /** @brief Validates the status line and records the version and code. */
    castle::status parse_status(string_view line) CASTLE_NOEXCEPT
    {
        bool is_http_10 = line.size() >= 9U && line.substr(0U, 9U) == string_view("HTTP/1.0 ");
        bool is_http_11 = line.size() >= 9U && line.substr(0U, 9U) == string_view("HTTP/1.1 ");
        if (line.size() < 12U || (!is_http_10 && !is_http_11) ||
            line[9U] < '0' || line[9U] > '9' || line[10U] < '0' || line[10U] > '9' ||
            line[11U] < '0' || line[11U] > '9' || (line.size() > 12U && line[12U] != ' '))
        {
            return fail(error_code::malformed_status_line);
        }
        response_version_ = is_http_10 ? version::http_1_0 : version::http_1_1;
        keep_alive_ = !is_http_10;
        connection_close_seen_ = false;
        status_code_ = static_cast<uint16_t>((line[9U] - '0') * 100 + (line[10U] - '0') * 10 + (line[11U] - '0'));
        if (status_code_ < 100U || status_code_ > 599U)
        {
            return fail(error_code::malformed_status_line);
        }
        state_ = state::header_line;
        return castle::status::ok;
    }

    /** @brief Chooses the body framing once the blank line after the headers arrives. */
    castle::status headers_done() CASTLE_NOEXCEPT
    {
        if (content_length_seen_ && chunked_seen_)
        {
            return fail(error_code::malformed_message);
        }
        header_count_ = headers_.size();
        for (size_type i = 0U; i < header_count_; ++i)
        {
            views_[i] = headers_[i];
        }
        if (status_code_ >= 100U && status_code_ < 200U && status_code_ != 101U)
        {
            // Consume interim response headers and continue with the final response on this connection.
            headers_.clear();
            header_count_ = 0U;
            content_length_ = 0U;
            content_length_seen_ = false;
            chunked_seen_ = false;
            status_code_ = 0U;
            response_version_ = version::http_1_1;
            keep_alive_ = true;
            connection_close_seen_ = false;
            state_ = state::status_line;
        }
        else if (status_code_ == 101U)
        {
            return fail(error_code::unsupported_feature);
        }
        else if (head_response_ || status_code_ == 204U || status_code_ == 304U)
        {
            mark_complete();
        }
        else if (chunked_seen_)
        {
            state_ = state::chunk_size;
        }
        else if (content_length_seen_)
        {
            if (content_length_ > static_cast<uint64_t>(static_cast<size_type>(-1))) // LCOV_EXCL_BR_LINE
            {
                return fail(error_code::body_limit);
            }
            if (content_length_ > MaxBodySize && !body_callback_)
            {
                return fail(error_code::body_limit);
            }
            if (content_length_ == 0U)
            {
                mark_complete();
            }
            else
            {
                state_ = state::fixed_body;
            }
        }
        else
        {
            state_ = state::close_body;
            keep_alive_ = false;
        }
        return castle::status::ok;
    }

    /** @brief Retains body bytes up to `MaxBodySize` and forwards all of them to the body callback. */
    castle::status append_body(char CASTLE_CONST* data, size_type size) CASTLE_NOEXCEPT
    {
        if (body_callback_)
        {
            body_callback_(string_view(data, size));
        }
        size_type room = MaxBodySize - body_size_;
        size_type keep = castle::min(room, size);
        for (size_type i = 0U; i < keep; ++i)
        {
            body_[body_size_++] = data[i];
        }
        if (keep != size)
        {
            if (!body_callback_)
            {
                return fail(error_code::body_limit);
            }
            streamed_ = true;
        }
        body_received_ += size;
        if (state_ == state::chunk_data)
        {
            chunk_remaining_ -= size;
        }
        return castle::status::ok;
    }

    /** @brief Marks the response complete. */
    void mark_complete() CASTLE_NOEXCEPT { state_ = state::complete; }

    headers_type headers_{};
    header_view views_[MaxHeaders]{};
    char line_[MaxLineSize]{};
    char body_[MaxBodySize]{};
    body_callback body_callback_{};
    size_type header_count_ = 0U;
    size_type line_size_ = 0U;
    size_type body_size_ = 0U;
    uint64_t body_received_ = 0U;
    size_type chunk_remaining_ = 0U;
    uint64_t content_length_ = 0U;
    uint16_t status_code_ = 0U;
    version response_version_ = version::http_1_1;
    state state_ = state::status_line;
    error_code error_ = error_code::ok;
    bool content_length_seen_ = false;
    bool chunked_seen_ = false;
    bool connection_close_seen_ = false;
    bool head_response_ = false;
    bool keep_alive_ = true;
    bool streamed_ = false;
};

} // namespace http
} // namespace protocols
} // namespace castle

#endif // CASTLE_EXT_PROTOCOLS_HTTP_HTTP1_HPP

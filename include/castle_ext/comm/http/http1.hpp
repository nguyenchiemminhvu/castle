// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
#ifndef CASTLE_EXT_COMM_HTTP_HTTP1_HPP
#define CASTLE_EXT_COMM_HTTP_HTTP1_HPP

#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/comm/http/headers.hpp"

#include <stdint.h>

namespace castle
{
namespace comm
{
namespace http
{

namespace detail
{
CASTLE_INLINE bool token_char(char c) CASTLE_NOEXCEPT
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*' ||
           c == '+' || c == '-' || c == '.' || c == '^' || c == '_' || c == '`' || c == '|' || c == '~';
}

CASTLE_INLINE string_view trim_ows(string_view value) CASTLE_NOEXCEPT
{
    size_type first = 0U;
    size_type last = value.size();
    while (first < last && (value[first] == ' ' || value[first] == '\t')) { ++first; }
    while (last > first && (value[last - 1U] == ' ' || value[last - 1U] == '\t')) { --last; }
    return value.substr(first, last - first);
}

CASTLE_INLINE bool parse_decimal(string_view value, uint64_t& out) CASTLE_NOEXCEPT
{
    if (value.empty()) { return false; }
    uint64_t result = 0U;
    for (size_type i = 0U; i < value.size(); ++i)
    {
        if (value[i] < '0' || value[i] > '9') { return false; }
        uint8_t digit = static_cast<uint8_t>(value[i] - '0');
        if (result > (static_cast<uint64_t>(-1) - digit) / 10U) { return false; }
        result = result * 10U + digit;
    }
    out = result;
    return true;
}

CASTLE_INLINE bool parse_hex(string_view value, uint64_t& out) CASTLE_NOEXCEPT
{
    if (value.empty()) { return false; }
    uint64_t result = 0U;
    for (size_type i = 0U; i < value.size(); ++i)
    {
        char c = value[i];
        uint8_t digit = (c >= '0' && c <= '9') ? static_cast<uint8_t>(c - '0') :
                        (c >= 'a' && c <= 'f') ? static_cast<uint8_t>(c - 'a' + 10) :
                        (c >= 'A' && c <= 'F') ? static_cast<uint8_t>(c - 'A' + 10) : 255U;
        if (digit > 15U || result > (static_cast<uint64_t>(-1) - digit) / 16U) { return false; }
        result = result * 16U + digit;
    }
    out = result;
    return true;
}

CASTLE_INLINE bool contains_token(string_view list, string_view token) CASTLE_NOEXCEPT
{
    size_type offset = 0U;
    while (offset < list.size())
    {
        size_type end = list.find(',', offset);
        if (end == string_view::npos) { end = list.size(); }
        if (ascii_iequal(trim_ows(list.substr(offset, end - offset)), token)) { return true; }
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

    castle::status build(method verb, string_view path, string_view authority,
                         headers_type const& headers,
                         string_view body = string_view(), string_view custom_method = string_view(),
                         bool keep_alive = true) CASTLE_NOEXCEPT
    {
        size_ = 0U;
        string_view token = (verb == method::custom) ? custom_method : method_token(verb);
        if (token.empty() || path.empty() || authority.empty()) { return castle::status::invalid_argument; }
        for (size_type i = 0U; i < token.size(); ++i)
        {
            if (!detail::token_char(token[i])) { return castle::status::invalid_argument; }
        }
        for (size_type i = 0U; i < path.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(path[i]);
            if (c <= 32U || c == 127U || c == '\r' || c == '\n') { return castle::status::invalid_argument; }
        }
        for (size_type i = 0U; i < authority.size(); ++i)
        {
            if (authority[i] == '\r' || authority[i] == '\n' || authority[i] == ' ' || authority[i] == '\t')
            {
                return castle::status::invalid_argument;
            }
        }

        if (!append(token) || !append(" ") || !append(path) || !append(" HTTP/1.1\r\n")) { return castle::status::full; }
        bool has_host = false;
        bool has_length = false;
        bool has_connection = false;
        for (size_type i = 0U; i < headers.size(); ++i)
        {
            header_view item = headers[i];
            if (item.name.empty() || item.name[0U] == ':') { return castle::status::invalid_argument; }
            if (ascii_iequal(item.name, string_view("host"))) { has_host = true; }
            if (ascii_iequal(item.name, string_view("content-length"))) { has_length = true; }
            if (ascii_iequal(item.name, string_view("connection"))) { has_connection = true; }
            if (!append(item.name) || !append(": ") || !append(item.value) || !append("\r\n")) { return castle::status::full; }
        }
        if (!has_host && (!append("Host: ") || !append(authority) || !append("\r\n"))) { return castle::status::full; }
        if (has_length)
        {
            for (size_type i = 0U; i < headers.size(); ++i)
            {
                header_view item = headers[i];
                if (ascii_iequal(item.name, string_view("content-length")))
                {
                    uint64_t explicit_length = 0U;
                    if (!detail::parse_decimal(item.value, explicit_length) || explicit_length != body.size())
                    {
                        return castle::status::invalid_argument;
                    }
                }
            }
        }
        if (!has_length && (body.size() != 0U || verb == method::post || verb == method::put || verb == method::patch))
        {
            if (!append("Content-Length: ") || !append_decimal(body.size()) || !append("\r\n")) { return castle::status::full; }
        }
        if (!has_connection && (!append(keep_alive ? "Connection: keep-alive\r\n" : "Connection: close\r\n")))
        {
            return castle::status::full;
        }
        if (!append("\r\n") || !append(body)) { return castle::status::full; }
        return castle::status::ok;
    }

    string_view data() CASTLE_CONST CASTLE_NOEXCEPT { return string_view(buffer_, size_); }
    size_type size() CASTLE_CONST CASTLE_NOEXCEPT { return size_; }

private:
    bool append(string_view value) CASTLE_NOEXCEPT
    {
        if (value.size() > Capacity - size_) { return false; }
        for (size_type i = 0U; i < value.size(); ++i) { buffer_[size_++] = value[i]; }
        return true;
    }
    bool append(char const* value) CASTLE_NOEXCEPT { return append(string_view(value)); }
    bool append_decimal(size_type value) CASTLE_NOEXCEPT
    {
        char digits[3U * sizeof(size_type) + 1U];
        size_type count = 0U;
        do { digits[count++] = static_cast<char>('0' + (value % 10U)); value /= 10U; } while (value != 0U);
        if (count > Capacity - size_) { return false; }
        while (count != 0U) { buffer_[size_++] = digits[--count]; }
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

    http1_response_parser() CASTLE_DEFAULT;

    template <typename Callback>
    castle::status set_body_callback(Callback&& callback)
    {
        body_callback next(CASTLE_FORWARD<Callback>(callback));
        if (!next) { return castle::status::invalid_callback; }
        body_callback_ = CASTLE_MOVE(next);
        return castle::status::ok;
    }
    void clear_body_callback() CASTLE_NOEXCEPT { body_callback_.reset(); }

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

    castle::status feed(void const* data, size_type size, bool peer_closed = false,
                        size_type* consumed = nullptr) CASTLE_NOEXCEPT
    {
        if (consumed != nullptr) { *consumed = 0U; }
        if (state_ == state::failed) { return to_status(error_); }
        if (state_ == state::complete) { return size == 0U ? castle::status::ok : castle::status::invalid_config; }
        if (data == nullptr && size != 0U) { return castle::status::invalid_argument; }
        char const* bytes = static_cast<char const*>(data);
        size_type offset = 0U;
        while (offset < size && state_ != state::complete && state_ != state::failed)
        {
            if (state_ == state::status_line || state_ == state::header_line ||
                state_ == state::chunk_size || state_ == state::trailer_line)
            {
                char c = bytes[offset++];
                if (line_size_ >= MaxLineSize) { if (consumed) { *consumed = offset; } return fail(error_code::header_line_limit); }
                line_[line_size_++] = c;
                if (c == '\n')
                {
                    size_type line_length = line_size_;
                    if (line_length < 2U || line_[line_length - 2U] != '\r')
                    {
                        if (consumed) { *consumed = offset; }
                        return fail(error_code::malformed_message);
                    }
                    line_length -= 2U;
                    string_view line(line_, line_length);
                    state current = state_;
                    line_size_ = 0U;
                    castle::status result = consume_line(current, line);
                    if (!castle::succeeded(result)) { if (consumed) { *consumed = offset; } return result; }
                }
                continue;
            }
            if (state_ == state::fixed_body || state_ == state::close_body || state_ == state::chunk_data)
            {
                size_type take = size - offset;
                if (state_ == state::fixed_body) { take = min_size(take, static_cast<size_type>(content_length_ - body_received_)); }
                if (state_ == state::chunk_data) { take = min_size(take, chunk_remaining_); }
                if (take == 0U) { if (consumed) { *consumed = offset; } return fail(error_code::malformed_message); }
                castle::status result = append_body(bytes + offset, take);
                if (!castle::succeeded(result)) { if (consumed) { *consumed = offset; } return result; }
                offset += take;
                if (state_ == state::fixed_body && body_received_ == content_length_) { mark_complete(); }
                else if (state_ == state::chunk_data && chunk_remaining_ == 0U) { state_ = state::chunk_data_cr; }
                continue;
            }
            if (state_ == state::chunk_data_cr)
            {
                if (bytes[offset++] != '\r') { if (consumed) { *consumed = offset; } return fail(error_code::invalid_chunk); }
                state_ = state::chunk_data_lf;
                continue;
            }
            if (state_ == state::chunk_data_lf)
            {
                if (bytes[offset++] != '\n') { if (consumed) { *consumed = offset; } return fail(error_code::invalid_chunk); }
                state_ = state::chunk_size;
                continue;
            }
        }
        if (consumed != nullptr) { *consumed = offset; }
        if (peer_closed && state_ != state::complete)
        {
            if (state_ == state::close_body) { mark_complete(); }
            else { return fail(error_code::connection_closed); }
        }
        return state_ == state::failed ? to_status(error_) : castle::status::ok;
    }

    bool complete() CASTLE_CONST CASTLE_NOEXCEPT { return state_ == state::complete; }
    bool failed() CASTLE_CONST CASTLE_NOEXCEPT { return state_ == state::failed; }
    error_code last_error() CASTLE_CONST CASTLE_NOEXCEPT { return error_; }
    uint64_t body_received() CASTLE_CONST CASTLE_NOEXCEPT { return body_received_; }

    response_view response() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return response_view{status_code_, response_version_, views_, header_count_,
                             string_view(body_, body_size_), 0U, keep_alive_, streamed_};
    }

private:
    enum class state : uint8_t
    {
        status_line, header_line, fixed_body, close_body,
        chunk_size, chunk_data, chunk_data_cr, chunk_data_lf, trailer_line,
        complete, failed
    };

    static size_type min_size(size_type a, size_type b) CASTLE_NOEXCEPT { return a < b ? a : b; }

    castle::status fail(error_code error) CASTLE_NOEXCEPT
    {
        error_ = error;
        state_ = state::failed;
        return to_status(error_);
    }

    castle::status consume_line(state current, string_view line) CASTLE_NOEXCEPT
    {
        if (current == state::status_line) { return parse_status(line); }
        if (current == state::header_line)
        {
            if (line.empty()) { return headers_done(); }
            size_type colon = line.find(':');
            if (colon == string_view::npos || colon == 0U) { return fail(error_code::invalid_header); }
            string_view name = line.substr(0U, colon);
            for (size_type i = 0U; i < name.size(); ++i)
            {
                if (!detail::token_char(name[i])) { return fail(error_code::invalid_header); }
            }
            string_view value = detail::trim_ows(line.substr(colon + 1U));
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
                    if (!EnableChunked) { return fail(error_code::chunked_not_supported); }
                }
                else { return fail(error_code::unsupported_feature); }
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
            if (!detail::parse_hex(digits, chunk) || chunk > static_cast<uint64_t>(static_cast<size_type>(-1)))
            {
                return fail(error_code::invalid_chunk);
            }
            chunk_remaining_ = static_cast<size_type>(chunk);
            if (chunk_remaining_ == 0U) { state_ = state::trailer_line; }
            else { state_ = state::chunk_data; }
            return castle::status::ok;
        }
        if (current == state::trailer_line)
        {
            if (line.empty()) { mark_complete(); return castle::status::ok; }
            size_type colon = line.find(':');
            if (colon == string_view::npos || colon == 0U) { return fail(error_code::invalid_chunk); }
            string_view name = line.substr(0U, colon);
            for (size_type i = 0U; i < name.size(); ++i)
            {
                if (!detail::token_char(name[i])) { return fail(error_code::invalid_chunk); }
            }
            if (ascii_iequal(name, string_view("content-length")) ||
                ascii_iequal(name, string_view("transfer-encoding")) ||
                ascii_iequal(name, string_view("host")) ||
                ascii_iequal(name, string_view("connection")) ||
                ascii_iequal(name, string_view("trailer")))
            {
                return fail(error_code::invalid_chunk);
            }
            castle::status added = headers_.add(name, detail::trim_ows(line.substr(colon + 1U)));
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
        if (status_code_ < 100U || status_code_ > 599U) { return fail(error_code::malformed_status_line); }
        state_ = state::header_line;
        return castle::status::ok;
    }

    castle::status headers_done() CASTLE_NOEXCEPT
    {
        if (content_length_seen_ && chunked_seen_) { return fail(error_code::malformed_message); }
        header_count_ = headers_.size();
        for (size_type i = 0U; i < header_count_; ++i) { views_[i] = headers_[i]; }
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
            if (content_length_ > static_cast<uint64_t>(static_cast<size_type>(-1))) { return fail(error_code::body_limit); }
            if (content_length_ > MaxBodySize && !body_callback_) { return fail(error_code::body_limit); }
            if (content_length_ == 0U) { mark_complete(); }
            else { state_ = state::fixed_body; }
        }
        else
        {
            state_ = state::close_body;
            keep_alive_ = false;
        }
        return castle::status::ok;
    }

    castle::status append_body(char const* data, size_type size) CASTLE_NOEXCEPT
    {
        if (body_callback_) { body_callback_(string_view(data, size)); }
        size_type room = MaxBodySize - body_size_;
        size_type keep = min_size(room, size);
        for (size_type i = 0U; i < keep; ++i) { body_[body_size_++] = data[i]; }
        if (keep != size)
        {
            if (!body_callback_) { return fail(error_code::body_limit); }
            streamed_ = true;
        }
        body_received_ += size;
        if (state_ == state::chunk_data) { chunk_remaining_ -= size; }
        return castle::status::ok;
    }

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
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_HTTP_HTTP1_HPP

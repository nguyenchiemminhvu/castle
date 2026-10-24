// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com
#ifndef CASTLE_EXT_COMM_HTTP_HTTP2_HPP
#define CASTLE_EXT_COMM_HTTP_HTTP2_HPP

#include "castle/callbacks/function.hpp"
#include "castle/core/compiler.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/comm/http/hpack.hpp"
#include "castle_ext/comm/http/headers.hpp"

#include <stdint.h>

namespace castle
{
namespace comm
{
namespace http
{

enum class http2_frame_type : uint8_t
{
    data = 0U,
    headers = 1U,
    priority = 2U,
    rst_stream = 3U,
    settings = 4U,
    push_promise = 5U,
    ping = 6U,
    goaway = 7U,
    window_update = 8U,
    continuation = 9U
};

struct http2_frame_view
{
    uint32_t length = 0U;
    http2_frame_type type = http2_frame_type::data;
    uint8_t flags = 0U;
    uint32_t stream_id = 0U;
    uint8_t const* payload = nullptr;
};

namespace detail
{
CASTLE_INLINE uint32_t read_u32(uint8_t const* p) CASTLE_NOEXCEPT
{
    return (static_cast<uint32_t>(p[0U]) << 24U) |
           (static_cast<uint32_t>(p[1U]) << 16U) |
           (static_cast<uint32_t>(p[2U]) << 8U) |
           static_cast<uint32_t>(p[3U]);
}

CASTLE_INLINE void write_u32(uint8_t* p, uint32_t value) CASTLE_NOEXCEPT
{
    p[0U] = static_cast<uint8_t>(value >> 24U);
    p[1U] = static_cast<uint8_t>(value >> 16U);
    p[2U] = static_cast<uint8_t>(value >> 8U);
    p[3U] = static_cast<uint8_t>(value);
}

CASTLE_INLINE bool h2_token(string_view text) CASTLE_NOEXCEPT
{
    if (text.empty()) { return false; }
    for (size_type i = 0U; i < text.size(); ++i)
    {
        char c = text[i];
        bool valid = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') ||
                     (c >= 'a' && c <= 'z') || c == '!' || c == '#' || c == '$' ||
                     c == '%' || c == '&' || c == '\'' || c == '*' || c == '+' ||
                     c == '-' || c == '.' || c == '^' || c == '_' || c == '`' ||
                     c == '|' || c == '~';
        if (!valid) { return false; }
    }
    return true;
}

CASTLE_INLINE bool h2_parse_decimal(string_view value, uint64_t& output) CASTLE_NOEXCEPT
{
    if (value.empty()) { return false; }
    uint64_t parsed = 0U;
    for (size_type i = 0U; i < value.size(); ++i)
    {
        if (value[i] < '0' || value[i] > '9') { return false; }
        uint8_t digit = static_cast<uint8_t>(value[i] - '0');
        if (parsed > (static_cast<uint64_t>(-1) - digit) / 10U) { return false; }
        parsed = parsed * 10U + digit;
    }
    output = parsed;
    return true;
}
 } // namespace detail

/**
 * @brief Incremental HTTP/2 frame parser with fixed payload storage.
 * @tparam MaxFrameSize Maximum accepted payload size. RFC 9113 requires at least 16,384 bytes.
 */
template <size_type MaxFrameSize = 16384U>
class http2_frame_parser
{
    static_assert(MaxFrameSize >= 16384U, "HTTP/2 clients must accept frames of at least 16,384 bytes");
    static_assert(MaxFrameSize <= 0xFFFFFFU, "HTTP/2 maximum frame size is 24 bits");
public:
    http2_frame_parser() CASTLE_DEFAULT;

    void reset() CASTLE_NOEXCEPT
    {
        header_used_ = 0U;
        payload_used_ = 0U;
        payload_size_ = 0U;
        error_ = error_code::ok;
    }

    template <typename Callback>
    castle::status feed(uint8_t const* data, size_type size, Callback&& callback,
                        size_type* consumed = nullptr) CASTLE_NOEXCEPT
    {
        if (consumed != nullptr) { *consumed = 0U; }
        if (error_ != error_code::ok) { return to_status(error_); }
        if (data == nullptr && size != 0U) { return castle::status::invalid_argument; }
        size_type offset = 0U;
        while (offset < size)
        {
            if (header_used_ < 9U)
            {
                size_type take = min_size(9U - header_used_, size - offset);
                for (size_type i = 0U; i < take; ++i) { header_[header_used_++] = data[offset++]; }
                if (header_used_ != 9U) { break; }
                payload_size_ = (static_cast<uint32_t>(header_[0U]) << 16U) |
                                (static_cast<uint32_t>(header_[1U]) << 8U) |
                                static_cast<uint32_t>(header_[2U]);
                if (payload_size_ > MaxFrameSize) { return fail(error_code::frame_size_limit, offset, consumed); }
                // The stream-id reserved bit is ignored by receivers as required by RFC 9113.
                stream_id_ = detail::read_u32(header_ + 5U) & 0x7FFFFFFFU;
                type_ = static_cast<http2_frame_type>(header_[3U]);
                flags_ = header_[4U];
                payload_used_ = 0U;
                castle::status valid = validate_header();
                if (!castle::succeeded(valid)) { return fail(error_, offset, consumed); }
                if (payload_size_ == 0U)
                {
                    castle::status result = callback(http2_frame_view{0U, type_, flags_, stream_id_, payload_});
                    reset_frame();
                    if (!castle::succeeded(result)) { if (consumed) { *consumed = offset; } return result; }
                }
            }
            else
            {
                size_type take = min_size(static_cast<size_type>(payload_size_) - payload_used_, size - offset);
                for (size_type i = 0U; i < take; ++i) { payload_[payload_used_++] = data[offset++]; }
                if (payload_used_ == payload_size_)
                {
                    castle::status result = callback(http2_frame_view{payload_size_, type_, flags_, stream_id_, payload_});
                    reset_frame();
                    if (!castle::succeeded(result)) { if (consumed) { *consumed = offset; } return result; }
                }
            }
        }
        if (consumed != nullptr) { *consumed = offset; }
        return castle::status::ok;
    }

    error_code last_error() CASTLE_CONST CASTLE_NOEXCEPT { return error_; }

private:
    static size_type min_size(size_type a, size_type b) CASTLE_NOEXCEPT { return a < b ? a : b; }

    castle::status validate_header() CASTLE_NOEXCEPT
    {
        uint8_t raw_type = header_[3U];
        switch (raw_type)
        {
            case 0U: // DATA
                if (stream_id_ == 0U) { error_ = error_code::invalid_frame; return castle::status::invalid_argument; }
                break;
            case 1U: // HEADERS
                if (stream_id_ == 0U) { error_ = error_code::invalid_frame; return castle::status::invalid_argument; }
                break;
            case 2U: // PRIORITY
                if (stream_id_ == 0U || payload_size_ != 5U) { error_ = error_code::invalid_frame; return castle::status::invalid_argument; }
                break;
            case 3U: // RST_STREAM
                if (stream_id_ == 0U || payload_size_ != 4U) { error_ = error_code::invalid_frame; return castle::status::invalid_argument; }
                break;
            case 4U: // SETTINGS
                if (stream_id_ != 0U || ((flags_ & 0x01U) != 0U ? payload_size_ != 0U : (payload_size_ % 6U) != 0U))
                { error_ = error_code::invalid_frame; return castle::status::invalid_argument; }
                break;
            case 5U: // PUSH_PROMISE
                if (stream_id_ == 0U || payload_size_ < 4U) { error_ = error_code::invalid_frame; return castle::status::invalid_argument; }
                break;
            case 6U: // PING
                if (stream_id_ != 0U || payload_size_ != 8U) { error_ = error_code::invalid_frame; return castle::status::invalid_argument; }
                break;
            case 7U: // GOAWAY
                if (stream_id_ != 0U || payload_size_ < 8U) { error_ = error_code::invalid_frame; return castle::status::invalid_argument; }
                break;
            case 8U: // WINDOW_UPDATE
                if (payload_size_ != 4U) { error_ = error_code::invalid_frame; return castle::status::invalid_argument; }
                break;
            case 9U: // CONTINUATION
                if (stream_id_ == 0U) { error_ = error_code::invalid_frame; return castle::status::invalid_argument; }
                break;
            default: break; // Unknown extension frames are ignored by the connection layer.
        }
        error_ = error_code::ok;
        return castle::status::ok;
    }

    castle::status fail(error_code value, size_type offset, size_type* consumed) CASTLE_NOEXCEPT
    {
        error_ = value;
        if (consumed != nullptr) { *consumed = offset; }
        return to_status(value);
    }

    void reset_frame() CASTLE_NOEXCEPT
    {
        header_used_ = 0U;
        payload_used_ = 0U;
        payload_size_ = 0U;
    }

    uint8_t header_[9U]{};
    uint8_t payload_[MaxFrameSize]{};
    uint32_t payload_size_ = 0U;
    uint32_t stream_id_ = 0U;
    size_type header_used_ = 0U;
    size_type payload_used_ = 0U;
    http2_frame_type type_ = http2_frame_type::data;
    uint8_t flags_ = 0U;
    error_code error_ = error_code::ok;
};

/**
 * @brief Static-memory HTTP/2 client session with bounded HPACK, streams and flow control.
 * @tparam MaxConcurrentStreams Maximum client-initiated streams tracked at compile time.
 * @tparam MaxHeaderBlockSize Maximum encoded header-block bytes per stream.
 * @tparam MaxFrameSize Maximum inbound payload accepted by the frame parser (minimum 16 KiB).
 * @tparam MaxBodySize Bytes retained per response; overflow is delivered to the optional body callback.
 * @tparam MaxOutputSize Fixed circular output queue capacity.
 *
 * `start()` queues the mandatory prior-knowledge connection preface and a SETTINGS frame. This class
 * contains protocol state only; `http2_client` in client.hpp pumps its bounded output/input through a
 * CASTLE TCP client. No HTTP Upgrade request is emitted.
 */
template <size_type MaxConcurrentStreams = 4U,
          size_type MaxHeaderBlockSize = 2048U,
          size_type MaxFrameSize = 16384U,
          size_type MaxBodySize = 1024U,
          size_type MaxHeaders = 24U,
          size_type MaxHeaderNameSize = 64U,
          size_type MaxHeaderValueSize = 256U,
          size_type MaxDynamicTableBytes = 0U,
          size_type MaxDynamicEntries = (MaxDynamicTableBytes / 32U) + 1U,
          size_type MaxOutputSize = 8192U,
          size_type CallbackStorageSize = castle::inplace_storage_reserved>
class http2_session
{
    static_assert(MaxConcurrentStreams > 0U, "HTTP/2 stream capacity must be non-zero");
    static_assert(MaxHeaderBlockSize > 0U, "HTTP/2 header block capacity must be non-zero");
    static_assert(MaxBodySize > 0U, "HTTP/2 body capacity must be non-zero");
    static_assert(MaxOutputSize >= 64U, "HTTP/2 output queue is too small for the preface and SETTINGS");
    static_assert(MaxDynamicTableBytes <= 0xFFFFFFFFULL, "HPACK table limit is 32-bit");

public:
    using headers_type = basic_headers<MaxHeaders, MaxHeaderNameSize, MaxHeaderValueSize>;
    using response_callback = castle::callbacks::function<void(castle::status, response_view const&), CallbackStorageSize>;
    using body_callback = castle::callbacks::function<void(uint32_t, string_view), CallbackStorageSize>;
    using frame_parser_type = http2_frame_parser<MaxFrameSize>;

    http2_session() CASTLE_DEFAULT;

    template <typename Callback>
    castle::status set_response_callback(Callback&& callback)
    {
        response_callback next(CASTLE_FORWARD<Callback>(callback));
        if (!next) { return castle::status::invalid_callback; }
        response_callback_ = CASTLE_MOVE(next);
        return castle::status::ok;
    }

    template <typename Callback>
    castle::status set_body_callback(Callback&& callback)
    {
        body_callback next(CASTLE_FORWARD<Callback>(callback));
        if (!next) { return castle::status::invalid_callback; }
        body_callback_ = CASTLE_MOVE(next);
        return castle::status::ok;
    }

    void clear_response_callback() CASTLE_NOEXCEPT { response_callback_.reset(); }
    void clear_body_callback() CASTLE_NOEXCEPT { body_callback_.reset(); }

    castle::status start() CASTLE_NOEXCEPT
    {
        reset();
        static char const client_preface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
        if (!enqueue_bytes(reinterpret_cast<uint8_t const*>(client_preface), 24U))
        {
            return fail_connection(error_code::output_full);
        }
        uint8_t settings[18U] = {
            0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U, // SETTINGS_HEADER_TABLE_SIZE
            0x00U, 0x02U, 0x00U, 0x00U, 0x00U, 0x00U, // SETTINGS_ENABLE_PUSH = 0
            0x00U, 0x05U, 0x00U, 0x00U, 0x40U, 0x00U  // SETTINGS_MAX_FRAME_SIZE
        };
        detail::write_u32(settings + 2U, static_cast<uint32_t>(MaxDynamicTableBytes));
        detail::write_u32(settings + 14U, static_cast<uint32_t>(MaxFrameSize));
        if (!enqueue_frame(http2_frame_type::settings, 0U, 0U, settings, sizeof(settings)))
        {
            return fail_connection(error_code::output_full);
        }
        started_ = true;
        return castle::status::ok;
    }

    void reset() CASTLE_NOEXCEPT
    {
        frame_parser_.reset();
        hpack_.clear_dynamic_table();
        for (size_type i = 0U; i < MaxConcurrentStreams; ++i) { streams_[i].clear(); }
        output_head_ = 0U;
        output_size_ = 0U;
        next_stream_id_ = 1U;
        connection_send_window_ = 65535;
        connection_receive_window_ = 65535;
        peer_initial_window_ = 65535U;
        peer_max_concurrent_streams_ = 0x7FFFFFFFU;
        peer_max_frame_size_ = 16384U;
        continuation_stream_id_ = 0U;
        goaway_received_ = false;
        started_ = false;
        failed_ = false;
        last_stream_id_ = 0x7FFFFFFFU;
        error_ = error_code::ok;
    }

    template <typename Headers>
    castle::status open_stream(method verb, string_view path, string_view authority,
                               Headers const& headers, string_view body, uint32_t& stream_id,
                               string_view custom_method = string_view()) CASTLE_NOEXCEPT
    {
        stream_id = 0U;
        if (!started_ || failed_) { return castle::status::not_configured; }
        if (goaway_received_) { return to_status(error_code::goaway_received); }
        if (active_stream_count() >= MaxConcurrentStreams || active_stream_count() >= peer_max_concurrent_streams_)
        {
            error_ = error_code::stream_limit;
            return to_status(error_);
        }
        if (next_stream_id_ > 0x7FFFFFFFU) { return fail_connection(error_code::protocol_error); }
        string_view token = verb == method::custom ? custom_method : method_token(verb);
        if (!detail::h2_token(token) || path.empty() || authority.empty()) { return castle::status::invalid_argument; }
        for (size_type i = 0U; i < authority.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(authority[i]);
            if (c <= 32U || c == 127U || c == '\r' || c == '\n') { return castle::status::invalid_argument; }
        }
        for (size_type i = 0U; i < path.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(path[i]);
            if (c <= 32U || c == 127U || c == '\r' || c == '\n') { return castle::status::invalid_argument; }
        }

        header_block_size_ = 0U;
        castle::status encoded = encode_header(string_view(":method"), token);
        if (!castle::succeeded(encoded)) { return encoded; }
        encoded = encode_header(string_view(":scheme"), string_view("http"));
        if (!castle::succeeded(encoded)) { return encoded; }
        encoded = encode_header(string_view(":authority"), authority);
        if (!castle::succeeded(encoded)) { return encoded; }
        encoded = encode_header(string_view(":path"), path);
        if (!castle::succeeded(encoded)) { return encoded; }
        for (size_type i = 0U; i < headers.size(); ++i)
        {
            header_view field = headers[i];
            if (field.name.empty() || field.name[0U] == ':' || !ascii_lowercase(field.name)) { return castle::status::invalid_argument; }
            if (ascii_iequal(field.name, string_view("te")) &&
                !ascii_iequal(field.value, string_view("trailers")))
            {
                return castle::status::invalid_argument;
            }
            if (ascii_iequal(field.name, string_view("host")) ||
                ascii_iequal(field.name, string_view("connection")) ||
                ascii_iequal(field.name, string_view("proxy-connection")) ||
                ascii_iequal(field.name, string_view("keep-alive")) ||
                ascii_iequal(field.name, string_view("transfer-encoding")) ||
                ascii_iequal(field.name, string_view("upgrade")))
            {
                return castle::status::invalid_argument;
            }
            encoded = encode_header(field.name, field.value);
            if (!castle::succeeded(encoded)) { return encoded; }
        }

        size_type fragments = (header_block_size_ + peer_max_frame_size_ - 1U) / peer_max_frame_size_;
        if (fragments == 0U) { fragments = 1U; }
        size_type required = header_block_size_ + fragments * 9U;
        if (output_free() < required) { error_ = error_code::output_full; return castle::status::full; }
        stream_record* stream = allocate_stream();
        if (stream == nullptr) { error_ = error_code::stream_limit; return castle::status::full; }
        stream_id = next_stream_id_;
        next_stream_id_ += 2U;
        stream->initialize(stream_id, peer_initial_window_, verb == method::head);
        stream->request_body = body;
        stream->request_body_sent = body.empty();

        size_type offset = 0U;
        for (size_type frame_index = 0U; frame_index < fragments; ++frame_index)
        {
            size_type fragment_size = min_size(header_block_size_ - offset, peer_max_frame_size_);
            uint8_t flags = 0U;
            if (frame_index + 1U == fragments) { flags = static_cast<uint8_t>(flags | 0x04U); }
            if (frame_index == 0U && body.empty()) { flags = static_cast<uint8_t>(flags | 0x01U); }
            http2_frame_type frame_type = frame_index == 0U ? http2_frame_type::headers : http2_frame_type::continuation;
            if (!enqueue_frame(frame_type, flags, stream_id, reinterpret_cast<uint8_t const*>(header_block_ + offset), fragment_size))
            {
                stream->clear();
                return fail_connection(error_code::output_full);
            }
            offset += fragment_size;
        }
        return pump_request_bodies();
    }

    castle::status feed(uint8_t const* data, size_type size, bool peer_closed = false) CASTLE_NOEXCEPT
    {
        if (!started_) { return castle::status::not_configured; }
        if (failed_) { return to_status(error_); }
        castle::status result = frame_parser_.feed(data, size,
            [this](http2_frame_view const& frame) { return handle_frame(frame); });
        if (!castle::succeeded(result))
        {
            if (error_ == error_code::ok)
            {
                error_ = frame_parser_.last_error() == error_code::ok ? error_code::protocol_error : frame_parser_.last_error();
            }
            return fail_connection(error_);
        }
        if (peer_closed) { return fail_connection(error_code::connection_closed); }
        return pump_request_bodies();
    }

    /** @brief Returns the next contiguous portion of queued bytes for the TCP sender. */
    uint8_t const* pending_output() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return output_size_ == 0U ? nullptr : output_ + output_head_;
    }

    size_type pending_output_size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type contiguous = MaxOutputSize - output_head_;
        return output_size_ < contiguous ? output_size_ : contiguous;
    }

    size_type output_size() CASTLE_CONST CASTLE_NOEXCEPT { return output_size_; }

    castle::status consume_output(size_type count) CASTLE_NOEXCEPT
    {
        if (count > pending_output_size()) { return castle::status::out_of_range; }
        output_head_ = (output_head_ + count) % MaxOutputSize;
        output_size_ -= count;
        if (output_size_ == 0U) { output_head_ = 0U; }
        return pump_request_bodies();
    }

    castle::status grant_connection_window(uint32_t increment) CASTLE_NOEXCEPT
    {
        if (increment == 0U || increment > 0x7FFFFFFFU || connection_receive_window_ + increment > 0x7FFFFFFFLL)
        {
            return fail_connection(error_code::flow_control_error);
        }
        uint8_t payload[4U];
        detail::write_u32(payload, increment);
        if (!enqueue_frame(http2_frame_type::window_update, 0U, 0U, payload, 4U))
        {
            error_ = error_code::output_full;
            return castle::status::full;
        }
        connection_receive_window_ += increment;
        return castle::status::ok;
    }

    castle::status grant_stream_window(uint32_t stream_id, uint32_t increment) CASTLE_NOEXCEPT
    {
        stream_record* stream = find_stream(stream_id);
        if (stream == nullptr || stream->response_complete || stream->reset) { return castle::status::not_found; }
        if (increment == 0U || increment > 0x7FFFFFFFU || stream->receive_window + increment > 0x7FFFFFFFLL)
        {
            return fail_connection(error_code::flow_control_error);
        }
        uint8_t payload[4U];
        detail::write_u32(payload, increment);
        if (!enqueue_frame(http2_frame_type::window_update, 0U, stream_id, payload, 4U))
        {
            error_ = error_code::output_full;
            return castle::status::full;
        }
        stream->receive_window += increment;
        return castle::status::ok;
    }

    castle::status release_stream(uint32_t stream_id) CASTLE_NOEXCEPT
    {
        stream_record* stream = find_stream(stream_id);
        if (stream == nullptr) { return castle::status::not_found; }
        if (!stream->response_complete && !stream->reset) { return castle::status::invalid_config; }
        stream->clear();
        return castle::status::ok;
    }

    response_view response(uint32_t stream_id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        stream_record const* stream = find_stream_const(stream_id);
        if (stream == nullptr) { return response_view{}; }
        return make_response(*stream);
    }

    bool started() CASTLE_CONST CASTLE_NOEXCEPT { return started_; }
    bool failed() CASTLE_CONST CASTLE_NOEXCEPT { return failed_; }
    bool goaway_received() CASTLE_CONST CASTLE_NOEXCEPT { return goaway_received_; }
    error_code last_error() CASTLE_CONST CASTLE_NOEXCEPT { return error_; }
    size_type active_stream_count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        size_type count = 0U;
        for (size_type i = 0U; i < MaxConcurrentStreams; ++i)
        {
            if (streams_[i].used && !streams_[i].response_complete && !streams_[i].reset) { ++count; }
        }
        return count;
    }

private:
    struct stream_record
    {
        uint32_t id = 0U;
        uint16_t status_code = 0U;
        uint64_t expected_body_length = 0U;
        int64_t send_window = 65535;
        int64_t receive_window = 65535;
        headers_type headers{};
        header_view header_views[MaxHeaders]{};
        char header_block[MaxHeaderBlockSize]{};
        char body[MaxBodySize]{};
        string_view request_body{};
        size_type header_block_size = 0U;
        size_type response_header_count = 0U;
        size_type body_size = 0U;
        size_type request_body_sent_bytes = 0U;
        uint64_t body_received = 0U;
        bool used = false;
        bool headers_received = false;
        bool end_stream_after_headers = false;
        bool response_complete = false;
        bool request_body_sent = false;
        bool request_head = false;
        bool content_length_seen = false;
        bool body_streamed = false;
        bool reset = false;

        void clear() CASTLE_NOEXCEPT
        {
            id = 0U; status_code = 0U; expected_body_length = 0U; send_window = 65535; receive_window = 65535;
            headers.clear(); header_block_size = 0U; response_header_count = 0U;
            body_size = 0U; request_body = string_view(); request_body_sent_bytes = 0U;
            body_received = 0U; used = false; headers_received = false;
            end_stream_after_headers = false; response_complete = false;
            request_body_sent = false; request_head = false; content_length_seen = false;
            body_streamed = false; reset = false;
        }

        void initialize(uint32_t stream_id, uint32_t initial_send_window, bool is_head_request) CASTLE_NOEXCEPT
        {
            clear(); id = stream_id; used = true; send_window = initial_send_window; request_head = is_head_request;
        }
    };

    static size_type min_size(size_type a, size_type b) CASTLE_NOEXCEPT { return a < b ? a : b; }
    static uint32_t min_u32(uint32_t a, uint32_t b) CASTLE_NOEXCEPT { return a < b ? a : b; }

    size_type output_free() CASTLE_CONST CASTLE_NOEXCEPT { return MaxOutputSize - output_size_; }

    bool enqueue_bytes(uint8_t const* data, size_type size) CASTLE_NOEXCEPT
    {
        if ((data == nullptr && size != 0U) || size > output_free()) { return false; }
        for (size_type i = 0U; i < size; ++i)
        {
            size_type tail = (output_head_ + output_size_) % MaxOutputSize;
            output_[tail] = data[i];
            ++output_size_;
        }
        return true;
    }

    bool enqueue_frame(http2_frame_type type, uint8_t flags, uint32_t stream_id,
                       uint8_t const* payload, size_type length) CASTLE_NOEXCEPT
    {
        if (length > 0xFFFFFFU || (payload == nullptr && length != 0U) || output_free() < length + 9U)
        {
            return false;
        }
        uint8_t header[9U]{};
        header[0U] = static_cast<uint8_t>(length >> 16U);
        header[1U] = static_cast<uint8_t>(length >> 8U);
        header[2U] = static_cast<uint8_t>(length);
        header[3U] = static_cast<uint8_t>(type);
        header[4U] = flags;
        detail::write_u32(header + 5U, stream_id & 0x7FFFFFFFU);
        return enqueue_bytes(header, sizeof(header)) && enqueue_bytes(payload, length);
    }

    castle::status encode_header(string_view name, string_view value) CASTLE_NOEXCEPT
    {
        size_type encoded_size = 0U;
        castle::status result = hpack_encoder::encode_literal(name, value,
            reinterpret_cast<uint8_t*>(header_block_) + header_block_size_,
            MaxHeaderBlockSize - header_block_size_, encoded_size);
        if (!castle::succeeded(result))
        {
            return result == castle::status::full ? to_status(error_code::header_limit) : result;
        }
        header_block_size_ += encoded_size;
        return castle::status::ok;
    }

    stream_record* allocate_stream() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxConcurrentStreams; ++i)
        {
            if (!streams_[i].used) { return &streams_[i]; }
        }
        return nullptr;
    }

    stream_record* find_stream(uint32_t id) CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxConcurrentStreams; ++i)
        {
            if (streams_[i].used && streams_[i].id == id) { return &streams_[i]; }
        }
        return nullptr;
    }

    stream_record const* find_stream_const(uint32_t id) CASTLE_CONST CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < MaxConcurrentStreams; ++i)
        {
            if (streams_[i].used && streams_[i].id == id) { return &streams_[i]; }
        }
        return nullptr;
    }

    response_view make_response(stream_record const& stream) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return response_view{stream.status_code, version::http_2, stream.header_views,
            stream.response_header_count, string_view(stream.body, stream.body_size),
            stream.id, !goaway_received_ && !failed_, stream.body_streamed};
    }

    castle::status pump_request_bodies() CASTLE_NOEXCEPT
    {
        if (failed_) { return to_status(error_); }
        for (size_type i = 0U; i < MaxConcurrentStreams; ++i)
        {
            stream_record& stream = streams_[i];
            while (stream.used && !stream.request_body_sent && !stream.response_complete &&
                   connection_send_window_ > 0 && stream.send_window > 0 && stream.request_body_sent_bytes < stream.request_body.size())
            {
                size_type remaining = stream.request_body.size() - stream.request_body_sent_bytes;
                size_type allowed = static_cast<size_type>(connection_send_window_ < stream.send_window ? connection_send_window_ : stream.send_window);
                allowed = min_size(allowed, static_cast<size_type>(peer_max_frame_size_));
                if (allowed > remaining) { allowed = remaining; }
                if (output_free() <= 9U) { return castle::status::ok; }
                if (allowed > output_free() - 9U) { allowed = output_free() - 9U; }
                if (allowed == 0U) { return castle::status::ok; }
                bool last = stream.request_body_sent_bytes + allowed == stream.request_body.size();
                uint8_t const* body = reinterpret_cast<uint8_t const*>(stream.request_body.data()) + stream.request_body_sent_bytes;
                if (!enqueue_frame(http2_frame_type::data, last ? 0x01U : 0U, stream.id, body, allowed))
                {
                    return castle::status::ok;
                }
                stream.request_body_sent_bytes += allowed;
                connection_send_window_ -= static_cast<int64_t>(allowed);
                stream.send_window -= static_cast<int64_t>(allowed);
                if (last) { stream.request_body_sent = true; }
            }
        }
        return castle::status::ok;
    }

    castle::status handle_frame(http2_frame_view const& frame) CASTLE_NOEXCEPT
    {
        if (continuation_stream_id_ != 0U && frame.type != http2_frame_type::continuation)
        {
            return fail_connection(error_code::protocol_error);
        }
        switch (static_cast<uint8_t>(frame.type))
        {
            case 0U: return handle_data(frame);
            case 1U: return handle_headers(frame);
            case 2U: return castle::status::ok; // PRIORITY has already passed its 5-byte length check.
            case 3U: return handle_rst_stream(frame);
            case 4U: return handle_settings(frame);
            case 5U: return fail_connection(error_code::protocol_error); // Server push is disabled for this client.
            case 6U: return handle_ping(frame);
            case 7U: return handle_goaway(frame);
            case 8U: return handle_window_update(frame);
            case 9U: return handle_continuation(frame);
            default: return castle::status::ok;
        }
    }

    castle::status handle_settings(http2_frame_view const& frame) CASTLE_NOEXCEPT
    {
        if ((frame.flags & 0x01U) != 0U) { return castle::status::ok; }
        for (size_type offset = 0U; offset < frame.length; offset += 6U)
        {
            uint16_t id = static_cast<uint16_t>((static_cast<uint16_t>(frame.payload[offset]) << 8U) | frame.payload[offset + 1U]);
            uint32_t value = detail::read_u32(frame.payload + offset + 2U);
            if (id == 2U) { return fail_connection(error_code::protocol_error); } // Server MUST NOT send ENABLE_PUSH.
            if (id == 3U) { peer_max_concurrent_streams_ = value; }
            else if (id == 4U)
            {
                if (value > 0x7FFFFFFFU) { return fail_connection(error_code::flow_control_error); }
                int64_t delta = static_cast<int64_t>(value) - static_cast<int64_t>(peer_initial_window_);
                for (size_type i = 0U; i < MaxConcurrentStreams; ++i)
                {
                    if (streams_[i].used && !streams_[i].response_complete && !streams_[i].reset)
                    {
                        streams_[i].send_window += delta;
                        if (streams_[i].send_window > 0x7FFFFFFFLL || streams_[i].send_window < -0x7FFFFFFFLL)
                        { return fail_connection(error_code::flow_control_error); }
                    }
                }
                peer_initial_window_ = value;
            }
            else if (id == 5U)
            {
                if (value < 16384U || value > 16777215U) { return fail_connection(error_code::protocol_error); }
                peer_max_frame_size_ = value;
            }
        }
        if (!enqueue_frame(http2_frame_type::settings, 0x01U, 0U, nullptr, 0U))
        {
            return fail_connection(error_code::output_full);
        }
        return pump_request_bodies();
    }

    castle::status handle_ping(http2_frame_view const& frame) CASTLE_NOEXCEPT
    {
        if ((frame.flags & 0x01U) == 0U &&
            !enqueue_frame(http2_frame_type::ping, 0x01U, 0U, frame.payload, 8U))
        {
            return fail_connection(error_code::output_full);
        }
        return castle::status::ok;
    }

    castle::status handle_goaway(http2_frame_view const& frame) CASTLE_NOEXCEPT
    {
        goaway_received_ = true;
        error_ = error_code::goaway_received;
        last_stream_id_ = detail::read_u32(frame.payload) & 0x7FFFFFFFU;
        for (size_type i = 0U; i < MaxConcurrentStreams; ++i)
        {
            stream_record& stream = streams_[i];
            if (stream.used && !stream.response_complete && !stream.reset && stream.id > last_stream_id_)
            {
                stream.reset = true;
                notify_stream(stream, castle::status::data_loss);
            }
        }
        return castle::status::ok;
    }

    castle::status handle_window_update(http2_frame_view const& frame) CASTLE_NOEXCEPT
    {
        uint32_t increment = detail::read_u32(frame.payload) & 0x7FFFFFFFU;
        if (increment == 0U) { return fail_connection(error_code::flow_control_error); }
        if (frame.stream_id == 0U)
        {
            connection_send_window_ += increment;
            if (connection_send_window_ > 0x7FFFFFFFLL) { return fail_connection(error_code::flow_control_error); }
        }
        else
        {
            stream_record* stream = find_stream(frame.stream_id);
            if (stream == nullptr) { return castle::status::ok; }
            stream->send_window += increment;
            if (stream->send_window > 0x7FFFFFFFLL) { return fail_connection(error_code::flow_control_error); }
        }
        return pump_request_bodies();
    }

    castle::status handle_rst_stream(http2_frame_view const& frame) CASTLE_NOEXCEPT
    {
        stream_record* stream = find_stream(frame.stream_id);
        if (stream == nullptr) { return castle::status::ok; }
        error_ = error_code::stream_reset;
        stream->reset = true;
        notify_stream(*stream, castle::status::data_loss);
        return castle::status::ok;
    }

    castle::status handle_headers(http2_frame_view const& frame) CASTLE_NOEXCEPT
    {
        stream_record* stream = find_stream(frame.stream_id);
        if (stream == nullptr || stream->response_complete || stream->reset) { return fail_connection(error_code::protocol_error); }
        size_type offset = 0U;
        size_type end = frame.length;
        if ((frame.flags & 0x08U) != 0U)
        {
            if (end == 0U) { return fail_connection(error_code::invalid_frame); }
            size_type padding = frame.payload[offset++];
            if (padding > end - offset) { return fail_connection(error_code::invalid_frame); }
            end -= padding;
        }
        if ((frame.flags & 0x20U) != 0U)
        {
            if (end - offset < 5U) { return fail_connection(error_code::invalid_frame); }
            offset += 5U;
        }
        if (end < offset) { return fail_connection(error_code::invalid_frame); }
        castle::status result = append_header_fragment(*stream, frame.payload + offset, end - offset);
        if (!castle::succeeded(result)) { return result; }
        stream->end_stream_after_headers = (frame.flags & 0x01U) != 0U;
        if ((frame.flags & 0x04U) != 0U) { return finish_header_block(*stream); }
        continuation_stream_id_ = stream->id;
        return castle::status::ok;
    }

    castle::status handle_continuation(http2_frame_view const& frame) CASTLE_NOEXCEPT
    {
        if (continuation_stream_id_ == 0U || frame.stream_id != continuation_stream_id_)
        {
            return fail_connection(error_code::protocol_error);
        }
        stream_record* stream = find_stream(frame.stream_id);
        if (stream == nullptr) { return fail_connection(error_code::protocol_error); }
        castle::status result = append_header_fragment(*stream, frame.payload, frame.length);
        if (!castle::succeeded(result)) { return result; }
        if ((frame.flags & 0x04U) != 0U)
        {
            continuation_stream_id_ = 0U;
            return finish_header_block(*stream);
        }
        return castle::status::ok;
    }

    castle::status append_header_fragment(stream_record& stream, uint8_t const* data, size_type size) CASTLE_NOEXCEPT
    {
        if (size > MaxHeaderBlockSize - stream.header_block_size)
        {
            stream.reset = true;
            error_ = error_code::header_limit;
            notify_stream(stream, to_status(error_code::header_limit));
            return to_status(error_code::header_limit);
        }
        for (size_type i = 0U; i < size; ++i) { stream.header_block[stream.header_block_size++] = static_cast<char>(data[i]); }
        return castle::status::ok;
    }

    castle::status finish_header_block(stream_record& stream) CASTLE_NOEXCEPT
    {
        bool const trailer_block = stream.headers_received;
        if (trailer_block && !stream.end_stream_after_headers)
        {
            return fail_connection(error_code::protocol_error);
        }
        decoded_headers_.clear();
        castle::status decoded = hpack_.decode_block(reinterpret_cast<uint8_t const*>(stream.header_block),
                                                       stream.header_block_size, decoded_headers_);
        stream.header_block_size = 0U;
        if (!castle::succeeded(decoded))
        {
            stream.reset = true;
            error_ = (decoded == castle::status::full || decoded == castle::status::out_of_range)
                ? error_code::header_limit : error_code::compression_error;
            notify_stream(stream, to_status(error_));
            return to_status(error_);
        }

        bool regular_seen = false;
        bool status_seen = false;
        for (size_type i = 0U; i < decoded_headers_.size(); ++i)
        {
            header_view header = decoded_headers_[i];
            bool pseudo = !header.name.empty() && header.name[0U] == ':';
            if (pseudo)
            {
                if (trailer_block || regular_seen || !ascii_iequal(header.name, string_view(":status")) || status_seen)
                {
                    return fail_connection(error_code::protocol_error);
                }
                status_seen = true;
                if (header.value.size() != 3U || header.value[0U] < '0' || header.value[0U] > '9' ||
                    header.value[1U] < '0' || header.value[1U] > '9' ||
                    header.value[2U] < '0' || header.value[2U] > '9')
                {
                    return fail_connection(error_code::protocol_error);
                }
                stream.status_code = static_cast<uint16_t>((header.value[0U] - '0') * 100 +
                    (header.value[1U] - '0') * 10 + (header.value[2U] - '0'));
                if (stream.status_code < 100U || stream.status_code > 599U || stream.status_code == 101U)
                { return fail_connection(error_code::protocol_error); }
            }
            else
            {
                regular_seen = true;
                if (!ascii_lowercase(header.name)) { return fail_connection(error_code::protocol_error); }
                if (ascii_iequal(header.name, string_view("connection")) ||
                    ascii_iequal(header.name, string_view("proxy-connection")) ||
                    ascii_iequal(header.name, string_view("keep-alive")) ||
                    ascii_iequal(header.name, string_view("transfer-encoding")) ||
                    ascii_iequal(header.name, string_view("upgrade")))
                {
                    return fail_connection(error_code::protocol_error);
                }
                if (ascii_iequal(header.name, string_view("content-length")))
                {
                    uint64_t content_length = 0U;
                    if (trailer_block || !detail::h2_parse_decimal(header.value, content_length) ||
                        (stream.content_length_seen && stream.expected_body_length != content_length))
                    {
                        return fail_connection(error_code::protocol_error);
                    }
                    stream.content_length_seen = true;
                    stream.expected_body_length = content_length;
                }
            }
        }
        if (!trailer_block && !status_seen) { return fail_connection(error_code::protocol_error); }

        for (size_type i = 0U; i < decoded_headers_.size(); ++i)
        {
            header_view header = decoded_headers_[i];
            castle::status added = stream.headers.add(header.name, header.value);
            if (!castle::succeeded(added))
            {
                stream.reset = true;
                error_ = error_code::header_limit;
                notify_stream(stream, to_status(error_));
                return to_status(error_);
            }
        }
        stream.headers_received = true;
        build_header_views(stream);
        if (!trailer_block && stream.status_code >= 100U && stream.status_code < 200U)
        {
            // Informational responses do not finish the stream; the next HEADERS block is the final response.
            if (stream.end_stream_after_headers) { return fail_connection(error_code::protocol_error); }
            stream.headers.clear();
            stream.response_header_count = 0U;
            stream.status_code = 0U;
            stream.headers_received = false;
            stream.content_length_seen = false;
            stream.expected_body_length = 0U;
            stream.end_stream_after_headers = false;
            return castle::status::ok;
        }
        if (stream.end_stream_after_headers)
        {
            stream.end_stream_after_headers = false;
            return finish_response(stream);
        }
        return castle::status::ok;
    }

    void build_header_views(stream_record& stream) CASTLE_NOEXCEPT
    {
        stream.response_header_count = 0U;
        for (size_type i = 0U; i < stream.headers.size(); ++i)
        {
            header_view field = stream.headers[i];
            if (!field.name.empty() && field.name[0U] == ':') { continue; }
            if (stream.response_header_count < MaxHeaders)
            {
                stream.header_views[stream.response_header_count++] = field;
            }
        }
    }

    castle::status handle_data(http2_frame_view const& frame) CASTLE_NOEXCEPT
    {
        stream_record* stream = find_stream(frame.stream_id);
        if (stream == nullptr || !stream->headers_received || stream->response_complete || stream->reset)
        { return fail_connection(error_code::protocol_error); }
        connection_receive_window_ -= static_cast<int64_t>(frame.length);
        stream->receive_window -= static_cast<int64_t>(frame.length);
        if (connection_receive_window_ < 0 || stream->receive_window < 0)
        { return fail_connection(error_code::flow_control_error); }

        size_type offset = 0U;
        size_type end = frame.length;
        if ((frame.flags & 0x08U) != 0U)
        {
            if (end == 0U) { return fail_connection(error_code::invalid_frame); }
            size_type padding = frame.payload[offset++];
            if (padding > end - offset) { return fail_connection(error_code::invalid_frame); }
            end -= padding;
        }
        if (end < offset) { return fail_connection(error_code::invalid_frame); }
        size_type data_size = end - offset;
        bool const no_body_semantics = stream->request_head || stream->status_code == 204U || stream->status_code == 304U;
        if (no_body_semantics && data_size != 0U) { return fail_connection(error_code::protocol_error); }
        if (stream->content_length_seen && !no_body_semantics &&
            (stream->body_received > stream->expected_body_length ||
             static_cast<uint64_t>(data_size) > stream->expected_body_length - stream->body_received))
        {
            return fail_connection(error_code::protocol_error);
        }
        if (data_size != 0U)
        {
            string_view chunk(reinterpret_cast<char const*>(frame.payload + offset), data_size);
            if (body_callback_) { body_callback_(stream->id, chunk); }
            size_type room = MaxBodySize - stream->body_size;
            size_type copy_size = min_size(room, data_size);
            for (size_type i = 0U; i < copy_size; ++i) { stream->body[stream->body_size++] = chunk[i]; }
            if (copy_size != data_size)
            {
                if (!body_callback_)
                {
                    stream->reset = true;
                    error_ = error_code::body_limit;
                    notify_stream(*stream, to_status(error_code::body_limit));
                    return to_status(error_code::body_limit);
                }
                stream->body_streamed = true;
            }
            stream->body_received += data_size;
        }
        if ((frame.flags & 0x01U) != 0U) { return finish_response(*stream); }
        return castle::status::ok;
    }

    castle::status finish_response(stream_record& stream) CASTLE_NOEXCEPT
    {
        bool const no_body_semantics = stream.request_head || stream.status_code == 204U || stream.status_code == 304U;
        if (stream.content_length_seen && !no_body_semantics && stream.body_received != stream.expected_body_length)
        {
            return fail_connection(error_code::protocol_error);
        }
        stream.response_complete = true;
        if (response_callback_)
        {
            response_view view = make_response(stream);
            response_callback_(castle::status::ok, view);
        }
        return castle::status::ok;
    }

    void notify_stream(stream_record& stream, castle::status result) CASTLE_NOEXCEPT
    {
        if (response_callback_)
        {
            response_view view = make_response(stream);
            response_callback_(result, view);
        }
    }

    castle::status fail_connection(error_code value) CASTLE_NOEXCEPT
    {
        error_ = value;
        failed_ = true;
        for (size_type i = 0U; i < MaxConcurrentStreams; ++i)
        {
            stream_record& stream = streams_[i];
            if (stream.used && !stream.response_complete && !stream.reset)
            {
                stream.reset = true;
                notify_stream(stream, to_status(value));
            }
        }
        return to_status(value);
    }

    frame_parser_type frame_parser_{};
    hpack_decoder<MaxDynamicTableBytes, MaxHeaderValueSize, MaxDynamicEntries> hpack_{};
    headers_type decoded_headers_{};
    stream_record streams_[MaxConcurrentStreams]{};
    response_callback response_callback_{};
    body_callback body_callback_{};
    char header_block_[MaxHeaderBlockSize]{};
    uint8_t output_[MaxOutputSize]{};
    size_type header_block_size_ = 0U;
    size_type output_head_ = 0U;
    size_type output_size_ = 0U;
    uint32_t next_stream_id_ = 1U;
    uint32_t peer_initial_window_ = 65535U;
    uint32_t peer_max_concurrent_streams_ = 0x7FFFFFFFU;
    uint32_t peer_max_frame_size_ = 16384U;
    uint32_t continuation_stream_id_ = 0U;
    uint32_t last_stream_id_ = 0x7FFFFFFFU;
    int64_t connection_send_window_ = 65535;
    int64_t connection_receive_window_ = 65535;
    bool goaway_received_ = false;
    bool started_ = false;
    bool failed_ = false;
    error_code error_ = error_code::ok;
};

} // namespace http
} // namespace comm
} // namespace castle

#endif // CASTLE_EXT_COMM_HTTP_HTTP2_HPP

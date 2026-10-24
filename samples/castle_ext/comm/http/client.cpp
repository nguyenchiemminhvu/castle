#include "sample_support.hpp"
#include "castle_ext/comm/http/client.hpp"

#include <stdint.h>

namespace
{
using castle::size_type;
using castle::status;

struct fake_socket
{
    status finish_connect(int, int*) { return status::ok; }
    status wait_readable(int, int*) { return input_size > input_offset ? status::ok : status::time_out; }
    size_type input_size = 0U;
    size_type input_offset = 0U;
};

struct fake_transport
{
    fake_socket& socket() { return socket_; }
    status connect(castle::comm::net::endpoint const&, int, bool, int*) { open_ = true; return status::ok; }
    status send_some(void const* data, size_type size, size_type& sent, int*)
    {
        sent = size < 13U ? size : 13U;
        const uint8_t* bytes = static_cast<uint8_t const*>(data);
        for (size_type i = 0U; i < sent && sent_size_ < sizeof(sent_); ++i) { sent_[sent_size_++] = bytes[i]; }
        return status::ok;
    }
    status receive_some(void* output, size_type capacity, size_type& received, int*, bool* peer_closed)
    {
        received = 0U;
        if (peer_closed != nullptr) { *peer_closed = false; }
        size_type available = socket_.input_size - socket_.input_offset;
        received = available < capacity ? available : capacity;
        uint8_t* bytes = static_cast<uint8_t*>(output);
        for (size_type i = 0U; i < received; ++i) { bytes[i] = input_[socket_.input_offset++]; }
        return received == 0U ? status::empty : status::ok;
    }
    status close(int*) { open_ = false; return status::ok; }
    void set_response(char const* response, size_type size)
    {
        socket_.input_offset = 0U;
        socket_.input_size = size;
        for (size_type i = 0U; i < size; ++i) { input_[i] = static_cast<uint8_t>(response[i]); }
    }
    bool sent_contains(char const* token) const
    {
        size_type token_size = 0U; while (token[token_size] != '\0') { ++token_size; }
        if (token_size > sent_size_) { return false; }
        for (size_type i = 0U; i + token_size <= sent_size_; ++i)
        {
            size_type j = 0U; while (j < token_size && sent_[i + j] == static_cast<uint8_t>(token[j])) { ++j; }
            if (j == token_size) { return true; }
        }
        return false;
    }
    fake_socket socket_{};
    uint8_t input_[128U]{};
    uint8_t sent_[1024U]{};
    size_type sent_size_ = 0U;
    bool open_ = false;
};
}

int main()
{
    using namespace castle::comm;
    const net::endpoint remote = net::endpoint::ipv4(192U, 0U, 2U, 10U, 80U);

    fake_transport http1_transport;
    const char reply[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok";
    http1_transport.set_response(reply, sizeof(reply) - 1U);
    http::http1_client<fake_transport> http1(http1_transport);
    size_type completed = 0U;
    CASTLE_SAMPLE_CHECK(http1.set_response_callback([&completed](castle::status result, http::response_view const& response)
    {
        if (result == castle::status::ok && response.status_code == 200U && response.body.size() == 2U) { ++completed; }
    }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(http1.connect(remote, "example.test:80") == castle::status::ok);
    http::http1_client<fake_transport>::headers_type request_headers;
    CASTLE_SAMPLE_CHECK(http1.request(http::method::get, "/", request_headers) == castle::status::ok);
    for (size_type i = 0U; i < 64U && http1.busy(); ++i) { (void)http1.poll(0); }
    CASTLE_SAMPLE_CHECK(completed == 1U);
    CASTLE_SAMPLE_CHECK(http1_transport.sent_contains("GET / HTTP/1.1"));
    CASTLE_SAMPLE_CHECK(http1.close() == castle::status::ok);

    fake_transport http2_transport;
    http::http2_client<fake_transport, 2U, 512U, 16384U, 128U> http2(http2_transport);
    CASTLE_SAMPLE_CHECK(http2.connect(remote, "example.test:80") == castle::status::ok);
    http::http2_client<fake_transport, 2U, 512U, 16384U, 128U>::headers_type h2_headers;
    uint32_t stream_id = 0U;
    CASTLE_SAMPLE_CHECK(http2.open_stream(http::method::get, "/", h2_headers, http::string_view(), stream_id) == castle::status::ok);
    (void)http2.poll(0);
    CASTLE_SAMPLE_CHECK(stream_id == 1U);
    CASTLE_SAMPLE_CHECK(http2_transport.sent_contains("PRI * HTTP/2.0"));
    CASTLE_SAMPLE_CHECK(http2.close() == castle::status::ok);
    return 0;
}

#include <gtest/gtest.h>
#include "castle/core/traits.hpp"
#include "castle_ext/protocols/http/client.hpp"

using castle::size_type;
using castle::status;
using castle::net::endpoint;

namespace
{
struct fake_socket
{
    status finish_connect(int, int*) { return finish_result; }
    status wait_readable(int, int*) { return input_offset < input_size || peer_closed ? status::ok : status::time_out; }
    status finish_result = status::ok;
    size_type input_size = 0U;
    size_type input_offset = 0U;
    bool peer_closed = false;
};

struct fake_transport
{
    status connect(endpoint const&, int, bool, int*) { open = true; return connect_result; }
    status send_some(void const* data, size_type size, size_type& sent, int*)
    {
        sent = size < max_send ? size : max_send;
        const uint8_t* bytes = static_cast<uint8_t const*>(data);
        if (sent > sizeof(output) - output_size) { sent = sizeof(output) - output_size; }
        for (size_type i = 0U; i < sent; ++i) { output[output_size++] = bytes[i]; }
        return sent == 0U ? status::full : status::ok;
    }
    status receive_some(void* data, size_type capacity, size_type& received, int*, bool* closed)
    {
        size_type available = socket_state.input_size - socket_state.input_offset;
        received = available < capacity ? available : capacity;
        uint8_t* output_bytes = static_cast<uint8_t*>(data);
        for (size_type i = 0U; i < received; ++i) { output_bytes[i] = input[socket_state.input_offset++]; }
        if (closed != nullptr) { *closed = socket_state.peer_closed && socket_state.input_offset == socket_state.input_size; }
        return received == 0U ? status::empty : status::ok;
    }
    status close(int*) { open = false; ++close_count; return status::ok; }
    fake_socket& socket() { return socket_state; }
    void set_input(char const* value, size_type size)
    {
        socket_state.input_offset = 0U; socket_state.input_size = size;
        for (size_type i = 0U; i < size; ++i) { input[i] = static_cast<uint8_t>(value[i]); }
    }
    bool contains(char const* value) const
    {
        size_type length = 0U; while (value[length] != '\0') { ++length; }
        if (length > output_size) { return false; }
        for (size_type i = 0U; i + length <= output_size; ++i)
        {
            size_type j = 0U; while (j < length && output[i + j] == static_cast<uint8_t>(value[j])) { ++j; }
            if (j == length) { return true; }
        }
        return false;
    }
    fake_socket socket_state{};
    uint8_t input[256U]{};
    uint8_t output[4096U]{};
    size_type output_size = 0U;
    size_type max_send = 5U;
    status connect_result = status::ok;
    bool open = false;
    size_type close_count = 0U;
};

static_assert(castle::meta::is_same<castle::protocols::http::basic_http_client<fake_transport>,
                                    castle::protocols::http::http1_client<fake_transport>>::value,
              "HTTP/1.1 is the default policy");
static_assert(castle::protocols::http::protocol_policy_traits<castle::protocols::http::http2_policy>::multiplexed,
              "HTTP/2 policy is multiplexed");
}

TEST(HttpClient, Http1ConnectSendReceiveAndCallback)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    const char reply[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nok";
    transport.set_input(reply, sizeof(reply) - 1U);
    http1_client<fake_transport> client(transport);
    size_type callbacks = 0U;
    EXPECT_EQ(client.set_response_callback([&callbacks](castle::status result, response_view const& response)
    {
        EXPECT_EQ(result, castle::status::ok);
        EXPECT_EQ(response.status_code, 200U);
        EXPECT_EQ(response.body.size(), 2U);
        ++callbacks;
    }), castle::status::ok);
    EXPECT_EQ(client.connect(endpoint::ipv4(192U, 0U, 2U, 1U, 80U), "api.test:80"), castle::status::ok);
    http1_client<fake_transport>::headers_type headers;
    EXPECT_EQ(client.request(method::get, "/status", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_EQ(callbacks, 1U);
    EXPECT_TRUE(transport.contains("GET /status HTTP/1.1"));
    EXPECT_TRUE(transport.contains("Host: api.test:80"));
    EXPECT_FALSE(client.busy());
}

TEST(HttpClient, Http1HonorsExplicitRequestConnectionClose)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    const char reply[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    transport.set_input(reply, sizeof(reply) - 1U);
    http1_client<fake_transport> client(transport);
    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U), "local.test:80"), castle::status::ok);
    typename http1_client<fake_transport>::headers_type headers;
    ASSERT_EQ(headers.add("Connection", "close"), castle::status::ok);
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_FALSE(client.connected());
    EXPECT_EQ(transport.close_count, 1U);
}

TEST(HttpClient, Http1NonblockingConnectTimeoutThenCompletes)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    transport.connect_result = castle::status::time_out;
    http1_client<fake_transport> client(transport);
    EXPECT_EQ(client.connect(endpoint::ipv4(198U, 51U, 100U, 3U, 80U)), castle::status::time_out);
    EXPECT_FALSE(client.connected());
    EXPECT_EQ(client.poll(10), castle::status::ok);
    EXPECT_TRUE(client.connected());
    EXPECT_EQ(client.close(), castle::status::ok);
}

TEST(HttpClient, Http1HonorsPeerCloseAndReceiveErrors)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    transport.socket_state.peer_closed = true;
    http1_client<fake_transport> client(transport);
    castle::status callback_status = castle::status::ok;
    EXPECT_EQ(client.set_response_callback([&callback_status](castle::status result, response_view const&) { callback_status = result; }), castle::status::ok);
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok);
    typename http1_client<fake_transport>::headers_type headers;
    EXPECT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_EQ(callback_status, castle::status::data_loss);
    EXPECT_EQ(client.close(), castle::status::ok);
}

TEST(HttpClient, Http2SendsPrefaceAndOpensMultiplexedStream)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http2_client<fake_transport, 2U, 512U, 16384U, 128U> client(transport);
    EXPECT_EQ(client.connect(endpoint::ipv4(203U, 0U, 113U, 8U, 80U), "h2.test:80"), castle::status::ok);
    typename decltype(client)::headers_type headers;
    uint32_t first = 0U;
    uint32_t second = 0U;
    EXPECT_EQ(client.open_stream(method::get, "/one", headers, string_view(), first), castle::status::ok);
    EXPECT_EQ(client.open_stream(method::get, "/two", headers, string_view(), second), castle::status::ok);
    EXPECT_EQ(first, 1U);
    EXPECT_EQ(second, 3U);
    EXPECT_EQ(client.open_stream(method::get, "/three", headers, string_view(), second), castle::status::out_of_range);
    EXPECT_EQ(client.poll(0), castle::status::time_out); // writes are drained; no server frame is ready
    EXPECT_TRUE(transport.contains("PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"));
    EXPECT_TRUE(transport.output_size > 24U);
    EXPECT_EQ(client.close(), castle::status::ok);
}

TEST(HttpClient, Http2ReadsWhileSocketWriteIsBackpressured)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    transport.max_send = 0U;
    http2_client<fake_transport> client(transport);
    ASSERT_EQ(client.connect(endpoint::ipv4(203U, 0U, 113U, 11U, 80U), "h2.test:80"), castle::status::ok);
    EXPECT_EQ(client.poll(0), castle::status::time_out);
    EXPECT_TRUE(client.session().output_size() != 0U);
    EXPECT_TRUE(client.connected());
}

TEST(HttpClient, Http2ConnectTimeoutIsPolledBeforeSessionStart)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    transport.connect_result = castle::status::time_out;
    http2_client<fake_transport> client(transport);
    EXPECT_EQ(client.connect(endpoint::ipv4(203U, 0U, 113U, 9U, 80U)), castle::status::time_out);
    EXPECT_FALSE(client.connected());
    EXPECT_EQ(client.poll(10), castle::status::time_out);
    EXPECT_TRUE(client.connected());
    EXPECT_TRUE(client.session().started());
}

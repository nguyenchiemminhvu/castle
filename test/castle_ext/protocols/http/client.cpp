#include <gtest/gtest.h>
#include <cstring>
#include <string>
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
    status wait_readable(int, int*) { return ready_error != status::ok ? ready_error : (input_offset < input_size || peer_closed || force_ready ? status::ok : status::time_out); }
    status finish_result = status::ok;
    status ready_error = status::ok;
    size_type input_size = 0U;
    size_type input_offset = 0U;
    bool peer_closed = false;
    bool force_ready = false;
};

struct fake_transport
{
    status connect(endpoint const&, int, bool, int*) { open = true; return connect_result; }
    status send_some(void const* data, size_type size, size_type& sent, int*)
    {
        sent = 0U;
        if (send_result != status::ok)
        {
            return send_result;
        }
        if (send_zero_ok)
        {
            return status::ok;
        }
        sent = size < max_send ? size : max_send;
        const uint8_t* bytes = static_cast<uint8_t const*>(data);
        if (sent > sizeof(output) - output_size) { sent = sizeof(output) - output_size; }
        for (size_type i = 0U; i < sent; ++i) { output[output_size++] = bytes[i]; }
        return sent == 0U ? status::full : status::ok;
    }
    status receive_some(void* data, size_type capacity, size_type& received, int*, bool* closed)
    {
        received = 0U;
        if (receive_result != status::ok)
        {
            return receive_result;
        }
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
    status send_result = status::ok;
    bool send_zero_ok = false;
    status receive_result = status::ok;
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

TEST(HttpClient, Http1AuthorityDefaultsToNumericEndpointAndRejectsInvalidText)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http1_client<fake_transport> client(transport);
    EXPECT_EQ(client.connect(endpoint::ipv4(192U, 0U, 2U, 1U, 8080U), "bad host"), castle::status::invalid_argument);
    ASSERT_EQ(client.connect(endpoint::ipv4(192U, 0U, 2U, 1U, 8080U)), castle::status::ok);
    typename http1_client<fake_transport>::headers_type headers;
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_TRUE(transport.contains("Host: 192.0.2.1:8080"));
}

TEST(HttpClient, Http1CanReconnectAfterFailureAndClosesOnTrailingBytes)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    const char garbage[] = "garbage\r\n\r\n";
    transport.set_input(garbage, sizeof(garbage) - 1U);
    http1_client<fake_transport> client(transport);
    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok);
    typename http1_client<fake_transport>::headers_type headers;
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_FALSE(client.connected());
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok);
    EXPECT_TRUE(client.connected());

    const char reply[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nokEXTRA";
    transport.output_size = 0U;
    transport.set_input(reply, sizeof(reply) - 1U);
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_FALSE(client.connected()); // pipelined bytes make the connection unsafe to reuse
}

TEST(HttpClient, PolicySelectsClientType)
{
    using namespace castle::protocols::http;
    static_assert(castle::meta::is_same<basic_http_client<fake_transport, http2_policy>, http2_client<fake_transport>>::value,
                  "HTTP/2 policy selects the HTTP/2 client");
    static_assert(!protocol_policy_traits<http1_policy>::multiplexed, "HTTP/1.1 is not multiplexed");
    EXPECT_EQ(protocol_policy_traits<http2_policy>::protocol, version::http_2);
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

TEST(HttpClient, Http1ConnectRejectsSecondConnectAndBusyRequest)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http1_client<fake_transport> client(transport);
    http1_client<fake_transport>::headers_type headers;
    EXPECT_EQ(client.request(method::get, "/", headers), castle::status::not_configured);
    ASSERT_EQ(client.connect(endpoint::ipv4(192U, 0U, 2U, 1U, 80U)), castle::status::ok);
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::invalid_config);
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    EXPECT_EQ(client.request(method::get, "/", headers), castle::status::not_configured);
    EXPECT_EQ(client.close(), castle::status::ok);
}

TEST(HttpClient, Http1ValidatesAuthorityCallbackAndIdlePoll)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http1_client<fake_transport> client(transport);
    EXPECT_EQ(client.poll(0), castle::status::not_configured);
    std::string long_authority(64U, 'a');
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U), string_view(long_authority.data(), long_authority.size())),
              castle::status::out_of_range);
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U), string_view()), castle::status::invalid_argument);
    EXPECT_EQ(client.set_response_callback(static_cast<void (*)(castle::status, response_view const&)>(nullptr)),
              castle::status::invalid_callback);
    EXPECT_EQ(client.set_body_callback(static_cast<void (*)(string_view)>(nullptr)), castle::status::invalid_callback);
    uint8_t v6[16] = {0x20U, 0x01U, 0x0dU, 0xb8U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U};
    ASSERT_EQ(client.connect(endpoint::ipv6(v6, 80U)), castle::status::ok);
    http1_client<fake_transport>::headers_type headers;
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_TRUE(transport.contains("Host: [2001:db8:0:0:0:0:0:1]:80"));
}

TEST(HttpClient, Http1RequestBuildFailureAndConnectionHeaderSelection)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http1_client<fake_transport> client(transport);
    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U), "local.test:80"), castle::status::ok);
    http1_client<fake_transport>::headers_type headers;
    EXPECT_EQ(client.request(method::custom, "/", headers, string_view(), string_view("bad token")), castle::status::invalid_argument);
    ASSERT_EQ(headers.add("Accept", "text/plain"), castle::status::ok);
    ASSERT_EQ(headers.add("Connection", "keep-alive"), castle::status::ok);
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_TRUE(transport.contains("Connection: keep-alive"));
    EXPECT_TRUE(client.connected());
}

TEST(HttpClient, Http1SendReceiveAndSocketErrorsFailTheRequest)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http1_client<fake_transport> client(transport);
    castle::status callback_status = castle::status::ok;
    ASSERT_EQ(client.set_response_callback([&callback_status](castle::status result, response_view const&) { callback_status = result; }), castle::status::ok);
    http1_client<fake_transport>::headers_type headers;

    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok);
    transport.send_result = castle::status::system_call_error;
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    EXPECT_EQ(client.poll(0), castle::status::system_call_error);
    EXPECT_EQ(callback_status, castle::status::system_call_error);
    EXPECT_FALSE(client.connected());
    transport.send_result = castle::status::ok;
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok); // a failed client may reconnect

    const char reply[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    transport.set_input(reply, sizeof(reply) - 1U);
    transport.receive_result = castle::status::system_call_error;
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_EQ(callback_status, castle::status::system_call_error);
    transport.receive_result = castle::status::ok;
    EXPECT_EQ(client.close(), castle::status::ok);

    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok);
    transport.socket().ready_error = castle::status::system_call_error;
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    castle::status last = castle::status::ok;
    for (size_type i = 0U; i < 128U; ++i) { last = client.poll(0); }
    EXPECT_EQ(last, castle::status::system_call_error);
    transport.socket().ready_error = castle::status::ok;
    EXPECT_EQ(client.close(), castle::status::ok);

    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok);
    transport.set_input(reply, 0U);
    transport.socket().force_ready = true; // readable, but the receive yields no bytes
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U; ++i) { last = client.poll(0); }
    EXPECT_EQ(last, castle::status::empty);
    EXPECT_TRUE(client.busy());
    EXPECT_EQ(client.close(), castle::status::ok);
}

TEST(HttpClient, Http1ResumesPartialResponseAcrossPolls)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http1_client<fake_transport> client(transport);
    size_type callbacks = 0U;
    ASSERT_EQ(client.set_response_callback([&callbacks](castle::status, response_view const& response)
    {
        EXPECT_EQ(response.body.size(), 4U);
        ++callbacks;
    }), castle::status::ok);
    const char first[] = "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\nab";
    const char second[] = "cd";
    transport.set_input(first, sizeof(first) - 1U);
    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok);
    http1_client<fake_transport>::headers_type headers;
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U; ++i) { (void)client.poll(0); }
    EXPECT_EQ(callbacks, 0U);
    EXPECT_TRUE(client.busy());
    transport.set_input(second, sizeof(second) - 1U);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_EQ(callbacks, 1U);
}

TEST(HttpClient, Http1ClosesOnPeerCloseWithCompleteResponseOrKeepAliveOff)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http1_client<fake_transport> client(transport);
    http1_client<fake_transport>::headers_type headers;
    const char reply[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    transport.set_input(reply, sizeof(reply) - 1U);
    transport.socket().peer_closed = true;
    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok);
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_FALSE(client.connected());

    transport.socket().peer_closed = false;
    transport.set_input(reply, sizeof(reply) - 1U);
    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok);
    ASSERT_EQ(client.request(method::get, "/", headers, string_view(), string_view(), false), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_FALSE(client.connected());
}

TEST(HttpClient, Http1ConnectErrorsAndConnectingStates)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http1_client<fake_transport> client(transport);
    transport.connect_result = castle::status::system_call_error;
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::system_call_error);
    EXPECT_FALSE(client.busy());
    transport.connect_result = castle::status::time_out;
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::time_out);
    EXPECT_TRUE(client.busy());
    transport.socket().finish_result = castle::status::time_out;
    EXPECT_EQ(client.poll(0), castle::status::time_out);
    EXPECT_TRUE(client.busy());
    transport.socket().finish_result = castle::status::ok;
    EXPECT_EQ(client.poll(0), castle::status::ok);
    EXPECT_TRUE(client.connected());
    EXPECT_FALSE(client.busy());
}

TEST(HttpClient, Http2AuthorityConnectErrorsAndIdleStates)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http2_client<fake_transport> client(transport);
    EXPECT_EQ(client.poll(0), castle::status::not_configured);
    http2_client<fake_transport>::headers_type headers;
    uint32_t stream = 0U;
    EXPECT_EQ(client.open_stream(method::get, "/", headers, string_view(), stream), castle::status::not_configured);
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U), "bad host"), castle::status::invalid_argument);
    std::string long_authority(64U, 'a');
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U), string_view(long_authority.data(), long_authority.size())),
              castle::status::out_of_range);
    transport.connect_result = castle::status::system_call_error;
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::system_call_error);
    EXPECT_FALSE(client.connected());
    transport.connect_result = castle::status::time_out;
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U), "h2.test:80"), castle::status::time_out);
    transport.socket().finish_result = castle::status::time_out;
    EXPECT_EQ(client.poll(0), castle::status::time_out);
    transport.socket().finish_result = castle::status::ok;
    EXPECT_EQ(client.poll(0), castle::status::time_out); // session started; no server frame yet
    EXPECT_TRUE(client.connected());
}

TEST(HttpClient, Http2FlushReceiveAndFeedErrorPaths)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http2_client<fake_transport> client(transport);
    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U), "h2.test:80"), castle::status::ok);
    http2_client<fake_transport>::headers_type headers;
    uint32_t stream = 0U;
    EXPECT_EQ(client.open_stream(method::get, "/", string_view(), headers, string_view(), stream), castle::status::ok);

    transport.send_result = castle::status::system_call_error;
    EXPECT_EQ(client.poll(0), castle::status::system_call_error);
    transport.send_result = castle::status::full;
    EXPECT_EQ(client.poll(0), castle::status::time_out);
    transport.send_result = castle::status::empty;
    EXPECT_EQ(client.poll(0), castle::status::time_out);
    transport.send_result = castle::status::ok;
    transport.send_zero_ok = true;
    EXPECT_EQ(client.poll(0), castle::status::time_out);
    transport.send_zero_ok = false;
    EXPECT_EQ(client.poll(0), castle::status::time_out);

    transport.socket().ready_error = castle::status::system_call_error;
    EXPECT_EQ(client.poll(0), castle::status::system_call_error);
    transport.socket().ready_error = castle::status::ok;

    transport.receive_result = castle::status::system_call_error;
    transport.socket().force_ready = true;
    EXPECT_EQ(client.poll(0), castle::status::system_call_error);
    transport.receive_result = castle::status::ok;
    EXPECT_EQ(client.poll(0), castle::status::empty);
    transport.socket().force_ready = false;

    const uint8_t bad_frame[9] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U}; // DATA on stream 0
    transport.set_input(reinterpret_cast<char const*>(bad_frame), sizeof(bad_frame));
    EXPECT_EQ(client.poll(0), castle::status::invalid_argument);
    EXPECT_FALSE(client.connected());
}

TEST(HttpClient, Http1ReportsStateAndSendBackpressure)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http1_client<fake_transport> client(transport);
    http1_client<fake_transport>::headers_type headers;
    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok);
    EXPECT_EQ(client.poll(0), castle::status::ok);
    EXPECT_EQ(client.connect(endpoint::loopback_ipv4(80U), "local.test:80"), castle::status::invalid_config);
    transport.send_result = castle::status::full;
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    EXPECT_EQ(client.poll(0), castle::status::full);
    transport.send_result = castle::status::empty;
    EXPECT_EQ(client.poll(0), castle::status::empty);
    transport.send_result = castle::status::ok;
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_EQ(client.close(), castle::status::ok);

    const char closing[] = "HTTP/1.1 200 OK\r\nConnection: close\r\nContent-Length: 0\r\n\r\n";
    transport.set_input(closing, sizeof(closing) - 1U);
    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U)), castle::status::ok);
    ASSERT_EQ(client.request(method::get, "/", headers), castle::status::ok);
    for (size_type i = 0U; i < 128U && client.busy(); ++i) { (void)client.poll(0); }
    EXPECT_FALSE(client.connected());
}

TEST(HttpClient, Http2PeerCloseEndsTheConnection)
{
    using namespace castle::protocols::http;
    fake_transport transport;
    http2_client<fake_transport> client(transport);
    ASSERT_EQ(client.connect(endpoint::loopback_ipv4(80U), "h2.test:80"), castle::status::ok);
    transport.socket().peer_closed = true;
    EXPECT_EQ(client.poll(0), castle::status::data_loss);
    EXPECT_FALSE(client.connected());
}

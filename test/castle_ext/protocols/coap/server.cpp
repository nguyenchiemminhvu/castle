#include <gtest/gtest.h>
#include "castle_ext/net/client.hpp"
#include "castle_ext/protocols/coap/server.hpp"

using namespace castle::protocols::coap;
using castle::size_type;

namespace {

using server_t = basic_server<128U, 2U, 5U, 4U, 2U, 4U, 16U, 32U, 32U, 64U>;
using message_t = server_t::message_type_t;
using peer_t = castle::net::udp_client<128U>;
using castle::net::endpoint;

message_t make_request(message_type type, code method, uint16_t id, char const* path, uint8_t token)
{
    message_t request;
    request.type = type;
    request.code_value = static_cast<uint8_t>(method);
    request.message_id = id;
    uint8_t bytes[1] = {token};
    (void)request.set_token(bytes, 1U);
    (void)append_uri_path(request, path);
    return request;
}

message_t make_control(message_type type, uint16_t id)
{
    message_t value;
    value.type = type;
    value.code_value = static_cast<uint8_t>(code::empty);
    value.message_id = id;
    return value;
}

bool peer_receive(peer_t& peer, message_t& out, endpoint& source)
{
    uint8_t buffer[128]{};
    size_type received = 0U;
    if (peer.socket().wait_readable(1000) != castle::status::ok)
    {
        return false;
    }
    if (!castle::succeeded(peer.receive_from(buffer, sizeof(buffer), received, source)))
    {
        return false;
    }
    decode_error reason = decode_error::none;
    return castle::succeeded(decode_message(buffer, received, out, &reason));
}

bool peer_has_datagram(peer_t& peer)
{
    return peer.socket().wait_readable(50) == castle::status::ok;
}

void peer_send(peer_t& peer, message_t const& value, endpoint const& to)
{
    uint8_t wire[128]{};
    size_type size = 0U;
    size_type sent = 0U;
    ASSERT_EQ(encode_message(value, wire, sizeof(wire), size), castle::status::ok);
    ASSERT_EQ(peer.send_to(wire, size, to, sent), castle::status::ok);
}

void peer_send_raw(peer_t& peer, uint8_t const* bytes, size_type size, endpoint const& to)
{
    size_type sent = 0U;
    ASSERT_EQ(peer.send_to(bytes, size, to, sent), castle::status::ok);
}

void server_process(server_t& server, uint32_t now)
{
    ASSERT_EQ(server.process_once(now, 1000), castle::status::ok);
}

} // namespace

TEST(CoapServer, RegistersResourcesAndMethodMasks)
{
    basic_server<128U, 2U, 2U, 4U, 2U, 4U, 16U, 32U, 32U, 64U> server;
    int reading = 0;
    EXPECT_EQ(server.register_resource(nullptr, allow_get, [](auto const&, auto&) {}), castle::status::invalid_argument);
    EXPECT_EQ(server.register_resource("bad", 0U, [](auto const&, auto&) {}), castle::status::invalid_argument);
    EXPECT_EQ(server.register_resource("sensors/temp", allow_get,
        [&reading](auto const&, auto& response) { ++reading; response.code_value = static_cast<uint8_t>(code::content); }), castle::status::ok);
    EXPECT_EQ(server.register_resource("actuators/led", allow_put,
        [](auto const&, auto& response) { response.code_value = static_cast<uint8_t>(code::changed); }), castle::status::ok);
    EXPECT_EQ(server.resource_count(), 2U);
    EXPECT_EQ(server.set_resource_max_age("/sensors/temp", 5U), castle::status::ok);
    EXPECT_EQ(server.set_resource_max_age("missing", 5U), castle::status::not_found);
    EXPECT_EQ(server.register_resource("actuators/led", allow_put,
        [](auto const&, auto& response) { response.code_value = static_cast<uint8_t>(code::changed); }), castle::status::ok);
    EXPECT_EQ(server.resource_count(), 2U);
    EXPECT_EQ(server.register_resource("third", allow_get, [](auto const&, auto&) {}), castle::status::full);
    EXPECT_EQ(server.unregister_resource("/sensors/temp"), castle::status::ok);
    EXPECT_EQ(server.unregister_resource("/sensors/temp"), castle::status::not_found);
    EXPECT_EQ(server.resource_count(), 1U);
    EXPECT_EQ(reading, 0);
}

TEST(CoapServer, ResourceCallbackCanReplaceItselfForNextRequest)
{
    using test_server = basic_server<128U, 2U, 2U, 4U, 2U, 4U, 16U, 32U, 32U, 64U>;
    test_server server;
    size_type first_calls = 0U;
    size_type second_calls = 0U;
    ASSERT_EQ(server.open(castle::net::endpoint::loopback_ipv4(56832U)), castle::status::ok);
    ASSERT_EQ(server.register_resource("sensors/temp", allow_get,
        [&](test_server::message_type_t const&, test_server::message_type_t& response)
        {
            ++first_calls;
            response.code_value = static_cast<uint8_t>(code::content);
            EXPECT_EQ(server.register_resource("sensors/temp", allow_get,
                [&second_calls](test_server::message_type_t const&, test_server::message_type_t& next)
                {
                    ++second_calls;
                    next.code_value = static_cast<uint8_t>(code::content);
                }), castle::status::ok);
        }), castle::status::ok);

    castle::net::udp_client<128U> peer;
    ASSERT_EQ(peer.open(), castle::status::ok);
    uint8_t wire[128U]{};
    size_type size = 0U;
    size_type sent = 0U;
    test_server::message_type_t request;
    request.type = message_type::non_confirmable;
    request.code_value = static_cast<uint8_t>(code::get);
    ASSERT_EQ(append_uri_path(request, "sensors/temp"), castle::status::ok);
    uint32_t now = 0U;
    auto send_and_process = [&](uint16_t id)
    {
        request.message_id = id;
        ASSERT_EQ(encode_message(request, wire, sizeof(wire), size), castle::status::ok);
        ASSERT_EQ(peer.send_to(wire, size, castle::net::endpoint::loopback_ipv4(56832U), sent), castle::status::ok);
        ASSERT_EQ(server.process_once(now += 10U, 1000), castle::status::ok);
    };

    send_and_process(1U);
    EXPECT_EQ(first_calls, 1U);
    EXPECT_EQ(second_calls, 0U);

    send_and_process(2U);
    EXPECT_EQ(first_calls, 1U);
    EXPECT_EQ(second_calls, 1U);
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(CoapServer, TransportStateAndSeparateResponsePreconditions)
{
    basic_server<128U, 2U, 2U, 4U, 1U, 4U, 16U, 32U, 32U, 64U> server;
    EXPECT_FALSE(server.is_open());
    EXPECT_EQ(server.client_count(), 0U);
    EXPECT_EQ(server.configure_rate_limit(0U, 1000U), castle::status::invalid_argument);
    EXPECT_EQ(server.configure_rate_limit(2U, 0U), castle::status::invalid_argument);
    EXPECT_EQ(server.configure_rate_limit(2U, 1000U), castle::status::ok);
    server.clear_rate_limit();
    EXPECT_EQ(server.process_once(0U, 0), castle::status::not_configured);
    using message_type_t = decltype(server)::message_type_t;
    message_type_t request;
    request.type = message_type::confirmable;
    request.message_id = 3U;
    message_type_t response;
    response.code_value = static_cast<uint8_t>(code::content);
    EXPECT_EQ(server.send_separate_response(castle::net::endpoint::loopback_ipv4(default_port), request, response, 0U),
              castle::status::not_configured);
}

TEST(CoapServer, OpenAndRegistrationValidation)
{
    server_t server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(56840U)), castle::status::ok);
    EXPECT_EQ(server.open(endpoint::loopback_ipv4(56840U)), castle::status::invalid_config);
    auto noop = [](message_t const&, message_t&) {};
    EXPECT_EQ(server.register_resource("x", 0x10U, noop), castle::status::invalid_argument);
    EXPECT_EQ(server.register_resource("", allow_get, noop), castle::status::invalid_argument);
    EXPECT_EQ(server.register_resource("/", allow_get, noop), castle::status::invalid_argument);
    EXPECT_EQ(server.register_resource("0123456789012345678901234567890123456789", allow_get, noop),
              castle::status::out_of_range);
    EXPECT_EQ(server.register_resource("nullcb", allow_get,
                                       static_cast<void (*)(message_t const&, message_t&)>(nullptr)),
              castle::status::invalid_callback);
    EXPECT_EQ(server.register_resource("/a/b", allow_get, noop), castle::status::ok);
    EXPECT_EQ(server.resource_count(), 1U);
    EXPECT_EQ(server.unregister_resource(nullptr), castle::status::invalid_argument);
    EXPECT_EQ(server.set_resource_max_age(nullptr, 5U), castle::status::invalid_argument);
    EXPECT_EQ(server.unregister_resource("a/b"), castle::status::ok);
    EXPECT_EQ(server.resource_count(), 0U);
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(CoapServer, ResourceLookupComparesEverySlot)
{
    server_t server;
    auto noop = [](message_t const&, message_t&) {};
    ASSERT_EQ(server.register_resource("aaa", allow_get, noop), castle::status::ok);
    EXPECT_EQ(server.set_resource_max_age("aaa", 5U), castle::status::ok);
    EXPECT_EQ(server.set_resource_max_age("bbb", 5U), castle::status::not_found);
    ASSERT_EQ(server.register_resource("bbb", allow_get, noop), castle::status::ok);
    EXPECT_EQ(server.set_resource_max_age("bbb", 5U), castle::status::ok);
    EXPECT_EQ(server.set_resource_max_age("aa", 5U), castle::status::not_found);
    EXPECT_EQ(server.set_resource_max_age("ccc", 5U), castle::status::not_found);
    EXPECT_EQ(server.unregister_resource("aaa"), castle::status::ok);
    EXPECT_EQ(server.resource_count(), 1U);
}

TEST(CoapServer, ReturnsErrorCodesAndCallbackOverrides)
{
    server_t server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(56841U)), castle::status::ok);
    peer_t peer;
    ASSERT_EQ(peer.bind(endpoint::loopback_ipv4(56842U)), castle::status::ok);
    endpoint const server_ep = endpoint::loopback_ipv4(56841U);

    ASSERT_EQ(server.register_resource("led/state", allow_put, [](message_t const&, message_t& response) {
        response.code_value = static_cast<uint8_t>(code::changed);
    }), castle::status::ok);
    ASSERT_EQ(server.register_resource("empty/code", allow_get, [](message_t const&, message_t& response) {
        response.code_value = static_cast<uint8_t>(code::empty);
    }), castle::status::ok);
    ASSERT_EQ(server.register_resource("options/full", allow_get, [](message_t const&, message_t& response) {
        uint8_t const byte = 1U;
        for (uint16_t i = 0U; i < 4U; ++i)
        {
            (void)response.append_option(100U, &byte, 1U);
        }
    }), castle::status::ok);
    ASSERT_EQ(server.register_resource("max/own", allow_get, [](message_t const&, message_t& response) {
        (void)response.append_uint_option(static_cast<uint16_t>(option_number::max_age), 30U);
    }), castle::status::ok);
    ASSERT_EQ(server.register_resource("bad/option", allow_get, [](message_t const&, message_t& response) {
        uint8_t const bytes[3] = {1U, 2U, 3U};
        (void)response.append_option(static_cast<uint16_t>(option_number::content_format), bytes, sizeof(bytes));
    }), castle::status::ok);

    message_t response;
    endpoint source;
    peer_send(peer, make_request(message_type::confirmable, code::get, 1U, "missing", 0x01U), server_ep);
    server_process(server, 10U);
    ASSERT_TRUE(peer_receive(peer, response, source));
    EXPECT_EQ(response.type, message_type::acknowledgement);
    EXPECT_EQ(response.message_id, 1U);
    EXPECT_EQ(response.code_value, static_cast<uint8_t>(code::not_found));

    peer_send(peer, make_request(message_type::confirmable, code::get, 2U, "led/state", 0x01U), server_ep);
    server_process(server, 20U);
    ASSERT_TRUE(peer_receive(peer, response, source));
    EXPECT_EQ(response.code_value, static_cast<uint8_t>(code::method_not_allowed));

    peer_send(peer, make_request(message_type::confirmable, code::put, 3U, "led/state", 0x01U), server_ep);
    server_process(server, 30U);
    ASSERT_TRUE(peer_receive(peer, response, source));
    EXPECT_EQ(response.code_value, static_cast<uint8_t>(code::changed));

    peer_send(peer, make_request(message_type::confirmable, code::get, 4U, "empty/code", 0x01U), server_ep);
    server_process(server, 40U);
    ASSERT_TRUE(peer_receive(peer, response, source));
    EXPECT_EQ(response.code_value, static_cast<uint8_t>(code::internal_server_error));

    peer_send(peer, make_request(message_type::confirmable, code::get, 5U, "options/full", 0x01U), server_ep);
    server_process(server, 50U);
    ASSERT_TRUE(peer_receive(peer, response, source));
    EXPECT_EQ(response.code_value, static_cast<uint8_t>(code::internal_server_error));

    peer_send(peer, make_request(message_type::confirmable, code::get, 6U, "max/own", 0x01U), server_ep);
    server_process(server, 60U);
    ASSERT_TRUE(peer_receive(peer, response, source));
    EXPECT_EQ(response.code_value, static_cast<uint8_t>(code::content));
    auto const* max_age = response.find_option(static_cast<uint16_t>(option_number::max_age));
    ASSERT_NE(max_age, nullptr);
    ASSERT_EQ(max_age->length, 1U);
    EXPECT_EQ(max_age->value[0U], 30U);

    // An unencodable callback response is dropped without a reply.
    peer_send(peer, make_request(message_type::confirmable, code::get, 7U, "bad/option", 0x01U), server_ep);
    server_process(server, 70U);
    EXPECT_FALSE(peer_has_datagram(peer));
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(CoapServer, ReplaysCachedRepliesAndExpiresDuplicateCache)
{
    server_t server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(56843U)), castle::status::ok);
    peer_t peer;
    ASSERT_EQ(peer.bind(endpoint::loopback_ipv4(56844U)), castle::status::ok);
    endpoint const server_ep = endpoint::loopback_ipv4(56843U);
    size_type calls = 0U;
    ASSERT_EQ(server.register_resource("dup", allow_get, [&calls](message_t const&, message_t&) { ++calls; }),
              castle::status::ok);

    message_t seen;
    endpoint source;
    peer_send(peer, make_request(message_type::confirmable, code::get, 5U, "dup", 0x01U), server_ep);
    server_process(server, 100U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.type, message_type::acknowledgement);
    EXPECT_EQ(calls, 1U);

    peer_send(peer, make_request(message_type::confirmable, code::get, 5U, "dup", 0x01U), server_ep);
    server_process(server, 110U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.message_id, 5U);
    EXPECT_EQ(calls, 1U);

    peer_send(peer, make_request(message_type::non_confirmable, code::get, 6U, "dup", 0x01U), server_ep);
    server_process(server, 120U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.type, message_type::non_confirmable);
    EXPECT_EQ(calls, 2U);

    peer_send(peer, make_request(message_type::non_confirmable, code::get, 6U, "dup", 0x01U), server_ep);
    server_process(server, 130U);
    EXPECT_FALSE(peer_has_datagram(peer));
    EXPECT_EQ(calls, 2U);

    // Entries older than the exchange lifetime are forgotten and the request runs again.
    peer_send(peer, make_request(message_type::confirmable, code::get, 5U, "dup", 0x01U), server_ep);
    server_process(server, 300000U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.type, message_type::acknowledgement);
    EXPECT_EQ(calls, 3U);
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(CoapServer, MalformedAndNonRequestDatagrams)
{
    server_t server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(56845U)), castle::status::ok);
    peer_t peer;
    ASSERT_EQ(peer.bind(endpoint::loopback_ipv4(56846U)), castle::status::ok);
    endpoint const server_ep = endpoint::loopback_ipv4(56845U);
    ASSERT_EQ(server.register_resource("ping", allow_get, [](message_t const&, message_t&) {}), castle::status::ok);

    message_t seen;
    endpoint source;
    const uint8_t three[] = {0x40U, 0x01U, 0x00U};
    peer_send_raw(peer, three, sizeof(three), server_ep);
    server_process(server, 10U);
    EXPECT_FALSE(peer_has_datagram(peer));

    const uint8_t empty_confirmable[] = {0x40U, 0x00U, 0x00U, 0x07U};
    peer_send_raw(peer, empty_confirmable, sizeof(empty_confirmable), server_ep);
    server_process(server, 20U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.type, message_type::reset);
    EXPECT_EQ(seen.message_id, 7U);

    const uint8_t empty_non_confirmable[] = {0x50U, 0x00U, 0x00U, 0x08U};
    peer_send_raw(peer, empty_non_confirmable, sizeof(empty_non_confirmable), server_ep);
    server_process(server, 30U);
    EXPECT_FALSE(peer_has_datagram(peer));

    const uint8_t critical_unknown[] = {0x40U, 0x01U, 0x00U, 0x09U, 0xD0U, 0x56U};
    peer_send_raw(peer, critical_unknown, sizeof(critical_unknown), server_ep);
    server_process(server, 40U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.type, message_type::acknowledgement);
    EXPECT_EQ(seen.code_value, static_cast<uint8_t>(code::bad_option));
    EXPECT_EQ(seen.message_id, 9U);

    message_t response_code = make_control(message_type::confirmable, 10U);
    response_code.code_value = static_cast<uint8_t>(code::content);
    peer_send(peer, response_code, server_ep);
    server_process(server, 50U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.type, message_type::reset);
    EXPECT_EQ(seen.message_id, 10U);

    response_code.type = message_type::non_confirmable;
    response_code.message_id = 11U;
    peer_send(peer, response_code, server_ep);
    server_process(server, 60U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.type, message_type::reset);
    EXPECT_EQ(seen.message_id, 11U);

    const uint8_t elective[] = {0x40U, 0x01U, 0x00U, 0x0CU, 0xD0U, 0x57U};
    peer_send_raw(peer, elective, sizeof(elective), server_ep);
    server_process(server, 70U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.type, message_type::acknowledgement);
    EXPECT_EQ(seen.code_value, static_cast<uint8_t>(code::not_found));
    EXPECT_EQ(seen.message_id, 12U);

    // Stray ACK and RST without a pending exchange are ignored.
    peer_send(peer, make_control(message_type::acknowledgement, 99U), server_ep);
    server_process(server, 80U);
    peer_send(peer, make_control(message_type::reset, 99U), server_ep);
    server_process(server, 90U);
    EXPECT_FALSE(peer_has_datagram(peer));
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(CoapServer, RateLimitingAnswersServiceUnavailable)
{
    server_t server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(56847U)), castle::status::ok);
    peer_t peer;
    ASSERT_EQ(peer.bind(endpoint::loopback_ipv4(56848U)), castle::status::ok);
    endpoint const server_ep = endpoint::loopback_ipv4(56847U);
    ASSERT_EQ(server.register_resource("rate", allow_get, [](message_t const&, message_t&) {}), castle::status::ok);

    EXPECT_EQ(server.configure_rate_limit(1U, 0x80000000U), castle::status::invalid_argument);
    ASSERT_EQ(server.configure_rate_limit(1U, 1000U), castle::status::ok);

    message_t seen;
    endpoint source;
    peer_send(peer, make_request(message_type::confirmable, code::get, 1U, "rate", 0x01U), server_ep);
    server_process(server, 0U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.code_value, static_cast<uint8_t>(code::content));

    peer_send(peer, make_request(message_type::confirmable, code::get, 2U, "rate", 0x01U), server_ep);
    server_process(server, 10U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.type, message_type::acknowledgement);
    EXPECT_EQ(seen.code_value, static_cast<uint8_t>(code::service_unavailable));
    auto const* retry = seen.find_option(static_cast<uint16_t>(option_number::max_age));
    ASSERT_NE(retry, nullptr);
    ASSERT_EQ(retry->length, 1U);
    EXPECT_EQ(retry->value[0U], 1U);

    peer_send(peer, make_request(message_type::non_confirmable, code::get, 3U, "rate", 0x01U), server_ep);
    server_process(server, 20U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.type, message_type::non_confirmable);
    EXPECT_EQ(seen.code_value, static_cast<uint8_t>(code::service_unavailable));

    // A new window admits requests again.
    peer_send(peer, make_request(message_type::confirmable, code::get, 4U, "rate", 0x01U), server_ep);
    server_process(server, 1000U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.code_value, static_cast<uint8_t>(code::content));

    server.clear_rate_limit();
    peer_send(peer, make_request(message_type::confirmable, code::get, 5U, "rate", 0x01U), server_ep);
    server_process(server, 1001U);
    ASSERT_TRUE(peer_receive(peer, seen, source));
    EXPECT_EQ(seen.code_value, static_cast<uint8_t>(code::content));
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(CoapServer, SeparateResponsesAcknowledgementsAndRetransmission)
{
    server_t server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(56849U)), castle::status::ok);
    peer_t peer;
    ASSERT_EQ(peer.bind(endpoint::loopback_ipv4(56850U)), castle::status::ok);
    endpoint const server_ep = endpoint::loopback_ipv4(56849U);
    endpoint const peer_ep = endpoint::loopback_ipv4(56850U);
    server.set_retransmission(100U, 1U);

    message_t response;
    response.code_value = static_cast<uint8_t>(code::content);
    uint8_t const payload[2] = {'o', 'k'};
    ASSERT_EQ(response.set_payload(payload, sizeof(payload)), castle::status::ok);

    endpoint from;
    message_t seen;
    message_t request = make_request(message_type::confirmable, code::get, 7U, "", 0x0AU);
    ASSERT_EQ(server.send_separate_response(peer_ep, request, response, 0U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.type, message_type::acknowledgement);
    EXPECT_EQ(seen.code_value, 0U);
    EXPECT_EQ(seen.message_id, 7U);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.type, message_type::confirmable);
    EXPECT_EQ(seen.code_value, static_cast<uint8_t>(code::content));
    EXPECT_EQ(seen.message_id, 1U);
    EXPECT_EQ(seen.token_length, 1U);
    EXPECT_EQ(seen.payload_size, 2U);

    // The client's ACK completes the pending response, so the timer stays silent.
    peer_send(peer, make_control(message_type::acknowledgement, 1U), server_ep);
    server_process(server, 10U);
    EXPECT_EQ(server.tick(1000U), castle::status::ok);
    EXPECT_FALSE(peer_has_datagram(peer));

    // A non-confirmable request gets a confirmable response without a preceding ACK; it retries once.
    message_t non_request = make_request(message_type::non_confirmable, code::get, 8U, "", 0x0BU);
    ASSERT_EQ(server.send_separate_response(peer_ep, non_request, response, 2000U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.type, message_type::confirmable);
    EXPECT_EQ(seen.message_id, 2U);
    EXPECT_EQ(server.tick(2050U), castle::status::ok);
    EXPECT_FALSE(peer_has_datagram(peer));
    EXPECT_EQ(server.tick(2100U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.message_id, 2U);
    EXPECT_EQ(server.tick(2300U), castle::status::ok);
    EXPECT_FALSE(peer_has_datagram(peer));

    // Unconfirmed separate responses are sent once and never tracked.
    message_t unconfirmed = make_request(message_type::non_confirmable, code::get, 9U, "", 0x0CU);
    ASSERT_EQ(server.send_separate_response(peer_ep, unconfirmed, response, 3000U, false), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.type, message_type::non_confirmable);
    EXPECT_EQ(seen.message_id, 3U);

    // A reset from the client cancels retransmission.
    message_t reset_target = make_request(message_type::non_confirmable, code::get, 10U, "", 0x0DU);
    ASSERT_EQ(server.send_separate_response(peer_ep, reset_target, response, 4000U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.message_id, 4U);
    peer_send(peer, make_control(message_type::reset, 4U), server_ep);
    server_process(server, 4010U);
    EXPECT_EQ(server.tick(5000U), castle::status::ok);
    EXPECT_FALSE(peer_has_datagram(peer));

    // A huge timeout saturates instead of doubling into overflow.
    server.set_retransmission(0x40000000U, 2U);
    message_t saturated = make_request(message_type::non_confirmable, code::get, 11U, "", 0x0EU);
    ASSERT_EQ(server.send_separate_response(peer_ep, saturated, response, 0U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.message_id, 5U);
    EXPECT_EQ(server.tick(0x40000000U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.message_id, 5U);
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(CoapServer, SeparateResponseRejectsInvalidArgumentsAndTransportErrors)
{
    server_t server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(56855U)), castle::status::ok);
    peer_t peer;
    ASSERT_EQ(peer.bind(endpoint::loopback_ipv4(56856U)), castle::status::ok);
    endpoint const peer_ep = endpoint::loopback_ipv4(56856U);
    endpoint const ipv6_ep = endpoint::loopback_ipv6(56856U);

    message_t response;
    response.code_value = static_cast<uint8_t>(code::content);
    message_t request = make_request(message_type::confirmable, code::get, 1U, "", 0x01U);

    message_t content_request = request;
    content_request.code_value = static_cast<uint8_t>(code::content);
    EXPECT_EQ(server.send_separate_response(peer_ep, content_request, response, 0U), castle::status::invalid_argument);
    message_t ack_request = request;
    ack_request.type = message_type::acknowledgement;
    EXPECT_EQ(server.send_separate_response(peer_ep, ack_request, response, 0U), castle::status::invalid_argument);
    message_t long_token = request;
    long_token.token_length = maximum_token_length + 1U;
    EXPECT_EQ(server.send_separate_response(peer_ep, long_token, response, 0U), castle::status::invalid_argument);
    message_t not_response = response;
    not_response.code_value = 0x01U;
    EXPECT_EQ(server.send_separate_response(peer_ep, request, not_response, 0U), castle::status::invalid_argument);

    // An option the codec cannot encode rejects the response before anything is sent.
    message_t bad_option = response;
    const uint8_t three[3] = {1U, 2U, 3U};
    (void)bad_option.append_option(static_cast<uint16_t>(option_number::content_format), three, sizeof(three));
    EXPECT_EQ(server.send_separate_response(peer_ep, request, bad_option, 0U), castle::status::invalid_argument);
    EXPECT_FALSE(peer_has_datagram(peer));

    // An IPv6 destination cannot be reached through the IPv4 listener.
    EXPECT_NE(server.send_separate_response(ipv6_ep, request, response, 0U), castle::status::ok);
    message_t non_request = make_request(message_type::non_confirmable, code::get, 2U, "", 0x02U);
    EXPECT_NE(server.send_separate_response(ipv6_ep, non_request, response, 0U), castle::status::ok);
    EXPECT_NE(server.send_separate_response(ipv6_ep, non_request, response, 0U, false), castle::status::ok);
    EXPECT_FALSE(peer_has_datagram(peer));

    EXPECT_EQ(server.send_empty_ack(peer_ep, 77U), castle::status::ok);
    endpoint from;
    message_t seen;
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.type, message_type::acknowledgement);
    EXPECT_EQ(seen.message_id, 77U);
    EXPECT_NE(server.send_empty_ack(ipv6_ep, 78U), castle::status::ok);
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(CoapServer, SeparateResponseSkipsInUseMessageIds)
{
    server_t server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(56851U)), castle::status::ok);
    peer_t peer;
    ASSERT_EQ(peer.bind(endpoint::loopback_ipv4(56852U)), castle::status::ok);
    endpoint const peer_ep = endpoint::loopback_ipv4(56852U);

    message_t response;
    response.code_value = static_cast<uint8_t>(code::content);
    message_t request = make_request(message_type::non_confirmable, code::get, 20U, "", 0x14U);

    endpoint from;
    message_t seen;
    ASSERT_EQ(server.send_separate_response(peer_ep, request, response, 0U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.message_id, 1U);

    // Advance the counter until it hands out the still-pending ID 1 again.
    for (uint32_t i = 0U; i < 65535U; ++i)
    {
        (void)server.allocate_message_id();
    }
    ASSERT_EQ(server.send_separate_response(peer_ep, request, response, 0U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.message_id, 2U);
    EXPECT_EQ(server.close(), castle::status::ok);
}

TEST(CoapServer, SeparateResponseReportsFullPendingTable)
{
    server_t server;
    ASSERT_EQ(server.open(endpoint::loopback_ipv4(56853U)), castle::status::ok);
    peer_t peer;
    ASSERT_EQ(peer.bind(endpoint::loopback_ipv4(56854U)), castle::status::ok);
    endpoint const server_ep = endpoint::loopback_ipv4(56853U);
    endpoint const peer_ep = endpoint::loopback_ipv4(56854U);

    message_t response;
    response.code_value = static_cast<uint8_t>(code::content);
    endpoint from;
    message_t seen;
    message_t first = make_request(message_type::non_confirmable, code::get, 30U, "", 0x1EU);
    message_t second = make_request(message_type::non_confirmable, code::get, 31U, "", 0x1FU);
    message_t third = make_request(message_type::non_confirmable, code::get, 32U, "", 0x20U);

    ASSERT_EQ(server.send_separate_response(peer_ep, first, response, 0U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    ASSERT_EQ(server.send_separate_response(peer_ep, second, response, 0U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(server.send_separate_response(peer_ep, third, response, 0U), castle::status::full);
    EXPECT_FALSE(peer_has_datagram(peer));

    // Acknowledging the first response frees its slot.
    peer_send(peer, make_control(message_type::acknowledgement, 1U), server_ep);
    server_process(server, 10U);
    ASSERT_EQ(server.send_separate_response(peer_ep, third, response, 20U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, from));
    EXPECT_EQ(seen.message_id, 3U);
    EXPECT_EQ(server.close(), castle::status::ok);
}


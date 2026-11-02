#include <gtest/gtest.h>
#include "castle_ext/protocols/coap/client.hpp"

using namespace castle::protocols::coap;
using castle::size_type;

namespace {

using test_client = basic_client<128U, 2U, 4U, 16U, 32U, 2U, 64U>;
using test_message = test_client::message_type_t;
using test_peer = castle::net::udp_client<128U>;
using castle::net::endpoint;

struct recorder
{
    size_type calls = 0U;
    castle::status last_status = castle::status::ok;
    bool has_message = false;
    test_message last_message;
};

auto make_recorder_callback(recorder& rec)
{
    return [&rec](test_client::exchange_handle, castle::status result, test_message const* value,
                  endpoint const&) {
        ++rec.calls;
        rec.last_status = result;
        rec.has_message = value != nullptr;
        if (value != nullptr)
        {
            rec.last_message = *value;
        }
    };
}

test_message make_request(message_type type, uint16_t id, uint8_t token)
{
    test_message request;
    request.type = type;
    request.code_value = static_cast<uint8_t>(code::get);
    request.message_id = id;
    uint8_t bytes[1] = {token};
    (void)request.set_token(bytes, 1U);
    return request;
}

test_message make_control(message_type type, uint16_t id)
{
    test_message value;
    value.type = type;
    value.code_value = static_cast<uint8_t>(code::empty);
    value.message_id = id;
    return value;
}

test_message make_response(message_type type, uint16_t id, uint8_t token, code response_code)
{
    test_message value;
    value.type = type;
    value.code_value = static_cast<uint8_t>(response_code);
    value.message_id = id;
    uint8_t bytes[1] = {token};
    (void)value.set_token(bytes, 1U);
    return value;
}

bool peer_receive(test_peer& peer, test_message& out, endpoint& source)
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

bool peer_has_datagram(test_peer& peer)
{
    return peer.socket().wait_readable(50) == castle::status::ok;
}

void peer_send(test_peer& peer, test_message const& value, endpoint const& to)
{
    uint8_t wire[128]{};
    size_type size = 0U;
    size_type sent = 0U;
    ASSERT_EQ(encode_message(value, wire, sizeof(wire), size), castle::status::ok);
    ASSERT_EQ(peer.send_to(wire, size, to, sent), castle::status::ok);
}

void peer_send_raw(test_peer& peer, uint8_t const* bytes, size_type size, endpoint const& to)
{
    size_type sent = 0U;
    ASSERT_EQ(peer.send_to(bytes, size, to, sent), castle::status::ok);
}

// Sends one request and consumes it at the fake peer, which records the client's source endpoint.
void send_and_drain(test_client& client, test_peer& peer, endpoint const& remote, test_message const& request,
                    test_client::exchange_handle& handle, uint32_t now, endpoint& client_endpoint)
{
    ASSERT_EQ(client.send_request(remote, request, handle, now), castle::status::ok);
    test_message seen;
    ASSERT_TRUE(peer_receive(peer, seen, client_endpoint));
}

} // namespace

TEST(CoapClient, ConfigurationAndEmptyState)
{
    basic_client<128U, 2U, 4U, 16U, 32U, 2U, 64U> client;
    EXPECT_FALSE(client.is_open());
    EXPECT_EQ(client.outstanding(), 0U);
    EXPECT_EQ(client.set_retransmission(0U, 1U), castle::status::invalid_argument);
    EXPECT_EQ(client.set_retransmission(250U, 9U), castle::status::invalid_argument);
    EXPECT_EQ(client.set_retransmission(500U, 2U), castle::status::ok);
    EXPECT_EQ(client.allocate_message_id(), 1U);
    client.set_next_message_id(65535U);
    EXPECT_EQ(client.allocate_message_id(), 65535U);
    EXPECT_EQ(client.allocate_message_id(), 0U);
}

TEST(CoapClient, CallbackCancellationAndClosedSend)
{
    using test_client = basic_client<128U, 2U, 4U, 16U, 32U, 2U, 64U>;
    test_client client;
    bool called = false;
    EXPECT_EQ(client.set_response_callback([&called](test_client::exchange_handle, castle::status,
        test_client::message_type_t const*, castle::net::endpoint const&) { called = true; }), castle::status::ok);
    EXPECT_EQ(client.set_response_callback(static_cast<void(*)(test_client::exchange_handle, castle::status,
        test_client::message_type_t const*, castle::net::endpoint const&)>(nullptr)), castle::status::invalid_callback);
    test_client::message_type_t request;
    request.code_value = static_cast<uint8_t>(code::get);
    test_client::exchange_handle handle;
    EXPECT_EQ(client.send_request(castle::net::endpoint::loopback_ipv4(default_port), request, handle, 0U),
              castle::status::not_configured);
    EXPECT_EQ(client.cancel(handle), castle::status::not_found);
    EXPECT_EQ(client.tick(5000U), castle::status::ok);
    EXPECT_FALSE(called);
}

TEST(CoapClient, ResponseCallbackCanReplaceItselfOnTimeout)
{
    using test_client = basic_client<128U, 2U, 4U, 16U, 32U, 2U, 64U>;
    test_client client;
    ASSERT_EQ(client.open(), castle::status::ok);
    size_type first_calls = 0U;
    size_type second_calls = 0U;
    ASSERT_EQ(client.set_response_callback([&](test_client::exchange_handle, castle::status result,
        test_client::message_type_t const*, castle::net::endpoint const&)
    {
        EXPECT_EQ(result, castle::status::time_out);
        ++first_calls;
        EXPECT_EQ(client.set_response_callback([&second_calls](test_client::exchange_handle, castle::status,
            test_client::message_type_t const*, castle::net::endpoint const&) { ++second_calls; }), castle::status::ok);
    }), castle::status::ok);

    test_client::message_type_t request;
    request.type = message_type::confirmable;
    request.code_value = static_cast<uint8_t>(code::get);
    uint32_t now = 0U;
    auto expire_next_request = [&](size_type& calls)
    {
        test_client::exchange_handle handle;
        ASSERT_EQ(client.send_request(castle::net::endpoint::loopback_ipv4(56831U), request, handle, now), castle::status::ok);
        for (uint32_t step = 0U; step < 10U && calls == 0U; ++step)
        {
            now += 100000U;
            ASSERT_EQ(client.tick(now), castle::status::ok);
        }
    };

    expire_next_request(first_calls);
    EXPECT_EQ(first_calls, 1U);
    EXPECT_EQ(second_calls, 0U);

    expire_next_request(second_calls);
    EXPECT_EQ(first_calls, 1U);
    EXPECT_EQ(second_calls, 1U);
    EXPECT_EQ(client.close(), castle::status::ok);
}

TEST(CoapClient, SendRequestRejectsInvalidRequests)
{
    test_client client;
    ASSERT_EQ(client.open(), castle::status::ok);
    endpoint const remote = endpoint::loopback_ipv4(56900U);
    test_client::exchange_handle handle;

    test_message request = make_request(message_type::confirmable, 1U, 0x01U);
    request.code_value = static_cast<uint8_t>(code::content);
    EXPECT_EQ(client.send_request(remote, request, handle, 0U), castle::status::invalid_argument);
    request.code_value = static_cast<uint8_t>(code::get);
    request.type = message_type::acknowledgement;
    EXPECT_EQ(client.send_request(remote, request, handle, 0U), castle::status::invalid_argument);
    request.type = message_type::confirmable;
    request.token_length = maximum_token_length + 1U;
    EXPECT_EQ(client.send_request(remote, request, handle, 0U), castle::status::invalid_argument);
    EXPECT_FALSE(handle.is_valid());
    EXPECT_EQ(client.outstanding(), 0U);
    EXPECT_EQ(client.close(), castle::status::ok);
}

TEST(CoapClient, SendRequestTracksDuplicatesCapacityAndCancellation)
{
    test_client client;
    ASSERT_EQ(client.open(), castle::status::ok);
    endpoint const remote = endpoint::loopback_ipv4(56901U);
    test_client::exchange_handle first;
    test_client::exchange_handle second;
    test_client::exchange_handle third;
    test_client::exchange_handle fourth;

    ASSERT_EQ(client.send_request(remote, make_request(message_type::confirmable, 1U, 0x11U), first, 0U),
              castle::status::ok);
    EXPECT_EQ(client.send_request(remote, make_request(message_type::confirmable, 1U, 0x22U), second, 0U),
              castle::status::already_exists);
    EXPECT_EQ(client.send_request(remote, make_request(message_type::confirmable, 2U, 0x11U), second, 0U),
              castle::status::already_exists);
    EXPECT_EQ(client.send_request(remote, make_request(message_type::confirmable, 2U, 0x12U), second, 0U),
              castle::status::ok);
    EXPECT_EQ(client.outstanding(), 2U);

    test_message longer = make_request(message_type::confirmable, 3U, 0x11U);
    longer.token_length = 2U;
    longer.token[1U] = 0x33U;
    EXPECT_EQ(client.send_request(remote, longer, third, 0U), castle::status::full);

    EXPECT_EQ(client.cancel(first), castle::status::ok);
    EXPECT_EQ(client.cancel(first), castle::status::not_found);
    EXPECT_EQ(client.cancel(test_client::exchange_handle{}), castle::status::not_found);
    EXPECT_EQ(client.cancel(test_client::exchange_handle{0U, 0U}), castle::status::not_found);
    EXPECT_EQ(client.cancel(test_client::exchange_handle{5U, 1U}), castle::status::not_found);
    EXPECT_EQ(client.outstanding(), 1U);

    ASSERT_EQ(client.send_request(remote, make_request(message_type::confirmable, 3U, 0x11U), fourth, 0U),
              castle::status::ok);
    EXPECT_NE(fourth.generation, first.generation);
    EXPECT_EQ(client.cancel(first), castle::status::not_found);
    EXPECT_EQ(client.close(), castle::status::ok);
}

TEST(CoapClient, SendRequestReportsEncodeAndTransportFailures)
{
    test_client client;
    ASSERT_EQ(client.open(), castle::status::ok);
    endpoint const remote = endpoint::loopback_ipv4(56903U);
    test_client::exchange_handle handle;

    test_message malformed = make_request(message_type::confirmable, 1U, 0x01U);
    const uint8_t three[3] = {1U, 2U, 3U};
    (void)malformed.append_option(static_cast<uint16_t>(option_number::content_format), three, sizeof(three));
    EXPECT_EQ(client.send_request(remote, malformed, handle, 0U), castle::status::invalid_argument);
    EXPECT_EQ(client.outstanding(), 0U);

    EXPECT_NE(client.send_request(endpoint::loopback_ipv6(56903U), make_request(message_type::confirmable, 2U, 0x02U),
                                  handle, 0U),
              castle::status::ok);
    EXPECT_EQ(client.outstanding(), 0U);
    EXPECT_EQ(client.close(), castle::status::ok);
}

TEST(CoapClient, RegistrationOptionsAndTokenCopy)
{
    test_client client;
    ASSERT_EQ(client.open(), castle::status::ok);
    test_peer peer;
    ASSERT_EQ(peer.bind(endpoint::loopback_ipv4(56904U)), castle::status::ok);
    endpoint const remote = endpoint::loopback_ipv4(56904U);

    test_message observe = make_request(message_type::confirmable, 7U, 0xAAU);
    observe.token_length = 2U;
    observe.token[1U] = 0xBBU;
    ASSERT_EQ(observe.append_uint_option(static_cast<uint16_t>(option_number::observe), 0U), castle::status::ok);
    test_client::exchange_handle observe_handle;
    endpoint client_endpoint;
    send_and_drain(client, peer, remote, observe, observe_handle, 0U, client_endpoint);

    test_message other = make_request(message_type::non_confirmable, 8U, 0xCCU);
    ASSERT_EQ(other.append_uint_option(static_cast<uint16_t>(option_number::observe), 1U), castle::status::ok);
    test_client::exchange_handle other_handle;
    send_and_drain(client, peer, remote, other, other_handle, 0U, client_endpoint);
    EXPECT_EQ(client.outstanding(), 2U);

    EXPECT_EQ(client.cancel(observe_handle), castle::status::ok);
    EXPECT_EQ(client.cancel(other_handle), castle::status::ok);
    EXPECT_EQ(client.close(), castle::status::ok);
}

TEST(CoapClient, ProcessOnceRoutesResponsesAcknowledgementsAndResets)
{
    test_client client;
    ASSERT_EQ(client.open(), castle::status::ok);
    test_peer peer;
    ASSERT_EQ(peer.bind(endpoint::loopback_ipv4(56905U)), castle::status::ok);
    endpoint const remote = endpoint::loopback_ipv4(56905U);
    recorder rec;
    ASSERT_EQ(client.set_response_callback(make_recorder_callback(rec)), castle::status::ok);

    endpoint client_endpoint;
    test_client::exchange_handle handle;
    test_message seen;

    // Acknowledged request completed by a non-confirmable response, then a duplicate of that response.
    send_and_drain(client, peer, remote, make_request(message_type::confirmable, 10U, 0x01U), handle, 0U, client_endpoint);
    peer_send(peer, make_control(message_type::acknowledgement, 10U), client_endpoint);
    EXPECT_EQ(client.process_once(100U, 1000), castle::status::ok);
    EXPECT_EQ(rec.calls, 0U);
    EXPECT_EQ(client.outstanding(), 1U);
    peer_send(peer, make_control(message_type::acknowledgement, 99U), client_endpoint);
    EXPECT_EQ(client.process_once(110U, 1000), castle::status::ok);
    peer_send(peer, make_response(message_type::non_confirmable, 11U, 0x01U, code::content), client_endpoint);
    EXPECT_EQ(client.process_once(120U, 1000), castle::status::ok);
    EXPECT_EQ(rec.calls, 1U);
    EXPECT_TRUE(rec.has_message);
    EXPECT_EQ(rec.last_message.code_value, static_cast<uint8_t>(code::content));
    EXPECT_EQ(client.outstanding(), 0U);
    peer_send(peer, make_response(message_type::non_confirmable, 11U, 0x01U, code::content), client_endpoint);
    EXPECT_EQ(client.process_once(130U, 1000), castle::status::ok);
    EXPECT_EQ(rec.calls, 1U);

    // Unmatched confirmable response is reset; its duplicate is acknowledged instead.
    peer_send(peer, make_response(message_type::confirmable, 12U, 0x02U, code::content), client_endpoint);
    EXPECT_EQ(client.process_once(140U, 1000), castle::status::not_found);
    ASSERT_TRUE(peer_receive(peer, seen, client_endpoint));
    EXPECT_EQ(seen.type, message_type::reset);
    EXPECT_EQ(seen.message_id, 12U);
    peer_send(peer, make_response(message_type::confirmable, 12U, 0x02U, code::content), client_endpoint);
    EXPECT_EQ(client.process_once(150U, 1000), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, client_endpoint));
    EXPECT_EQ(seen.type, message_type::acknowledgement);
    EXPECT_EQ(seen.message_id, 12U);

    // Unmatched non-confirmable response is ignored silently.
    peer_send(peer, make_response(message_type::non_confirmable, 13U, 0x09U, code::content), client_endpoint);
    EXPECT_EQ(client.process_once(160U, 1000), castle::status::not_found);
    EXPECT_FALSE(peer_has_datagram(peer));

    // A piggybacked ACK must carry the exchange's message ID to complete it.
    send_and_drain(client, peer, remote, make_request(message_type::confirmable, 30U, 0x03U), handle, 170U, client_endpoint);
    peer_send(peer, make_response(message_type::acknowledgement, 31U, 0x03U, code::content), client_endpoint);
    EXPECT_EQ(client.process_once(170U, 1000), castle::status::not_found);
    peer_send(peer, make_response(message_type::acknowledgement, 30U, 0x03U, code::content), client_endpoint);
    EXPECT_EQ(client.process_once(180U, 1000), castle::status::ok);
    EXPECT_EQ(rec.calls, 2U);
    EXPECT_EQ(client.outstanding(), 0U);

    // A reset for a pending exchange reports invalid_config; unknown IDs are ignored.
    send_and_drain(client, peer, remote, make_request(message_type::confirmable, 20U, 0x04U), handle, 185U, client_endpoint);
    peer_send(peer, make_control(message_type::reset, 20U), client_endpoint);
    EXPECT_EQ(client.process_once(190U, 1000), castle::status::ok);
    EXPECT_EQ(rec.calls, 3U);
    EXPECT_EQ(rec.last_status, castle::status::invalid_config);
    EXPECT_FALSE(rec.has_message);
    peer_send(peer, make_control(message_type::reset, 98U), client_endpoint);
    EXPECT_EQ(client.process_once(200U, 1000), castle::status::ok);
    EXPECT_EQ(rec.calls, 3U);

    // Observe registration stays active after a confirmable notification, which is acknowledged.
    test_message observe = make_request(message_type::confirmable, 40U, 0x05U);
    ASSERT_EQ(observe.append_uint_option(static_cast<uint16_t>(option_number::observe), 0U), castle::status::ok);
    test_client::exchange_handle observe_handle;
    send_and_drain(client, peer, remote, observe, observe_handle, 205U, client_endpoint);
    test_message notification = make_response(message_type::confirmable, 41U, 0x05U, code::content);
    ASSERT_EQ(notification.append_uint_option(static_cast<uint16_t>(option_number::observe), 5U), castle::status::ok);
    peer_send(peer, notification, client_endpoint);
    EXPECT_EQ(client.process_once(210U, 1000), castle::status::ok);
    EXPECT_EQ(rec.calls, 4U);
    EXPECT_EQ(client.outstanding(), 1U);
    ASSERT_TRUE(peer_receive(peer, seen, client_endpoint));
    EXPECT_EQ(seen.type, message_type::acknowledgement);
    EXPECT_EQ(seen.message_id, 41U);
    EXPECT_EQ(client.cancel(observe_handle), castle::status::ok);

    // A confirmable response to a plain request is acknowledged and completes the exchange.
    send_and_drain(client, peer, remote, make_request(message_type::confirmable, 50U, 0x06U), handle, 215U, client_endpoint);
    peer_send(peer, make_response(message_type::confirmable, 51U, 0x06U, code::content), client_endpoint);
    EXPECT_EQ(client.process_once(220U, 1000), castle::status::ok);
    EXPECT_EQ(rec.calls, 5U);
    EXPECT_EQ(client.outstanding(), 0U);
    ASSERT_TRUE(peer_receive(peer, seen, client_endpoint));
    EXPECT_EQ(seen.type, message_type::acknowledgement);
    EXPECT_EQ(seen.message_id, 51U);

    // Without a callback the exchange still completes.
    client.clear_response_callback();
    send_and_drain(client, peer, remote, make_request(message_type::confirmable, 52U, 0x07U), handle, 225U, client_endpoint);
    peer_send(peer, make_response(message_type::non_confirmable, 53U, 0x07U, code::content), client_endpoint);
    EXPECT_EQ(client.process_once(230U, 1000), castle::status::ok);
    EXPECT_EQ(rec.calls, 5U);
    EXPECT_EQ(client.outstanding(), 0U);

    // Malformed and oversized datagrams are rejected; a malformed confirmable gets a reset.
    const uint8_t three[] = {0x40U, 0x01U, 0x00U};
    peer_send_raw(peer, three, sizeof(three), client_endpoint);
    EXPECT_EQ(client.process_once(240U, 1000), castle::status::invalid_argument);
    const uint8_t empty_confirmable[] = {0x40U, 0x00U, 0x00U, 0x07U};
    peer_send_raw(peer, empty_confirmable, sizeof(empty_confirmable), client_endpoint);
    EXPECT_EQ(client.process_once(250U, 1000), castle::status::invalid_argument);
    ASSERT_TRUE(peer_receive(peer, seen, client_endpoint));
    EXPECT_EQ(seen.type, message_type::reset);
    EXPECT_EQ(seen.message_id, 7U);
    const uint8_t empty_non_confirmable[] = {0x50U, 0x00U, 0x00U, 0x08U};
    peer_send_raw(peer, empty_non_confirmable, sizeof(empty_non_confirmable), client_endpoint);
    EXPECT_EQ(client.process_once(260U, 1000), castle::status::invalid_argument);
    EXPECT_FALSE(peer_has_datagram(peer));
    uint8_t oversized[200]{};
    peer_send_raw(peer, oversized, sizeof(oversized), client_endpoint);
    EXPECT_EQ(client.process_once(270U, 1000), castle::status::data_loss);

    // Nothing pending: a zero-timeout poll reports empty.
    EXPECT_EQ(client.process_once(280U, 0), castle::status::empty);
    EXPECT_EQ(client.close(), castle::status::ok);

    test_client closed;
    EXPECT_NE(closed.process_once(0U, 0), castle::status::ok);
}

TEST(CoapClient, TickRetransmitsAndReportsTimeouts)
{
    test_client client;
    ASSERT_EQ(client.open(), castle::status::ok);
    test_peer peer;
    ASSERT_EQ(peer.bind(endpoint::loopback_ipv4(56906U)), castle::status::ok);
    endpoint const remote = endpoint::loopback_ipv4(56906U);
    recorder rec;
    ASSERT_EQ(client.set_response_callback(make_recorder_callback(rec)), castle::status::ok);
    ASSERT_EQ(client.set_retransmission(100U, 2U), castle::status::ok);

    endpoint client_endpoint;
    test_message seen;
    test_client::exchange_handle confirmable;
    test_client::exchange_handle non_confirmable;
    send_and_drain(client, peer, remote, make_request(message_type::confirmable, 60U, 0x06U), confirmable, 0U, client_endpoint);
    send_and_drain(client, peer, remote, make_request(message_type::non_confirmable, 61U, 0x07U), non_confirmable, 0U, client_endpoint);

    EXPECT_EQ(client.tick(50U), castle::status::ok);
    EXPECT_FALSE(peer_has_datagram(peer));
    EXPECT_EQ(rec.calls, 0U);

    EXPECT_EQ(client.tick(100U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, client_endpoint));
    EXPECT_EQ(seen.message_id, 60U);
    EXPECT_EQ(rec.calls, 1U);
    EXPECT_EQ(rec.last_status, castle::status::time_out);
    EXPECT_EQ(client.outstanding(), 1U);

    EXPECT_EQ(client.tick(300U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, client_endpoint));
    EXPECT_EQ(seen.message_id, 60U);

    EXPECT_EQ(client.tick(700U), castle::status::ok);
    EXPECT_EQ(rec.calls, 2U);
    EXPECT_EQ(client.outstanding(), 0U);
    EXPECT_FALSE(peer_has_datagram(peer));

    // Zero retransmissions: the first due tick reports time_out immediately.
    ASSERT_EQ(client.set_retransmission(100U, 0U), castle::status::ok);
    test_client::exchange_handle no_retry;
    send_and_drain(client, peer, remote, make_request(message_type::confirmable, 62U, 0x08U), no_retry, 2000U, client_endpoint);
    EXPECT_EQ(client.tick(2100U), castle::status::ok);
    EXPECT_EQ(rec.calls, 3U);
    EXPECT_FALSE(peer_has_datagram(peer));

    // Without a callback a timeout still releases the exchange.
    client.clear_response_callback();
    test_client::exchange_handle silent;
    send_and_drain(client, peer, remote, make_request(message_type::confirmable, 63U, 0x09U), silent, 3000U, client_endpoint);
    EXPECT_EQ(client.tick(3100U), castle::status::ok);
    EXPECT_EQ(rec.calls, 3U);
    EXPECT_EQ(client.outstanding(), 0U);
    ASSERT_EQ(client.set_response_callback(make_recorder_callback(rec)), castle::status::ok);

    // An acknowledged exchange stops retransmitting and times out only at its lifetime deadline.
    ASSERT_EQ(client.set_retransmission(100U, 2U), castle::status::ok);
    test_client::exchange_handle acknowledged;
    send_and_drain(client, peer, remote, make_request(message_type::confirmable, 64U, 0x0AU), acknowledged, 4000U, client_endpoint);
    peer_send(peer, make_control(message_type::acknowledgement, 64U), client_endpoint);
    EXPECT_EQ(client.process_once(4000U, 1000), castle::status::ok);
    EXPECT_EQ(client.tick(250999U), castle::status::ok);
    EXPECT_EQ(rec.calls, 3U);
    EXPECT_EQ(client.tick(251000U), castle::status::ok);
    EXPECT_EQ(rec.calls, 4U);
    EXPECT_FALSE(peer_has_datagram(peer));

    // A huge timeout saturates instead of doubling into overflow.
    ASSERT_EQ(client.set_retransmission(0x40000000U, 2U), castle::status::ok);
    test_client::exchange_handle saturated;
    send_and_drain(client, peer, remote, make_request(message_type::confirmable, 65U, 0x0BU), saturated, 0U, client_endpoint);
    EXPECT_EQ(client.tick(0x40000000U), castle::status::ok);
    ASSERT_TRUE(peer_receive(peer, seen, client_endpoint));
    EXPECT_EQ(seen.message_id, 65U);
    EXPECT_EQ(client.close(), castle::status::ok);
}

TEST(CoapClient, MatchesExchangesByRemoteEndpoint)
{
    test_client client;
    ASSERT_EQ(client.open(), castle::status::ok);
    test_peer first;
    ASSERT_EQ(first.bind(endpoint::loopback_ipv4(56907U)), castle::status::ok);
    test_peer second;
    ASSERT_EQ(second.bind(endpoint::loopback_ipv4(56908U)), castle::status::ok);
    endpoint const first_remote = endpoint::loopback_ipv4(56907U);
    endpoint const second_remote = endpoint::loopback_ipv4(56908U);
    recorder rec;
    ASSERT_EQ(client.set_response_callback(make_recorder_callback(rec)), castle::status::ok);

    endpoint client_endpoint;
    endpoint ignored;
    test_message seen;
    test_client::exchange_handle first_handle;
    test_client::exchange_handle second_handle;
    send_and_drain(client, first, first_remote, make_request(message_type::confirmable, 70U, 0x7AU), first_handle, 0U, client_endpoint);
    send_and_drain(client, second, second_remote, make_request(message_type::confirmable, 70U, 0x7AU), second_handle, 0U, ignored);
    EXPECT_EQ(client.send_request(first_remote, make_request(message_type::confirmable, 70U, 0x7AU), second_handle, 0U),
              castle::status::already_exists);
    EXPECT_EQ(client.outstanding(), 2U);

    // Same token from the second peer completes only the exchange addressed to it.
    peer_send(second, make_response(message_type::non_confirmable, 80U, 0x7AU, code::content), client_endpoint);
    EXPECT_EQ(client.process_once(10U, 1000), castle::status::ok);
    EXPECT_EQ(rec.calls, 1U);
    EXPECT_EQ(client.outstanding(), 1U);

    // A token that differs in one byte, or in length, does not match the first peer's exchange.
    peer_send(first, make_response(message_type::non_confirmable, 81U, 0x7BU, code::content), client_endpoint);
    EXPECT_EQ(client.process_once(20U, 1000), castle::status::not_found);
    test_message longer_token = make_response(message_type::non_confirmable, 82U, 0x7AU, code::content);
    longer_token.token_length = 2U;
    longer_token.token[1U] = 0U;
    peer_send(first, longer_token, client_endpoint);
    EXPECT_EQ(client.process_once(30U, 1000), castle::status::not_found);

    // An acknowledgement from the wrong peer does not match; the right peer completes it.
    peer_send(second, make_control(message_type::acknowledgement, 70U), client_endpoint);
    EXPECT_EQ(client.process_once(40U, 1000), castle::status::ok);
    peer_send(first, make_control(message_type::acknowledgement, 70U), client_endpoint);
    EXPECT_EQ(client.process_once(50U, 1000), castle::status::ok);
    EXPECT_EQ(client.outstanding(), 1U);
    EXPECT_EQ(client.cancel(first_handle), castle::status::ok);
    EXPECT_EQ(client.close(), castle::status::ok);
}


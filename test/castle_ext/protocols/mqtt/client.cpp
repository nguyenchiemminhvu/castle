#include <gtest/gtest.h>
#include "castle_ext/protocols/mqtt/client.hpp"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace castle::protocols::mqtt;
using castle::size_type;

namespace
{

using client_type = basic_client<128U, 256U, 4U, 48U, 64U>;
using tiny_client_type = basic_client<64U, 24U, 2U, 48U, 32U>;
using options_type = connect_options<>;

constexpr int peer_wait_ms = 1000;

const uint8_t kConnackOk[] = {0x20U, 0x02U, 0x00U, 0x00U};
const uint8_t kConnackV5Ok[] = {0x20U, 0x03U, 0x00U, 0x00U, 0x00U};
const uint8_t kSubackOne[] = {0x90U, 0x03U, 0x00U, 0x01U, 0x01U};
const uint8_t kPubackTwo[] = {0x40U, 0x02U, 0x00U, 0x02U};
const uint8_t kPubrecThree[] = {0x50U, 0x02U, 0x00U, 0x03U};
const uint8_t kPubcompThree[] = {0x70U, 0x02U, 0x00U, 0x03U};
const uint8_t kIncomingQos0[] = {0x30U, 0x07U, 0x00U, 0x03U, 'a', '/', 'b', 'h', 'i'};
const uint8_t kIncomingQos1[] = {0x32U, 0x08U, 0x00U, 0x03U, 'a', '/', 'b', 0x00U, 0x07U, 'x'};
const uint8_t kIncomingQos2[] = {0x34U, 0x08U, 0x00U, 0x03U, 'a', '/', 'b', 0x00U, 0x08U, 'y'};
const uint8_t kPubrelEight[] = {0x62U, 0x02U, 0x00U, 0x08U};
const uint8_t kPubrelUnknown[] = {0x62U, 0x02U, 0x00U, 0x09U};
const uint8_t kUnsubackFour[] = {0xB0U, 0x02U, 0x00U, 0x04U};
const uint8_t kPingresp[] = {0xD0U, 0x00U};

// Scripted loopback peer: one accepted connection answers CONNECT with the scripted bytes and stays open,
// so the test thread can inject packets once connect() has returned.
class scripted_peer
{
public:
    scripted_peer() = default;
    scripted_peer(scripted_peer const&) = delete;
    scripted_peer& operator=(scripted_peer const&) = delete;
    ~scripted_peer() { close_peer(); }

    bool open()
    {
        listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_fd_ < 0) { return false; }
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        socklen_t length = sizeof(address);
        if (bind(listen_fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) != 0 ||
            listen(listen_fd_, 4) != 0 ||
            getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&address), &length) != 0)
        {
            return false;
        }
        port_ = ntohs(address.sin_port);
        return true;
    }

    // Starts the accept thread. `hang_up` closes the accepted socket instead of replying.
    bool serve(uint8_t const* reply, size_t size, bool hang_up = false)
    {
        reply_ = reply;
        reply_size_ = size;
        hang_up_ = hang_up;
        running_ = pthread_create(&thread_, nullptr, &serve_main, this) == 0;
        return running_;
    }

    void join()
    {
        if (running_) { (void)pthread_join(thread_, nullptr); running_ = false; }
    }

    void inject(uint8_t const* bytes, size_t size) const { (void)send(fd_, bytes, size, MSG_NOSIGNAL); }

    // Discards whatever the client has sent so far without blocking.
    void discard_inbound() const
    {
        uint8_t sink[512U];
        while (recv(fd_, sink, sizeof(sink), MSG_DONTWAIT) > 0) {}
    }

    // Aborts the accepted connection with a TCP reset.
    void reset()
    {
        join();
        if (fd_ >= 0)
        {
            linger abort_now{1, 0};
            (void)setsockopt(fd_, SOL_SOCKET, SO_LINGER, &abort_now, sizeof(abort_now));
            ::close(fd_);
            fd_ = -1;
        }
    }

    void close_peer()
    {
        join();
        if (fd_ >= 0) { ::close(fd_); fd_ = -1; }
        if (listen_fd_ >= 0) { ::close(listen_fd_); listen_fd_ = -1; }
    }

    uint16_t port() const { return port_; }

private:
    static void* serve_main(void* argument)
    {
        scripted_peer& self = *static_cast<scripted_peer*>(argument);
        pollfd ready{self.listen_fd_, POLLIN, 0};
        if (poll(&ready, 1, peer_wait_ms) > 0) { self.fd_ = accept(self.listen_fd_, nullptr, nullptr); }
        if (self.fd_ < 0) { return nullptr; }
        uint8_t connect_packet[256U]{};
        pollfd readable{self.fd_, POLLIN, 0};
        if (poll(&readable, 1, peer_wait_ms) > 0) { (void)recv(self.fd_, connect_packet, sizeof(connect_packet), 0); }
        if (self.hang_up_)
        {
            ::close(self.fd_);
            self.fd_ = -1;
            return nullptr;
        }
        if (self.reply_size_ != 0U) { (void)send(self.fd_, self.reply_, self.reply_size_, MSG_NOSIGNAL); }
        return nullptr;
    }

    int listen_fd_ = -1;
    int fd_ = -1;
    uint16_t port_ = 0U;
    uint8_t const* reply_ = nullptr;
    size_t reply_size_ = 0U;
    bool hang_up_ = false;
    bool running_ = false;
    pthread_t thread_{};
};

uint8_t const* connack_for(version level) { return level == version::v5_0 ? kConnackV5Ok : kConnackOk; }
size_t connack_size(version level) { return level == version::v5_0 ? sizeof(kConnackV5Ok) : sizeof(kConnackOk); }

// Runs connect() against a peer that is already serving; the peer thread is joined before returning.
template <typename Client>
castle::status connect_to(Client& client, scripted_peer& peer, options_type const& options,
                          uint8_t const* reply, size_t size)
{
    if (!peer.serve(reply, size)) { return castle::status::system_call_error; }
    castle::status result = client.connect(castle::net::endpoint::loopback_ipv4(peer.port()), options, peer_wait_ms);
    peer.join();
    return result;
}

// Lets the client process whatever the peer has injected so far.
void drain(client_type& client, uint32_t& now)
{
    for (int round = 0; round < 4; ++round)
    {
        now += 10U;
        (void)client.poll(now, 25);
    }
}

// Connects a fresh client at the given protocol level and feeds one scripted packet after a request; returns the poll status.
castle::status feed_after_subscribe(version level, bool unsubscribe, uint8_t const* bytes, size_t size, bool& open)
{
    scripted_peer peer;
    EXPECT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "ack-client";
    options.protocol_version = level;
    options.keep_alive_seconds = 0U;
    EXPECT_EQ(connect_to(client, peer, options, connack_for(level), connack_size(level)), castle::status::ok);
    uint16_t id = 0U;
    castle::status request = unsubscribe ? client.unsubscribe("a/b", id)
                                         : client.subscribe("a/b", qos::at_most_once, id);
    EXPECT_EQ(request, castle::status::ok);
    peer.inject(bytes, size);
    castle::status result = client.poll(10U, 100);
    open = client.is_open();
    return result;
}

// Publishes one message at the given QoS (identifier 1) and feeds one scripted acknowledgement.
castle::status feed_after_publish(version level, qos quality, uint8_t const* bytes, size_t size, bool& open)
{
    scripted_peer peer;
    EXPECT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "ack-publisher";
    options.protocol_version = level;
    options.keep_alive_seconds = 0U;
    EXPECT_EQ(connect_to(client, peer, options, connack_for(level), connack_size(level)), castle::status::ok);
    const uint8_t payload[] = {'x'};
    EXPECT_EQ(client.publish("a/b", payload, sizeof(payload), quality), castle::status::ok);
    peer.inject(bytes, size);
    castle::status result = client.poll(10U, 100);
    open = client.is_open();
    return result;
}

}

TEST(MqttClient, ConnectsAndCompletesQosExchanges)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    size_type delivered = 0U;
    size_type observed[3U]{};
    ASSERT_EQ(client.set_message_callback([&](message_view const& message)
    {
        if (delivered < 3U) { observed[delivered++] = static_cast<size_type>(message.quality); }
    }), castle::status::ok);

    options_type options;
    options.client_id = "client-one";
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);
    EXPECT_TRUE(client.is_connected());
    EXPECT_FALSE(client.session_present());
    uint32_t now = 0U;

    uint16_t subscription = 0U;
    ASSERT_EQ(client.subscribe("a/#", qos::at_least_once, subscription), castle::status::ok);
    EXPECT_EQ(subscription, 1U);
    peer.inject(kSubackOne, sizeof(kSubackOne));
    drain(client, now);
    EXPECT_EQ(client.last_acknowledged_packet_id(), 1U);

    const uint8_t payload[] = {'x'};
    EXPECT_EQ(client.publish("a/b", "hi"), castle::status::ok);
    EXPECT_EQ(client.publish("a/b", payload, sizeof(payload), qos::at_least_once), castle::status::ok);
    peer.inject(kPubackTwo, sizeof(kPubackTwo));
    drain(client, now);
    EXPECT_EQ(client.last_acknowledged_packet_id(), 2U);

    EXPECT_EQ(client.publish("a/b", payload, sizeof(payload), qos::exactly_once, true), castle::status::ok);
    peer.inject(kPubrecThree, sizeof(kPubrecThree));
    drain(client, now);
    EXPECT_EQ(client.last_acknowledged_packet_id(), 3U);
    peer.inject(kPubcompThree, sizeof(kPubcompThree));
    drain(client, now);
    EXPECT_EQ(client.last_acknowledged_packet_id(), 3U);

    peer.inject(kIncomingQos0, sizeof(kIncomingQos0));
    drain(client, now);
    EXPECT_EQ(delivered, 1U);
    peer.inject(kIncomingQos1, sizeof(kIncomingQos1));
    drain(client, now);
    EXPECT_EQ(delivered, 2U);
    peer.inject(kIncomingQos2, sizeof(kIncomingQos2));
    drain(client, now);
    peer.inject(kIncomingQos2, sizeof(kIncomingQos2));
    drain(client, now);
    EXPECT_EQ(delivered, 2U);
    peer.inject(kPubrelEight, sizeof(kPubrelEight));
    drain(client, now);
    EXPECT_EQ(delivered, 3U);
    EXPECT_EQ(observed[0U], 0U);
    EXPECT_EQ(observed[1U], 1U);
    EXPECT_EQ(observed[2U], 2U);
    peer.inject(kPubrelUnknown, sizeof(kPubrelUnknown));
    drain(client, now);
    EXPECT_EQ(delivered, 3U);

    client.clear_message_callback();
    peer.inject(kIncomingQos0, sizeof(kIncomingQos0));
    drain(client, now);
    EXPECT_EQ(delivered, 3U);

    uint16_t unsubscription = 0U;
    EXPECT_EQ(client.unsubscribe("a/#", unsubscription), castle::status::ok);
    EXPECT_EQ(unsubscription, 4U);
    peer.inject(kUnsubackFour, sizeof(kUnsubackFour));
    drain(client, now);
    EXPECT_EQ(client.last_acknowledged_packet_id(), 4U);

    EXPECT_EQ(client.disconnect(), castle::status::ok);
    EXPECT_FALSE(client.is_connected());
    EXPECT_FALSE(client.is_open());
}

TEST(MqttClient, ConnectRejectsNullCallbackAndInvalidOptionsBeforeOpening)
{
    client_type client;
    void (*null_callback)(message_view const&) = nullptr;
    EXPECT_EQ(client.set_message_callback(null_callback), castle::status::invalid_callback);

    const castle::net::endpoint remote = castle::net::endpoint::loopback_ipv4(1883U);
    options_type options;
    options.client_id = "ok";
    options.has_password = true;
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    options.has_password = false;

    char long_name[49U]{};
    for (size_type i = 0U; i < 48U; ++i) { long_name[i] = 'u'; }
    const char bad_utf8[] = {static_cast<char>(0xFF)};
    options.has_username = true;
    options.username = castle::container::string_view(long_name, 48U);
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    options.username = castle::container::string_view(bad_utf8, sizeof(bad_utf8));
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    options.username = "user";
    options.has_password = true;
    options.password = castle::container::string_view(long_name, 48U);
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    options.password = castle::container::string_view(bad_utf8, sizeof(bad_utf8));
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    options.has_password = false;
    options.has_username = false;

    options.client_id = castle::container::string_view(long_name, 48U);
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    options.client_id = castle::container::string_view(bad_utf8, sizeof(bad_utf8));
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    options.client_id = "ok";

    options.has_will = true;
    options.will.topic = "a/+";
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    options.will.topic = "a/b";
    options.will.quality = static_cast<qos>(3U);
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    options.will.quality = qos::at_most_once;
    options.will.payload_size = 65U;
    const uint8_t will_payload[65U]{};
    options.will.payload = will_payload;
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    options.will.payload_size = 1U;
    options.will.payload = nullptr;
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    options.has_will = false;

    options.protocol_version = static_cast<version>(9U);
    EXPECT_EQ(client.connect(remote, options, 1), castle::status::invalid_argument);
    EXPECT_EQ(client.last_error(), castle::status::invalid_argument);
    EXPECT_FALSE(client.is_open());
}

TEST(MqttClient, ConnackReturnCodesMapToStatuses)
{
    struct return_code_case
    {
        uint8_t code;
        castle::status expected;
    };
    const return_code_case cases[] = {
        {1U, castle::status::invalid_config}, {2U, castle::status::invalid_argument},
        {3U, castle::status::time_out},       {4U, castle::status::not_configured},
        {5U, castle::status::not_configured}, {6U, castle::status::invalid_argument},
    };
    for (return_code_case const& item : cases)
    {
        const uint8_t reply[] = {0x20U, 0x02U, 0x00U, item.code};
        scripted_peer peer;
        ASSERT_TRUE(peer.open());
        client_type client;
        options_type options;
        options.client_id = "rejected";
        EXPECT_EQ(connect_to(client, peer, options, reply, sizeof(reply)), item.expected) << static_cast<int>(item.code);
        EXPECT_FALSE(client.is_open());
        EXPECT_FALSE(client.is_connected());
    }
}

TEST(MqttClient, RejectsMalformedHandshakeTraffic)
{
    const uint8_t reserved_flags[] = {0x20U, 0x02U, 0x02U, 0x00U};
    const uint8_t extra_bytes[] = {0x20U, 0x03U, 0x00U, 0x00U, 0x00U};
    const uint8_t short_body[] = {0x20U, 0x01U, 0x00U};
    const uint8_t v5_without_properties[] = {0x20U, 0x02U, 0x00U, 0x00U};
    const uint8_t v5_bad_properties[] = {0x20U, 0x03U, 0x00U, 0x00U, 0x05U};
    const uint8_t v5_unsupported[] = {0x20U, 0x03U, 0x00U, 0x84U, 0x00U};
    const uint8_t v5_unknown[] = {0x20U, 0x03U, 0x00U, 0x87U, 0x00U};
    const uint8_t pingresp_first[] = {0xD0U, 0x00U, 0x20U, 0x02U, 0x00U, 0x00U};
    const uint8_t disconnect_first[] = {0xE0U, 0x00U};
    struct handshake_case
    {
        uint8_t const* bytes;
        size_t size;
        version level;
        castle::status expected;
    };
    const handshake_case cases[] = {
        {reserved_flags, sizeof(reserved_flags), version::v3_1_1, castle::status::invalid_argument},
        {extra_bytes, sizeof(extra_bytes), version::v3_1_1, castle::status::invalid_argument},
        {short_body, sizeof(short_body), version::v3_1_1, castle::status::invalid_argument},
        {v5_without_properties, sizeof(v5_without_properties), version::v5_0, castle::status::invalid_argument},
        {v5_bad_properties, sizeof(v5_bad_properties), version::v5_0, castle::status::invalid_argument},
        {v5_unsupported, sizeof(v5_unsupported), version::v5_0, castle::status::invalid_config},
        {v5_unknown, sizeof(v5_unknown), version::v5_0, castle::status::unknown_error},
        {pingresp_first, sizeof(pingresp_first), version::v3_1_1, castle::status::invalid_argument},
        {disconnect_first, sizeof(disconnect_first), version::v3_1_1, castle::status::invalid_argument},
    };
    for (handshake_case const& item : cases)
    {
        scripted_peer peer;
        ASSERT_TRUE(peer.open());
        client_type client;
        options_type options;
        options.client_id = "malformed";
        options.protocol_version = item.level;
        EXPECT_EQ(connect_to(client, peer, options, item.bytes, item.size), item.expected) << item.size;
        EXPECT_FALSE(client.is_open());
    }
}

TEST(MqttClient, ConnectFailureModesReleaseTheTransport)
{
    scripted_peer closed;
    ASSERT_TRUE(closed.open());
    const uint16_t dead_port = closed.port();
    closed.close_peer();
    client_type refused;
    options_type options;
    options.client_id = "refused";
    EXPECT_NE(refused.connect(castle::net::endpoint::loopback_ipv4(dead_port), options, 200), castle::status::ok);
    EXPECT_FALSE(refused.is_open());

    scripted_peer silent;
    ASSERT_TRUE(silent.open());
    client_type timed_out;
    EXPECT_EQ(connect_to(timed_out, silent, options, nullptr, 0U), castle::status::time_out);
    EXPECT_FALSE(timed_out.is_open());

    scripted_peer hang_up;
    ASSERT_TRUE(hang_up.open());
    client_type dropped;
    ASSERT_TRUE(hang_up.serve(nullptr, 0U, true));
    EXPECT_NE(dropped.connect(castle::net::endpoint::loopback_ipv4(hang_up.port()), options, peer_wait_ms), castle::status::ok);
    hang_up.join();
    EXPECT_FALSE(dropped.is_open());

    scripted_peer accepted;
    ASSERT_TRUE(accepted.open());
    client_type twice;
    ASSERT_EQ(connect_to(twice, accepted, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);
    EXPECT_EQ(twice.connect(castle::net::endpoint::loopback_ipv4(accepted.port()), options, 1), castle::status::invalid_config);
    EXPECT_EQ(twice.disconnect(), castle::status::ok);
}

TEST(MqttClient, ReportsOversizedConnectPropertiesBeforeSending)
{
    char long_text[61U]{};
    for (size_type i = 0U; i < 60U; ++i) { long_text[i] = 'p'; }
    const castle::container::string_view text(long_text, 60U);

    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "oversized";
    options.protocol_version = version::v5_0;
    for (size_type i = 0U; i < 3U; ++i) { ASSERT_EQ(options.properties.add_user_property(text, text), castle::status::ok); }
    ASSERT_TRUE(peer.serve(nullptr, 0U));
    EXPECT_EQ(client.connect(castle::net::endpoint::loopback_ipv4(peer.port()), options, 200), castle::status::full);
    peer.join();
    EXPECT_FALSE(client.is_open());

    scripted_peer will_peer;
    ASSERT_TRUE(will_peer.open());
    client_type will_client;
    options_type will_options;
    will_options.client_id = "oversized-will";
    will_options.protocol_version = version::v5_0;
    will_options.has_will = true;
    will_options.will.topic = "w/t";
    for (size_type i = 0U; i < 3U; ++i) { ASSERT_EQ(will_options.will_properties.add_user_property(text, text), castle::status::ok); }
    ASSERT_TRUE(will_peer.serve(nullptr, 0U));
    EXPECT_EQ(will_client.connect(castle::net::endpoint::loopback_ipv4(will_peer.port()), will_options, 200), castle::status::full);
    will_peer.join();
    EXPECT_FALSE(will_client.is_open());
}

TEST(MqttClient, EncodesProtocolNamesFlagsAndPropertiesOnConnect)
{
    const uint8_t blob[] = {0x01U, 0x02U};
    {
        scripted_peer peer;
        ASSERT_TRUE(peer.open());
        client_type client;
        options_type options;
        options.client_id = "legacy";
        options.protocol_version = version::v3_1_0;
        options.clean_session = false;
        options.has_will = true;
        options.will.topic = "w/t";
        options.will.payload = blob;
        options.will.payload_size = sizeof(blob);
        options.will.quality = qos::at_least_once;
        options.will.retain = true;
        options.has_username = true;
        options.username = "user";
        options.has_password = true;
        options.password = "pw";
        EXPECT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);
        EXPECT_TRUE(client.is_connected());
        EXPECT_EQ(client.disconnect(), castle::status::ok);
    }
    {
        scripted_peer peer;
        ASSERT_TRUE(peer.open());
        client_type client;
        options_type options;
        options.client_id = "modern";
        options.protocol_version = version::v5_0;
        options.clean_session = true;
        options.has_will = true;
        options.will.topic = "w/t";
        options.will.payload = blob;
        options.will.payload_size = sizeof(blob);
        options.will.quality = qos::exactly_once;
        ASSERT_EQ(options.properties.add_integer(property_id::session_expiry_interval, 30U), castle::status::ok);
        ASSERT_EQ(options.will_properties.add_string(property_id::content_type, "json"), castle::status::ok);
        ASSERT_EQ(options.will_properties.add_binary(property_id::correlation_data, blob, sizeof(blob)), castle::status::ok);
        options.has_username = true;
        options.username = "user";
        EXPECT_EQ(connect_to(client, peer, options, kConnackV5Ok, sizeof(kConnackV5Ok)), castle::status::ok);
        EXPECT_EQ(client.disconnect(reason_code::administrative_action), castle::status::ok);
    }
}

TEST(MqttClient, PublishValidatesStateTopicPayloadAndQosSlots)
{
    client_type unconnected;
    EXPECT_EQ(unconnected.publish("a/b", "x"), castle::status::not_configured);

    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "publisher";
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);

    const uint8_t payload[] = {'x'};
    EXPECT_EQ(client.publish("a/+", "x"), castle::status::invalid_argument);
    EXPECT_EQ(client.publish("a/b", static_cast<uint8_t const*>(nullptr), 1U), castle::status::invalid_argument);
    EXPECT_EQ(client.publish("a/b", static_cast<uint8_t const*>(nullptr), 0U), castle::status::ok);

    char long_topic[49U]{};
    for (size_type i = 0U; i < 48U; ++i) { long_topic[i] = 't'; }
    EXPECT_EQ(client.publish(castle::container::string_view(long_topic, 48U), payload, sizeof(payload)), castle::status::out_of_range);

    uint8_t oversized[65U]{};
    EXPECT_EQ(client.publish("a/b", oversized, sizeof(oversized), qos::at_most_once), castle::status::out_of_range);
    EXPECT_EQ(client.publish("a/b", payload, sizeof(payload), static_cast<qos>(3U)), castle::status::out_of_range);

    for (size_type i = 0U; i < 4U; ++i)
    {
        EXPECT_EQ(client.publish("a/b", payload, sizeof(payload), qos::at_least_once), castle::status::ok);
    }
    EXPECT_EQ(client.publish("a/b", payload, sizeof(payload), qos::at_least_once), castle::status::full);
    EXPECT_EQ(client.publish("a/b", payload, sizeof(payload), qos::at_most_once), castle::status::ok);
}

TEST(MqttClient, PublishAndSubscribeReleaseSlotsWhenSendingFails)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    tiny_client_type client;
    options_type options;
    options.client_id = "c";
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);

    uint8_t big_payload[32U]{};
    EXPECT_EQ(client.publish("a/b", big_payload, sizeof(big_payload), qos::at_least_once), castle::status::full);
    EXPECT_EQ(client.publish("a/b", big_payload, sizeof(big_payload), qos::at_most_once), castle::status::full);
    const uint8_t small[] = {'s'};
    EXPECT_EQ(client.publish("a/b", small, sizeof(small), qos::at_least_once), castle::status::ok);
    EXPECT_EQ(client.publish("a/b", small, sizeof(small), qos::at_least_once), castle::status::ok);
    EXPECT_EQ(client.publish("a/b", small, sizeof(small), qos::at_least_once), castle::status::full);
}

TEST(MqttClient, SubscribeReleasesRequestSlotsWhenBodyCannotBeSent)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    tiny_client_type client;
    options_type options;
    options.client_id = "c";
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);

    char long_filter[31U]{};
    for (size_type i = 0U; i < 30U; ++i) { long_filter[i] = 'f'; }
    char medium_filter[19U]{};
    for (size_type i = 0U; i < 18U; ++i) { medium_filter[i] = 'm'; }
    uint16_t id = 0U;
    EXPECT_EQ(client.subscribe(castle::container::string_view(long_filter, 30U), qos::at_most_once, id), castle::status::full);
    EXPECT_EQ(client.subscribe(castle::container::string_view(medium_filter, 18U), qos::at_most_once, id), castle::status::full);
    EXPECT_EQ(client.subscribe("a/b", qos::at_most_once, id), castle::status::ok);
    EXPECT_EQ(client.subscribe("a/c", qos::at_most_once, id), castle::status::ok);
    EXPECT_EQ(client.subscribe("a/d", qos::at_most_once, id), castle::status::full);
}

TEST(MqttClient, UnsubscribeReleasesRequestSlotsWhenBodyCannotBeSent)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    tiny_client_type client;
    options_type options;
    options.client_id = "c";
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);

    char long_filter[31U]{};
    for (size_type i = 0U; i < 30U; ++i) { long_filter[i] = 'f'; }
    char medium_filter[20U]{};
    for (size_type i = 0U; i < 19U; ++i) { medium_filter[i] = 'u'; }
    uint16_t id = 0U;
    EXPECT_EQ(client.unsubscribe(castle::container::string_view(long_filter, 30U), id), castle::status::full);
    EXPECT_EQ(client.unsubscribe(castle::container::string_view(medium_filter, 19U), id), castle::status::full);
    EXPECT_EQ(id, 0U);
    EXPECT_EQ(client.unsubscribe("a/b", id), castle::status::ok);
    EXPECT_EQ(id, 3U);
}


TEST(MqttClient, RejectsOperationsBeforeConnect)
{
    basic_client<128U, 256U, 2U, 48U, 64U> client;
    EXPECT_FALSE(client.is_connected());
    EXPECT_EQ(client.publish("topic", "payload"), castle::status::not_configured);
    uint16_t identifier = 0U;
    EXPECT_EQ(client.subscribe("a/#", qos::at_least_once, identifier), castle::status::not_configured);
    EXPECT_EQ(client.unsubscribe("a/#", identifier), castle::status::not_configured);
    EXPECT_EQ(client.poll(0U), castle::status::not_configured);
    EXPECT_EQ(client.disconnect(), castle::status::not_configured);
}

TEST(MqttClient, CallbackAndConnectValidation)
{
    basic_client<128U, 256U, 2U, 48U, 64U> client;
    size_t calls = 0U;
    EXPECT_EQ(client.set_message_callback([&calls](message_view const&) { ++calls; }), castle::status::ok);
    connect_options<> invalid; invalid.client_id = "";
    EXPECT_EQ(client.connect(castle::net::endpoint::loopback_ipv4(1883U), invalid, 1), castle::status::invalid_argument);
    invalid.client_id = "valid-id";
    invalid.protocol_version = static_cast<version>(6U);
    EXPECT_EQ(client.connect(castle::net::endpoint::loopback_ipv4(1883U), invalid, 1), castle::status::invalid_argument);
    client.clear_message_callback();
    EXPECT_EQ(calls, 0U);
    EXPECT_EQ(client.force_close(), castle::status::not_configured);
}

TEST(MqttClient, PacketIdentifiersWrapAndNeverUseZeroIndirectly)
{
    // Public operations are gated before CONNECT; identifier allocation is exercised by the online sample.
    basic_client<> client;
    EXPECT_EQ(client.publish("x", nullptr, 1U), castle::status::not_configured);
}

TEST(MqttClient, SubscribeAndUnsubscribeValidateArguments)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "args";
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);

    uint16_t id = 99U;
    castle::container::string_view const filters[2U] = {"a/#", "b/+"};
    qos const qualities[2U] = {qos::at_most_once, qos::exactly_once};
    castle::container::string_view const bad_filters[2U] = {"a/#", "a+"};
    EXPECT_EQ(client.subscribe(static_cast<castle::container::string_view const*>(nullptr), qualities, 1U, id), castle::status::invalid_argument);
    EXPECT_EQ(client.subscribe(filters, nullptr, 1U, id), castle::status::invalid_argument);
    EXPECT_EQ(client.subscribe(filters, qualities, 0U, id), castle::status::invalid_argument);
    EXPECT_EQ(client.subscribe(bad_filters, qualities, 2U, id), castle::status::invalid_argument);
    EXPECT_EQ(client.subscribe("a/b", static_cast<qos>(3U), id), castle::status::invalid_argument);
    EXPECT_EQ(id, 0U);
    EXPECT_EQ(client.unsubscribe(static_cast<castle::container::string_view const*>(nullptr), 1U, id), castle::status::invalid_argument);
    EXPECT_EQ(client.unsubscribe(filters, 0U, id), castle::status::invalid_argument);
    EXPECT_EQ(client.unsubscribe(bad_filters, 2U, id), castle::status::invalid_argument);
    EXPECT_EQ(id, 0U);

    uint32_t now = 0U;
    castle::container::string_view const pair[2U] = {"a/b", "c/d"};
    qos const pair_qualities[2U] = {qos::at_most_once, qos::at_most_once};
    uint16_t pair_id = 0U;
    ASSERT_EQ(client.subscribe(pair, pair_qualities, 2U, pair_id), castle::status::ok);
    const uint8_t suback[] = {0x90U, 0x04U, 0x00U, static_cast<uint8_t>(pair_id), 0x00U, 0x01U};
    peer.inject(suback, sizeof(suback));
    drain(client, now);
    EXPECT_EQ(client.last_acknowledged_packet_id(), pair_id);

    uint16_t unsub_id = 0U;
    ASSERT_EQ(client.unsubscribe(pair, 2U, unsub_id), castle::status::ok);
    const uint8_t unsuback[] = {0xB0U, 0x02U, 0x00U, static_cast<uint8_t>(unsub_id)};
    peer.inject(unsuback, sizeof(unsuback));
    drain(client, now);
    EXPECT_EQ(client.last_acknowledged_packet_id(), unsub_id);
    EXPECT_EQ(client.disconnect(), castle::status::ok);
}

TEST(MqttClient, SubscriptionAcknowledgementsAreValidatedAgainstRequests)
{
    const uint8_t failure_code[] = {0x90U, 0x03U, 0x00U, 0x01U, 0x80U};
    const uint8_t unknown_id[] = {0x90U, 0x03U, 0x00U, 0x02U, 0x01U};
    const uint8_t no_codes[] = {0x90U, 0x02U, 0x00U, 0x01U};
    const uint8_t zero_id[] = {0x90U, 0x03U, 0x00U, 0x00U, 0x01U};
    const uint8_t invalid_code[] = {0x90U, 0x03U, 0x00U, 0x01U, 0x03U};
    const uint8_t v5_suback[] = {0x90U, 0x04U, 0x00U, 0x01U, 0x00U, 0x01U};
    const uint8_t v5_bad_properties[] = {0x90U, 0x03U, 0x00U, 0x01U, 0x05U};
    struct ack_case
    {
        uint8_t const* bytes;
        size_t size;
        version level;
        castle::status expected;
        bool keep_open;
    };
    const ack_case cases[] = {
        {failure_code, sizeof(failure_code), version::v3_1_1, castle::status::ok, true},
        {unknown_id, sizeof(unknown_id), version::v3_1_1, castle::status::invalid_argument, false},
        {no_codes, sizeof(no_codes), version::v3_1_1, castle::status::invalid_argument, false},
        {zero_id, sizeof(zero_id), version::v3_1_1, castle::status::invalid_argument, false},
        {invalid_code, sizeof(invalid_code), version::v3_1_1, castle::status::invalid_argument, false},
        {v5_suback, sizeof(v5_suback), version::v5_0, castle::status::ok, true},
        {v5_bad_properties, sizeof(v5_bad_properties), version::v5_0, castle::status::invalid_argument, false},
    };
    for (ack_case const& item : cases)
    {
        bool open = false;
        EXPECT_EQ(feed_after_subscribe(item.level, false, item.bytes, item.size, open), item.expected) << item.size;
        EXPECT_EQ(open, item.keep_open) << item.size;
    }
}

TEST(MqttClient, UnsubscribeAcknowledgementsAreValidatedAgainstRequests)
{
    const uint8_t unsuback[] = {0xB0U, 0x02U, 0x00U, 0x01U};
    const uint8_t unknown_id[] = {0xB0U, 0x02U, 0x00U, 0x07U};
    const uint8_t extra_bytes[] = {0xB0U, 0x03U, 0x00U, 0x01U, 0x00U};
    const uint8_t zero_id[] = {0xB0U, 0x02U, 0x00U, 0x00U};
    const uint8_t v5_unsuback[] = {0xB0U, 0x04U, 0x00U, 0x01U, 0x00U, 0x00U};
    const uint8_t v5_bad_properties[] = {0xB0U, 0x03U, 0x00U, 0x01U, 0x05U};
    const uint8_t suback_for_unsubscribe[] = {0x90U, 0x03U, 0x00U, 0x01U, 0x00U};
    struct ack_case
    {
        uint8_t const* bytes;
        size_t size;
        version level;
        castle::status expected;
    };
    const ack_case cases[] = {
        {unsuback, sizeof(unsuback), version::v3_1_1, castle::status::ok},
        {unknown_id, sizeof(unknown_id), version::v3_1_1, castle::status::invalid_argument},
        {extra_bytes, sizeof(extra_bytes), version::v3_1_1, castle::status::invalid_argument},
        {zero_id, sizeof(zero_id), version::v3_1_1, castle::status::invalid_argument},
        {suback_for_unsubscribe, sizeof(suback_for_unsubscribe), version::v3_1_1, castle::status::invalid_argument},
        {v5_unsuback, sizeof(v5_unsuback), version::v5_0, castle::status::ok},
        {v5_bad_properties, sizeof(v5_bad_properties), version::v5_0, castle::status::invalid_argument},
    };
    for (ack_case const& item : cases)
    {
        bool open = false;
        EXPECT_EQ(feed_after_subscribe(item.level, true, item.bytes, item.size, open), item.expected) << item.size;
    }
}

TEST(MqttClient, PublishAcknowledgementsAreValidatedAgainstOutgoingState)
{
    const uint8_t puback[] = {0x40U, 0x02U, 0x00U, 0x01U};
    const uint8_t unknown_puback[] = {0x40U, 0x02U, 0x00U, 0x02U};
    const uint8_t zero_puback[] = {0x40U, 0x02U, 0x00U, 0x00U};
    const uint8_t wrong_state[] = {0x50U, 0x02U, 0x00U, 0x01U};
    const uint8_t v5_rejected[] = {0x40U, 0x03U, 0x00U, 0x01U, 0x91U};
    const uint8_t v5_success[] = {0x40U, 0x04U, 0x00U, 0x01U, 0x00U, 0x00U};
    const uint8_t v5_without_properties[] = {0x40U, 0x02U, 0x00U, 0x01U};
    const uint8_t v5_reason_only[] = {0x40U, 0x03U, 0x00U, 0x01U, 0x00U};
    const uint8_t pubcomp_early[] = {0x70U, 0x02U, 0x00U, 0x01U};
    const uint8_t v5_pubrec_rejected[] = {0x50U, 0x03U, 0x00U, 0x01U, 0x91U};
    struct ack_case
    {
        uint8_t const* bytes;
        size_t size;
        version level;
        qos quality;
        castle::status expected;
        bool keep_open;
    };
    const ack_case cases[] = {
        {puback, sizeof(puback), version::v3_1_1, qos::at_least_once, castle::status::ok, true},
        {unknown_puback, sizeof(unknown_puback), version::v3_1_1, qos::at_least_once, castle::status::invalid_argument, false},
        {zero_puback, sizeof(zero_puback), version::v3_1_1, qos::at_least_once, castle::status::invalid_argument, false},
        {wrong_state, sizeof(wrong_state), version::v3_1_1, qos::at_least_once, castle::status::invalid_argument, false},
        {v5_rejected, sizeof(v5_rejected), version::v5_0, qos::at_least_once, castle::status::full, false},
        {v5_success, sizeof(v5_success), version::v5_0, qos::at_least_once, castle::status::ok, true},
        {v5_without_properties, sizeof(v5_without_properties), version::v5_0, qos::at_least_once, castle::status::ok, true},
        {v5_reason_only, sizeof(v5_reason_only), version::v5_0, qos::at_least_once, castle::status::ok, true},
        {pubcomp_early, sizeof(pubcomp_early), version::v3_1_1, qos::exactly_once, castle::status::invalid_argument, false},
        {v5_pubrec_rejected, sizeof(v5_pubrec_rejected), version::v5_0, qos::exactly_once, castle::status::full, false},
    };
    for (ack_case const& item : cases)
    {
        bool open = false;
        EXPECT_EQ(feed_after_publish(item.level, item.quality, item.bytes, item.size, open), item.expected) << item.size;
        EXPECT_EQ(open, item.keep_open) << item.size;
    }
}

TEST(MqttClient, QosTwoCompletionReportsRejectedPubcomp)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "qos2";
    options.protocol_version = version::v5_0;
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackV5Ok, sizeof(kConnackV5Ok)), castle::status::ok);
    const uint8_t payload[] = {'z'};
    ASSERT_EQ(client.publish("a/b", payload, sizeof(payload), qos::exactly_once), castle::status::ok);
    const uint8_t pubrec[] = {0x50U, 0x02U, 0x00U, 0x01U};
    peer.inject(pubrec, sizeof(pubrec));
    EXPECT_EQ(client.poll(10U, 100), castle::status::ok);
    EXPECT_EQ(client.last_acknowledged_packet_id(), 1U);
    const uint8_t pubcomp_rejected[] = {0x70U, 0x03U, 0x00U, 0x01U, 0x92U};
    peer.inject(pubcomp_rejected, sizeof(pubcomp_rejected));
    EXPECT_EQ(client.poll(20U, 100), castle::status::not_found);
    EXPECT_FALSE(client.is_open());
}

TEST(MqttClient, RejectsMalformedInboundPublishes)
{
    const uint8_t qos3[] = {0x36U, 0x07U, 0x00U, 0x03U, 'a', '/', 'b', 'h', 'i'};
    const uint8_t wildcard_topic[] = {0x30U, 0x05U, 0x00U, 0x03U, 'a', '/', '+'};
    const uint8_t zero_identifier[] = {0x32U, 0x07U, 0x00U, 0x03U, 'a', '/', 'b', 0x00U, 0x00U};
    const uint8_t v5_bad_properties[] = {0x30U, 0x06U, 0x00U, 0x03U, 'a', '/', 'b', 0x05U};
    uint8_t oversized[72U]{0x30U, 0x46U, 0x00U, 0x03U, 'a', '/', 'b'};
    struct inbound_case
    {
        uint8_t const* bytes;
        size_t size;
        version level;
        castle::status expected;
        bool keep_open;
    };
    const inbound_case cases[] = {
        {qos3, sizeof(qos3), version::v3_1_1, castle::status::invalid_argument, false},
        {wildcard_topic, sizeof(wildcard_topic), version::v3_1_1, castle::status::invalid_argument, false},
        {zero_identifier, sizeof(zero_identifier), version::v3_1_1, castle::status::invalid_argument, false},
        {v5_bad_properties, sizeof(v5_bad_properties), version::v5_0, castle::status::invalid_argument, false},
        {oversized, sizeof(oversized), version::v3_1_1, castle::status::out_of_range, false},
        {kIncomingQos0, sizeof(kIncomingQos0), version::v3_1_1, castle::status::ok, true},
    };
    for (inbound_case const& item : cases)
    {
        scripted_peer peer;
        ASSERT_TRUE(peer.open());
        client_type client;
        options_type options;
        options.client_id = "inbound";
        options.protocol_version = item.level;
        options.keep_alive_seconds = 0U;
        ASSERT_EQ(connect_to(client, peer, options, connack_for(item.level), connack_size(item.level)), castle::status::ok);
        peer.inject(item.bytes, item.size);
        EXPECT_EQ(client.poll(10U, 100), item.expected) << item.size;
        EXPECT_EQ(client.is_open(), item.keep_open) << item.size;
    }
}

TEST(MqttClient, IncomingQosTwoStoreIsBounded)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "bounded";
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);
    uint8_t burst[50U]{};
    for (uint8_t i = 0U; i < 5U; ++i)
    {
        uint8_t* packet = burst + i * 10U;
        const uint8_t header[] = {0x34U, 0x08U, 0x00U, 0x03U, 'a', '/', 'b', 0x00U, static_cast<uint8_t>(i + 1U), 'y'};
        for (size_type j = 0U; j < sizeof(header); ++j) { packet[j] = header[j]; }
    }
    peer.inject(burst, sizeof(burst));
    EXPECT_EQ(client.poll(10U, 100), castle::status::full);
    EXPECT_FALSE(client.is_open());
}

TEST(MqttClient, KeepAliveSendsPingAndDetectsMissingPingResponse)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "keep-alive";
    options.keep_alive_seconds = 1U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);

    EXPECT_EQ(client.poll(0U, 0), castle::status::empty);
    EXPECT_EQ(client.poll(1500U, 0), castle::status::empty);
    peer.inject(kPingresp, sizeof(kPingresp));
    EXPECT_EQ(client.poll(1600U, 100), castle::status::ok);
    EXPECT_EQ(client.poll(2600U, 0), castle::status::empty);
    EXPECT_EQ(client.poll(4000U, 0), castle::status::time_out);
    EXPECT_FALSE(client.is_open());
}

TEST(MqttClient, UnsolicitedPingResponseIsRejected)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "unsolicited";
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);
    peer.inject(kPingresp, sizeof(kPingresp));
    EXPECT_EQ(client.poll(10U, 100), castle::status::invalid_argument);
    EXPECT_FALSE(client.is_open());
}

TEST(MqttClient, ServerDisconnectClosesTheSession)
{
    const uint8_t v3_disconnect[] = {0xE0U, 0x00U};
    const uint8_t v5_empty_disconnect[] = {0xE0U, 0x00U};
    const uint8_t v5_disconnect[] = {0xE0U, 0x02U, 0x04U, 0x00U};
    const uint8_t v5_reason_disconnect[] = {0xE0U, 0x01U, 0x04U};
    struct disconnect_case
    {
        uint8_t const* bytes;
        size_t size;
        version level;
    };
    const disconnect_case cases[] = {
        {v3_disconnect, sizeof(v3_disconnect), version::v3_1_1},
        {v5_empty_disconnect, sizeof(v5_empty_disconnect), version::v5_0},
        {v5_disconnect, sizeof(v5_disconnect), version::v5_0},
        {v5_reason_disconnect, sizeof(v5_reason_disconnect), version::v5_0},
    };
    for (disconnect_case const& item : cases)
    {
        scripted_peer peer;
        ASSERT_TRUE(peer.open());
        client_type client;
        options_type options;
        options.client_id = "disconnected";
        options.protocol_version = item.level;
        options.keep_alive_seconds = 0U;
        ASSERT_EQ(connect_to(client, peer, options, connack_for(item.level), connack_size(item.level)), castle::status::ok);
        peer.inject(item.bytes, item.size);
        EXPECT_EQ(client.poll(10U, 100), castle::status::not_configured) << item.size;
        EXPECT_FALSE(client.is_connected());
        EXPECT_FALSE(client.is_open());
        EXPECT_EQ(client.disconnect(), castle::status::not_configured);
    }
}

TEST(MqttClient, ResumedSessionResendsUnacknowledgedPublishes)
{
    const version levels[] = {version::v3_1_1, version::v5_0};
    for (version level : levels)
    {
        options_type options;
        options.client_id = "resume";
        options.protocol_version = level;
        options.clean_session = false;
        options.keep_alive_seconds = 0U;
        const uint8_t session_present_v3[] = {0x20U, 0x02U, 0x01U, 0x00U};
        const uint8_t session_present_v5[] = {0x20U, 0x03U, 0x01U, 0x00U, 0x00U};
        uint8_t const* resumed_reply = level == version::v5_0 ? session_present_v5 : session_present_v3;
        size_t resumed_size = level == version::v5_0 ? sizeof(session_present_v5) : sizeof(session_present_v3);

        client_type client;
        {
            scripted_peer first;
            ASSERT_TRUE(first.open());
            ASSERT_EQ(connect_to(client, first, options, connack_for(level), connack_size(level)), castle::status::ok);
            EXPECT_FALSE(client.session_present());
            const uint8_t payload[] = {'x'};
            ASSERT_EQ(client.publish("a/b", payload, sizeof(payload), qos::at_least_once), castle::status::ok);
            ASSERT_EQ(client.publish("a/b", payload, sizeof(payload), qos::exactly_once), castle::status::ok);
            const uint8_t pubrec[] = {0x50U, 0x02U, 0x00U, 0x02U};
            first.inject(pubrec, sizeof(pubrec));
            uint32_t now = 0U;
            drain(client, now);
            EXPECT_EQ(client.last_acknowledged_packet_id(), 2U);
            client.force_close();
        }

        scripted_peer second;
        ASSERT_TRUE(second.open());
        ASSERT_EQ(connect_to(client, second, options, resumed_reply, resumed_size), castle::status::ok);
        EXPECT_TRUE(client.session_present());
        EXPECT_EQ(client.disconnect(), castle::status::ok);
    }
}

TEST(MqttClient, ConnectsWithSessionCleanStartAndDisconnectsWithReason)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "clean";
    options.protocol_version = version::v5_0;
    options.clean_session = true;
    options.keep_alive_seconds = 0U;
    const uint8_t session_present_v5[] = {0x20U, 0x03U, 0x01U, 0x00U, 0x00U};
    ASSERT_EQ(connect_to(client, peer, options, session_present_v5, sizeof(session_present_v5)), castle::status::ok);
    EXPECT_TRUE(client.session_present());
    EXPECT_EQ(client.disconnect(reason_code::administrative_action), castle::status::ok);
    EXPECT_FALSE(client.is_open());
}

TEST(MqttClient, PacketIdentifiersWrapAroundSkippingInUseValues)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "wrap";
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);

    const uint8_t payload[] = {'x'};
    uint32_t now = 0U;
    ASSERT_EQ(client.publish("a/b", payload, sizeof(payload), qos::at_least_once), castle::status::ok);
    uint16_t pending_subscription = 0U;
    ASSERT_EQ(client.subscribe("x/y", qos::at_most_once, pending_subscription), castle::status::ok);
    ASSERT_EQ(pending_subscription, 2U);

    uint16_t expected = 3U;
    for (uint32_t round = 0U; round < 65533U; ++round)
    {
        ASSERT_EQ(client.publish("a/b", payload, sizeof(payload), qos::at_least_once), castle::status::ok) << round;
        const uint8_t puback[] = {0x40U, 0x02U, static_cast<uint8_t>(expected >> 8U), static_cast<uint8_t>(expected)};
        peer.inject(puback, sizeof(puback));
        ASSERT_EQ(client.poll(++now, 10), castle::status::ok) << round;
        ASSERT_EQ(client.last_acknowledged_packet_id(), expected) << round;
        peer.discard_inbound();
        expected = expected == 65535U ? 3U : static_cast<uint16_t>(expected + 1U);
    }

    ASSERT_EQ(client.publish("a/b", payload, sizeof(payload), qos::at_least_once), castle::status::ok);
    const uint8_t final_ack[] = {0x40U, 0x02U, 0x00U, 0x03U};
    peer.inject(final_ack, sizeof(final_ack));
    EXPECT_EQ(client.poll(++now, 100), castle::status::ok);
    EXPECT_EQ(client.last_acknowledged_packet_id(), 3U);
    EXPECT_EQ(client.disconnect(), castle::status::ok);
}

TEST(MqttClient, IncomingQosFlowsDeliverOnceAndAcknowledge)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "v5-inbound";
    options.protocol_version = version::v5_0;
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackV5Ok, sizeof(kConnackV5Ok)), castle::status::ok);

    size_type delivered = 0U;
    ASSERT_EQ(client.set_message_callback([&delivered](message_view const&) { ++delivered; }), castle::status::ok);
    uint32_t now = 0U;

    const uint8_t qos0[] = {0x30U, 0x08U, 0x00U, 0x03U, 'a', '/', 'b', 0x00U, 'h', 'i'};
    peer.inject(qos0, sizeof(qos0));
    drain(client, now);
    EXPECT_EQ(delivered, 1U);

    const uint8_t qos1[] = {0x32U, 0x09U, 0x00U, 0x03U, 'a', '/', 'b', 0x00U, 0x04U, 0x00U, 'x'};
    peer.inject(qos1, sizeof(qos1));
    drain(client, now);
    EXPECT_EQ(delivered, 2U);

    const uint8_t qos2[] = {0x34U, 0x09U, 0x00U, 0x03U, 'a', '/', 'b', 0x00U, 0x05U, 0x00U, 'y'};
    peer.inject(qos2, sizeof(qos2));
    drain(client, now);
    EXPECT_EQ(delivered, 2U);
    const uint8_t pubrel[] = {0x62U, 0x04U, 0x00U, 0x05U, 0x00U, 0x00U};
    peer.inject(pubrel, sizeof(pubrel));
    drain(client, now);
    EXPECT_EQ(delivered, 3U);
    const uint8_t unknown_pubrel[] = {0x62U, 0x04U, 0x00U, 0x06U, 0x00U, 0x00U};
    peer.inject(unknown_pubrel, sizeof(unknown_pubrel));
    drain(client, now);
    EXPECT_EQ(delivered, 3U);
    const uint8_t pubrel_without_properties[] = {0x62U, 0x02U, 0x00U, 0x0AU};
    peer.inject(pubrel_without_properties, sizeof(pubrel_without_properties));
    drain(client, now);
    const uint8_t pubrel_reason_only[] = {0x62U, 0x03U, 0x00U, 0x0BU, 0x00U};
    peer.inject(pubrel_reason_only, sizeof(pubrel_reason_only));
    drain(client, now);
    EXPECT_EQ(delivered, 3U);
    EXPECT_TRUE(client.is_open());

    client.clear_message_callback();
    const uint8_t silent_qos2[] = {0x34U, 0x09U, 0x00U, 0x03U, 'a', '/', 'b', 0x00U, 0x07U, 0x00U, 'z'};
    peer.inject(silent_qos2, sizeof(silent_qos2));
    drain(client, now);
    const uint8_t silent_pubrel[] = {0x62U, 0x04U, 0x00U, 0x07U, 0x00U, 0x00U};
    peer.inject(silent_pubrel, sizeof(silent_pubrel));
    drain(client, now);
    EXPECT_EQ(delivered, 3U);
    EXPECT_TRUE(client.is_open());
    EXPECT_EQ(client.disconnect(reason_code::administrative_action), castle::status::ok);
}

TEST(MqttClient, MessageCallbackCanReplaceAndClearItselfDuringDelivery)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "callback-self-update";
    options.protocol_version = version::v5_0;
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackV5Ok, sizeof(kConnackV5Ok)), castle::status::ok);

    size_type first_calls = 0U;
    size_type second_calls = 0U;
    size_type third_calls = 0U;
    ASSERT_EQ(client.set_message_callback([&](message_view const&)
    {
        ++first_calls;
        EXPECT_EQ(client.set_message_callback([&second_calls](message_view const&) { ++second_calls; }), castle::status::ok);
    }), castle::status::ok);
    uint32_t now = 0U;

    const uint8_t qos0[] = {0x30U, 0x08U, 0x00U, 0x03U, 'a', '/', 'b', 0x00U, 'h', 'i'};
    peer.inject(qos0, sizeof(qos0));
    drain(client, now);
    EXPECT_EQ(first_calls, 1U);
    EXPECT_EQ(second_calls, 0U);

    peer.inject(qos0, sizeof(qos0));
    drain(client, now);
    EXPECT_EQ(first_calls, 1U);
    EXPECT_EQ(second_calls, 1U);

    ASSERT_EQ(client.set_message_callback([&](message_view const&)
    {
        ++third_calls;
        client.clear_message_callback();
    }), castle::status::ok);
    peer.inject(qos0, sizeof(qos0));
    drain(client, now);
    EXPECT_EQ(third_calls, 1U);

    peer.inject(qos0, sizeof(qos0));
    drain(client, now);
    EXPECT_EQ(third_calls, 1U);
    EXPECT_EQ(second_calls, 1U);
    EXPECT_TRUE(client.is_open());
    EXPECT_EQ(client.disconnect(reason_code::administrative_action), castle::status::ok);
}

TEST(MqttClient, BuffersPartialPacketsAcrossReceiveChunks)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "partial";
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);

    size_type delivered = 0U;
    ASSERT_EQ(client.set_message_callback([&delivered](message_view const&) { ++delivered; }), castle::status::ok);
    uint8_t combined[sizeof(kIncomingQos0) + 4U]{};
    for (size_type i = 0U; i < sizeof(kIncomingQos0); ++i) { combined[i] = kIncomingQos0[i]; }
    for (size_type i = 0U; i < 4U; ++i) { combined[sizeof(kIncomingQos0) + i] = kIncomingQos1[i]; }
    peer.inject(combined, sizeof(combined));
    EXPECT_EQ(client.poll(10U, 100), castle::status::ok);
    EXPECT_EQ(delivered, 1U);

    peer.inject(kIncomingQos1 + 4U, 3U);
    EXPECT_EQ(client.poll(15U, 100), castle::status::ok);
    EXPECT_EQ(delivered, 1U);
    peer.inject(kIncomingQos1 + 7U, sizeof(kIncomingQos1) - 7U);
    EXPECT_EQ(client.poll(20U, 100), castle::status::ok);
    EXPECT_EQ(delivered, 2U);
    EXPECT_EQ(client.disconnect(), castle::status::ok);
}

TEST(MqttClient, UnexpectedAndMalformedPacketsCloseTheSession)
{
    const uint8_t auth_packet[] = {0xF0U, 0x00U};
    const uint8_t connect_packet[] = {0x10U, 0x00U};
    const uint8_t pingresp_with_body[] = {0xD0U, 0x01U, 0x00U};
    const uint8_t second_connack[] = {0x20U, 0x02U, 0x00U, 0x00U};
    const uint8_t empty_puback[] = {0x40U, 0x00U};
    const uint8_t empty_pubrel[] = {0x62U, 0x00U};
    const uint8_t zero_pubrel[] = {0x62U, 0x02U, 0x00U, 0x00U};
    const uint8_t v3_pubrel_extra[] = {0x62U, 0x03U, 0x00U, 0x08U, 0x00U};
    const uint8_t v3_puback_extra[] = {0x40U, 0x03U, 0x00U, 0x01U, 0x00U};
    const uint8_t empty_suback[] = {0x90U, 0x00U};
    const uint8_t empty_unsuback[] = {0xB0U, 0x00U};
    const uint8_t qos1_without_identifier[] = {0x32U, 0x05U, 0x00U, 0x03U, 'a', '/', 'b'};
    const uint8_t truncated_topic[] = {0x30U, 0x03U, 0x00U, 0x05U, 'a'};
    struct inbound_case
    {
        uint8_t const* bytes;
        size_t size;
        castle::status expected;
    };
    const inbound_case cases[] = {
        {auth_packet, sizeof(auth_packet), castle::status::invalid_config},
        {connect_packet, sizeof(connect_packet), castle::status::invalid_argument},
        {pingresp_with_body, sizeof(pingresp_with_body), castle::status::invalid_argument},
        {second_connack, sizeof(second_connack), castle::status::invalid_argument},
        {empty_puback, sizeof(empty_puback), castle::status::invalid_argument},
        {empty_pubrel, sizeof(empty_pubrel), castle::status::invalid_argument},
        {zero_pubrel, sizeof(zero_pubrel), castle::status::invalid_argument},
        {v3_pubrel_extra, sizeof(v3_pubrel_extra), castle::status::invalid_argument},
        {v3_puback_extra, sizeof(v3_puback_extra), castle::status::invalid_argument},
        {empty_suback, sizeof(empty_suback), castle::status::invalid_argument},
        {empty_unsuback, sizeof(empty_unsuback), castle::status::invalid_argument},
        {qos1_without_identifier, sizeof(qos1_without_identifier), castle::status::invalid_argument},
        {truncated_topic, sizeof(truncated_topic), castle::status::invalid_argument},
    };
    for (inbound_case const& item : cases)
    {
        scripted_peer peer;
        ASSERT_TRUE(peer.open());
        client_type client;
        options_type options;
        options.client_id = "hostile";
        options.keep_alive_seconds = 0U;
        ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);
        peer.inject(item.bytes, item.size);
        EXPECT_EQ(client.poll(10U, 100), item.expected) << item.size;
        EXPECT_FALSE(client.is_open()) << item.size;
    }
}

TEST(MqttClient, KeepAliveWaitsForOutstandingPingWithinDeadline)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "keep-alive-window";
    options.keep_alive_seconds = 1U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);

    EXPECT_EQ(client.poll(0U, 0), castle::status::empty);
    EXPECT_EQ(client.poll(1500U, 0), castle::status::empty);
    EXPECT_EQ(client.poll(2000U, 0), castle::status::empty);
    EXPECT_TRUE(client.is_open());
    peer.inject(kPingresp, sizeof(kPingresp));
    EXPECT_EQ(client.poll(2100U, 100), castle::status::ok);
    EXPECT_EQ(client.poll(2600U, 0), castle::status::empty);
    EXPECT_EQ(client.poll(2700U, 0), castle::status::empty);
    EXPECT_TRUE(client.is_open());
    EXPECT_EQ(client.poll(4000U, 0), castle::status::time_out);
    EXPECT_FALSE(client.is_open());
}

TEST(MqttClient, ResetConnectionSurfacesTransportErrors)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "reset";
    options.keep_alive_seconds = 1U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);
    EXPECT_EQ(client.poll(0U, 0), castle::status::empty);

    peer.reset();
    EXPECT_NE(client.poll(10U, 100), castle::status::ok);
    const uint8_t payload[] = {'x'};
    EXPECT_NE(client.publish("a/b", payload, sizeof(payload), qos::at_least_once), castle::status::ok);
    EXPECT_NE(client.last_error(), castle::status::ok);
    EXPECT_NE(client.poll(1500U, 0), castle::status::ok);
    EXPECT_FALSE(client.is_open());
    EXPECT_EQ(client.disconnect(), castle::status::not_configured);
}

TEST(MqttClient, UnsubscribeReservesRequestSlots)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    client_type client;
    options_type options;
    options.client_id = "unsub-slots";
    options.keep_alive_seconds = 0U;
    ASSERT_EQ(connect_to(client, peer, options, kConnackOk, sizeof(kConnackOk)), castle::status::ok);

    uint16_t id = 0U;
    for (size_type i = 0U; i < 4U; ++i)
    {
        EXPECT_EQ(client.unsubscribe("a/b", id), castle::status::ok) << i;
    }
    EXPECT_EQ(client.unsubscribe("a/b", id), castle::status::full);
    EXPECT_EQ(client.disconnect(), castle::status::ok);
}

TEST(MqttClient, ConnectRejectsConnectPacketThatExceedsPacketCapacity)
{
    scripted_peer peer;
    ASSERT_TRUE(peer.open());
    tiny_client_type client;
    options_type options;
    options.client_id = "0123456789012345678901234567890";
    ASSERT_TRUE(peer.serve(nullptr, 0U));
    EXPECT_EQ(client.connect(castle::net::endpoint::loopback_ipv4(peer.port()), options, 200), castle::status::full);
    peer.join();
    EXPECT_FALSE(client.is_open());
}



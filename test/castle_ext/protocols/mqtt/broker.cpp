#include <gtest/gtest.h>
#include "castle_ext/protocols/mqtt/broker.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstddef>
#include <cstring>
#include <string>
#include <vector>

using namespace castle::protocols::mqtt;

namespace
{

using broker_type = basic_broker<2U, 128U, 256U, 8U, 4U, 2U, 48U, 64U>;
using wide_type = basic_broker<4U, 128U, 256U, 8U, 4U, 2U, 48U, 64U>;
using tiny_type = basic_broker<4U, 64U, 64U, 4U, 2U, 2U, 32U, 40U>;
using handle_type = broker_type::server_type::client_handle;
using bytes = std::vector<uint8_t>;

constexpr int io_wait_ms = 1000;

const bytes kConnackV4{0x20U, 0x02U, 0x00U, 0x00U};
const bytes kConnackV4Session{0x20U, 0x02U, 0x01U, 0x00U};
const bytes kConnackV5{0x20U, 0x03U, 0x00U, 0x00U, 0x00U};
const bytes kConnackV5Session{0x20U, 0x03U, 0x01U, 0x00U, 0x00U};
const bytes kPingreq{0xC0U, 0x00U};
const bytes kPingresp{0xD0U, 0x00U};
const bytes kDisconnect{0xE0U, 0x00U};

void append(bytes& out, bytes const& more)
{
    out.insert(out.end(), more.begin(), more.end());
}

void put_u16(bytes& out, size_t value)
{
    out.push_back(static_cast<uint8_t>(value >> 8U));
    out.push_back(static_cast<uint8_t>(value));
}

// Bytes of a C string without a length prefix.
bytes raw(char const* value)
{
    bytes out;
    for (char const* c = value; *c != '\0'; ++c) { out.push_back(static_cast<uint8_t>(*c)); }
    return out;
}

// Length-prefixed UTF-8 string.
bytes text(char const* value)
{
    bytes out;
    put_u16(out, std::strlen(value));
    append(out, raw(value));
    return out;
}

// Length-prefixed binary data.
bytes binary(bytes const& data)
{
    bytes out;
    put_u16(out, data.size());
    append(out, data);
    return out;
}

bytes filled(size_t count)
{
    return bytes(count, static_cast<uint8_t>('x'));
}

bytes zeros(size_t count)
{
    return bytes(count, static_cast<uint8_t>(0U));
}

bytes prefix(bytes const& data, size_t count)
{
    return bytes(data.begin(), data.begin() + static_cast<std::ptrdiff_t>(count));
}

bytes concat_tail(bytes head, bytes const& tail)
{
    append(head, tail);
    return head;
}

// Property block: one length byte followed by the encoded properties.
bytes block(bytes const& properties)
{
    bytes out{static_cast<uint8_t>(properties.size())};
    append(out, properties);
    return out;
}

bytes session_expiry(uint32_t seconds)
{
    return bytes{0x11U, static_cast<uint8_t>(seconds >> 24U), static_cast<uint8_t>(seconds >> 16U),
                 static_cast<uint8_t>(seconds >> 8U), static_cast<uint8_t>(seconds)};
}

bytes will_fields(char const* topic, char const* payload)
{
    bytes out = text(topic);
    append(out, binary(raw(payload)));
    return out;
}

bytes packet(uint8_t first, bytes const& body)
{
    bytes out{first};
    size_t remaining = body.size();
    do
    {
        uint8_t digit = static_cast<uint8_t>(remaining % 128U);
        remaining /= 128U;
        out.push_back(remaining != 0U ? static_cast<uint8_t>(digit | 0x80U) : digit);
    } while (remaining != 0U);
    append(out, body);
    return out;
}

bytes connect_body(char const* name, uint8_t level, uint8_t flags, char const* client_id,
                   bytes const& tail = {}, uint16_t keep_alive = 0U, bytes const& properties = {})
{
    bytes body = text(name);
    body.push_back(level);
    body.push_back(flags);
    put_u16(body, keep_alive);
    if (level == 5U) { append(body, block(properties)); }
    append(body, text(client_id));
    append(body, tail);
    return body;
}

bytes connect(char const* name, uint8_t level, uint8_t flags, char const* client_id,
              bytes const& tail = {}, uint16_t keep_alive = 0U, bytes const& properties = {})
{
    return packet(0x10U, connect_body(name, level, flags, client_id, tail, keep_alive, properties));
}

bytes connect4(char const* client_id, uint8_t flags = 0x02U, bytes const& tail = {}, uint16_t keep_alive = 0U)
{
    return connect("MQTT", 4U, flags, client_id, tail, keep_alive);
}

bytes connect5(char const* client_id, uint8_t flags = 0x02U, bytes const& tail = {}, bytes const& properties = {})
{
    return connect("MQTT", 5U, flags, client_id, tail, 0U, properties);
}

bytes publish(char const* topic, uint8_t quality, bool retain, uint16_t identifier, bytes const& payload,
              bool v5 = false, bytes const& properties = {})
{
    bytes body = text(topic);
    if (quality != 0U) { put_u16(body, identifier); }
    if (v5) { append(body, block(properties)); }
    append(body, payload);
    return packet(static_cast<uint8_t>(0x30U | (quality << 1U) | (retain ? 1U : 0U)), body);
}

// Topic filter followed by its subscription options byte.
bytes filter(char const* value, uint8_t options = 0U)
{
    bytes out = text(value);
    out.push_back(options);
    return out;
}

bytes subscribe(uint16_t identifier, bytes const& filters, bool v5 = false)
{
    bytes body;
    put_u16(body, identifier);
    if (v5) { body.push_back(0U); }
    append(body, filters);
    return packet(0x82U, body);
}

bytes unsubscribe(uint16_t identifier, bytes const& filters, bool v5 = false)
{
    bytes body;
    put_u16(body, identifier);
    if (v5) { body.push_back(0U); }
    append(body, filters);
    return packet(0xA2U, body);
}

// Packet with a two-byte identifier followed by optional tail bytes (PUBACK, PUBREC, PUBREL, PUBCOMP).
bytes ack(uint8_t first, uint16_t identifier, bytes const& tail = {})
{
    bytes body;
    put_u16(body, identifier);
    append(body, tail);
    return packet(first, body);
}

bytes suback(uint16_t identifier, bytes const& reasons, bool v5 = false)
{
    bytes body;
    put_u16(body, identifier);
    if (v5) { body.push_back(0U); }
    append(body, reasons);
    return packet(0x90U, body);
}

bytes unsuback(uint16_t identifier, bytes const& results = {}, bool v5 = false)
{
    bytes body;
    put_u16(body, identifier);
    if (v5) { body.push_back(0U); }
    append(body, results);
    return packet(0xB0U, body);
}

// Connected or unconnected loopback TCP peer driven from the test thread.
class raw_peer
{
public:
    raw_peer() = default;
    raw_peer(raw_peer const&) = delete;
    raw_peer& operator=(raw_peer const&) = delete;
    ~raw_peer() { close(); }

    bool connect_to(uint16_t port)
    {
        close();
        fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ < 0) { return false; }
        int no_delay = 1;
        // Small writes must not wait for the broker's delayed ACKs (Nagle), or multi-step exchanges stall.
        (void)::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &no_delay, sizeof(no_delay));
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        address.sin_port = htons(port);
        return fd_ >= 0 && ::connect(fd_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
    }

    bool send(bytes const& data) const
    {
        return ::send(fd_, data.data(), data.size(), MSG_NOSIGNAL) == static_cast<ssize_t>(data.size());
    }

    // Reads one whole MQTT packet; false on timeout, EOF or reset.
    bool read_packet(bytes& out) const
    {
        out.clear();
        if (!read_bytes(out, 1U)) { return false; }
        size_t remaining = 0U;
        size_t multiplier = 1U;
        uint8_t digit = 0x80U;
        for (size_t i = 0U; (digit & 0x80U) != 0U; ++i)
        {
            if (i == 4U || !read_bytes(out, 1U)) { return false; }
            digit = out.back();
            remaining += static_cast<size_t>(digit & 0x7FU) * multiplier;
            multiplier *= 128U;
        }
        return read_bytes(out, remaining);
    }

    // Waits for the broker to close the connection; bytes received before that are appended to `leftover`.
    bool wait_closed(bytes& leftover) const
    {
        for (int round = 0; round < 64; ++round)
        {
            if (!wait_readable(io_wait_ms)) { return false; }
            uint8_t chunk[256U];
            ssize_t received = ::recv(fd_, chunk, sizeof(chunk), 0);
            if (received <= 0) { return true; }
            for (ssize_t i = 0; i < received; ++i) { leftover.push_back(chunk[i]); }
        }
        return false;
    }

    bool wait_closed() const
    {
        bytes leftover;
        return wait_closed(leftover);
    }

    // True when nothing is pending within a short window.
    bool silent() const { return !wait_readable(30); }

    // Closes with a TCP reset instead of a FIN.
    void reset()
    {
        if (fd_ >= 0)
        {
            linger abort_now{1, 0};
            (void)::setsockopt(fd_, SOL_SOCKET, SO_LINGER, &abort_now, sizeof(abort_now));
        }
        close();
    }

    void close()
    {
        if (fd_ >= 0)
        {
            ::close(fd_);
            fd_ = -1;
        }
    }

private:
    bool wait_readable(int milliseconds) const
    {
        pollfd ready{fd_, POLLIN, 0};
        return fd_ >= 0 && ::poll(&ready, 1, milliseconds) > 0;
    }

    bool read_bytes(bytes& out, size_t count) const
    {
        while (count != 0U)
        {
            if (!wait_readable(io_wait_ms)) { return false; }
            int quick_ack = 1;
            // Acknowledge pending data immediately so the broker's Nagle-held second write is released.
            (void)::setsockopt(fd_, IPPROTO_TCP, TCP_QUICKACK, &quick_ack, sizeof(quick_ack));
            uint8_t chunk[256U];
            size_t want = count < sizeof(chunk) ? count : sizeof(chunk);
            ssize_t received = ::recv(fd_, chunk, want, 0);
            if (received <= 0) { return false; }
            for (ssize_t i = 0; i < received; ++i) { out.push_back(chunk[i]); }
            count -= static_cast<size_t>(received);
        }
        return true;
    }

    int fd_ = -1;
};

// Synthetic clock: every step is deterministic and polls without blocking.
template <typename Broker>
class timeline
{
public:
    explicit timeline(Broker& broker) : broker_(broker) {}

    uint32_t now() const { return now_; }
    void advance(uint32_t milliseconds) { now_ += milliseconds; }

    castle::status tick()
    {
        now_ += 10U;
        return broker_.poll(now_, 0);
    }

    castle::status poll_at(uint32_t when)
    {
        now_ = when;
        return broker_.poll(now_, 0);
    }

    // Runs enough ticks to drain every buffered receive chunk.
    void pump(int rounds = 4)
    {
        for (int round = 0; round < rounds; ++round) { EXPECT_EQ(tick(), castle::status::ok); }
    }

    // Accepts one pending connection, waiting briefly for it.
    castle::status accept()
    {
        now_ += 10U;
        return broker_.poll(now_, 100);
    }

private:
    Broker& broker_;
    uint32_t now_ = 1000U;
};

template <typename Broker>
uint16_t open_loopback(Broker& broker)
{
    EXPECT_EQ(broker.open(castle::net::endpoint::loopback_ipv4(0U)), castle::status::ok);
    castle::net::endpoint bound;
    EXPECT_EQ(broker.transport().socket().local_endpoint(bound), castle::status::ok);
    return static_cast<uint16_t>(bound.port());
}

template <typename Broker>
struct harness
{
    Broker broker;
    uint16_t port;
    timeline<Broker> timing;

    harness() : port(open_loopback(broker)), timing(broker) {}
};

template <typename Broker>
bool open_connection(harness<Broker>& h, raw_peer& peer)
{
    auto before = h.broker.client_count();
    return peer.connect_to(h.port) && h.timing.accept() == castle::status::ok &&
           h.broker.client_count() == before + 1U;
}

template <typename Broker>
bool send_and_pump(harness<Broker>& h, raw_peer const& peer, bytes const& data)
{
    bool sent = peer.send(data);
    h.timing.pump();
    return sent;
}

// Opens a connection, sends CONNECT and reads the CONNACK into `reply`.
template <typename Broker>
bool login(harness<Broker>& h, raw_peer& peer, bytes const& connect_packet, bytes& reply)
{
    return open_connection(h, peer) && send_and_pump(h, peer, connect_packet) && peer.read_packet(reply);
}

void expect_next(raw_peer const& peer, bytes const& expected)
{
    bytes reply;
    ASSERT_TRUE(peer.read_packet(reply));
    EXPECT_EQ(reply, expected);
}

// CONNACK carrying a non-zero return or reason code without session present.
bool is_refusal(bytes const& reply)
{
    return reply.size() >= 4U && reply[0] == 0x20U && reply[2] == 0x00U && reply[3] != 0x00U;
}

enum class outcome { accepted, refused, closed };

struct wire_case
{
    char const* label;
    bytes wire;
    outcome expected;
    bytes connack;
};

// Sends `item.wire` on a new connection of `h` (after `preamble` when non-empty) and checks the reaction.
void expect_reaction_on(harness<broker_type>& h, bytes const& preamble, wire_case const& item)
{
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(open_connection(h, peer)) << item.label;
    if (!preamble.empty())
    {
        ASSERT_TRUE(send_and_pump(h, peer, preamble)) << item.label;
        ASSERT_TRUE(peer.read_packet(reply)) << item.label;
    }
    ASSERT_TRUE(send_and_pump(h, peer, item.wire)) << item.label;
    if (item.expected == outcome::closed)
    {
        EXPECT_FALSE(peer.read_packet(reply)) << item.label;
    }
    else
    {
        ASSERT_TRUE(peer.read_packet(reply)) << item.label;
    }
    if (item.expected == outcome::accepted)
    {
        EXPECT_EQ(reply, item.connack) << item.label;
        EXPECT_EQ(h.broker.client_count(), 1U) << item.label;
        return;
    }
    if (item.expected == outcome::refused) { EXPECT_TRUE(is_refusal(reply)) << item.label; }
    EXPECT_TRUE(peer.wait_closed()) << item.label;
    EXPECT_EQ(h.broker.client_count(), 0U) << item.label;
}

// Sends `item.wire` on a fresh broker (after `preamble` when non-empty) and checks the reaction.
void expect_reaction(bytes const& preamble, wire_case const& item)
{
    harness<broker_type> h;
    expect_reaction_on(h, preamble, item);
}

// CONNECT whose v5 property block declares five bytes but carries none.
bytes connect_with_bad_properties()
{
    bytes body = text("MQTT");
    body.push_back(5U);
    body.push_back(0x02U);
    put_u16(body, 0U);
    body.push_back(5U);
    append(body, text("a"));
    return packet(0x10U, body);
}

}  // namespace

TEST(MqttBroker, FixedCapacityAndAuthenticationConfiguration)
{
    basic_broker<2U, 128U, 256U, 8U, 4U, 2U, 48U, 64U> broker;
    EXPECT_EQ(broker.client_count(), 0U);
    EXPECT_EQ(broker.subscription_count(), 0U);
    EXPECT_EQ(broker.retained_count(), 0U);
    EXPECT_EQ(broker.poll(0U), castle::status::not_configured);
    EXPECT_EQ(broker.set_authentication("node", "secret"), castle::status::ok);
    EXPECT_EQ(broker.set_authentication("", ""), castle::status::ok);
    EXPECT_EQ(broker.set_authentication("username-that-is-deliberately-longer-than-forty-eight-bytes-to-test-capacity", "x"), castle::status::out_of_range);
}

TEST(MqttBroker, OpensAnEphemeralLoopbackListenerAndPolls)
{
    basic_broker<1U, 128U, 256U, 4U, 2U, 2U, 48U, 64U> broker;
    ASSERT_EQ(broker.open(castle::net::endpoint::loopback_ipv4(0U)), castle::status::ok);
    castle::net::endpoint bound;
    ASSERT_EQ(broker.transport().socket().local_endpoint(bound), castle::status::ok);
    EXPECT_EQ(bound.family(), castle::net::address_family::ipv4);
    EXPECT_NE(bound.port(), 0U);
    EXPECT_EQ(broker.poll(1U), castle::status::ok);
    EXPECT_EQ(broker.close(), castle::status::ok);
    EXPECT_FALSE(broker.transport().is_open());
}

TEST(MqttBroker, SetAuthenticationRejectsOversizedAndInvalidText)
{
    broker_type broker;
    std::string forty_seven(47U, 'p');
    std::string forty_eight(48U, 'p');
    EXPECT_EQ(broker.set_authentication("node", forty_seven.c_str()), castle::status::ok);
    EXPECT_EQ(broker.set_authentication("node", forty_eight.c_str()), castle::status::out_of_range);
    EXPECT_EQ(broker.set_authentication("\xC3\x28", "secret"), castle::status::out_of_range);
    EXPECT_EQ(broker.set_authentication("node", "\xFF"), castle::status::out_of_range);
}

TEST(MqttBroker, AuthenticationAdmitsOnlyMatchingCredentials)
{
    harness<broker_type> h;
    ASSERT_EQ(h.broker.set_authentication("node", "secret"), castle::status::ok);
    std::vector<wire_case> const cases{
        {"anonymous", connect4("a"), outcome::refused, {}},
        {"user name without password", connect4("a", 0x82U, text("node")), outcome::refused, {}},
        {"other user name", connect4("a", 0xC2U, concat_tail(text("other"), binary(raw("secret")))), outcome::refused, {}},
        {"short password", connect4("a", 0xC2U, concat_tail(text("node"), binary(raw("secre")))), outcome::refused, {}},
        {"long password", connect4("a", 0xC2U, concat_tail(text("node"), binary(raw("secretX")))), outcome::refused, {}},
        {"long user name", connect4("a", 0xC2U, concat_tail(text("nodex"), binary(raw("secret")))), outcome::refused, {}},
        {"same length wrong password", connect4("a", 0xC2U, concat_tail(text("node"), binary(raw("secrex")))), outcome::refused, {}},
    };
    for (wire_case const& item : cases) { expect_reaction_on(h, bytes{}, item); }
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect4("a", 0xC2U, concat_tail(text("node"), binary(raw("secret")))), reply));
    EXPECT_EQ(reply, kConnackV4);
    EXPECT_EQ(h.broker.client_count(), 1U);
}

TEST(MqttBroker, EmptyUserNameDisablesAuthentication)
{
    harness<broker_type> h;
    ASSERT_EQ(h.broker.set_authentication("node", "secret"), castle::status::ok);
    ASSERT_EQ(h.broker.set_authentication("", ""), castle::status::ok);
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect4("anon"), reply));
    EXPECT_EQ(reply, kConnackV4);
}

TEST(MqttBroker, ConnectNegotiatesProtocolNameAndLevel)
{
    std::vector<wire_case> const cases{
        {"MQIsdp level 3", connect("MQIsdp", 3U, 0x02U, "a"), outcome::accepted, kConnackV4},
        {"MQTT level 5", connect5("a"), outcome::accepted, kConnackV5},
        {"MQTT level 3", connect("MQTT", 3U, 0x02U, "a"), outcome::refused, {}},
        {"MQIsdp level 4", connect("MQIsdp", 4U, 0x02U, "a"), outcome::refused, {}},
        {"MQTT level 6", connect("MQTT", 6U, 0x02U, "a"), outcome::refused, {}},
        {"MQTT level 0", connect("MQTT", 0U, 0x02U, "a"), outcome::refused, {}},
        {"unknown name", connect("MQXX", 4U, 0x02U, "a"), outcome::refused, {}},
        {"name beyond buffer", connect("MQTT-LONG", 4U, 0x02U, "a"), outcome::closed, {}},
    };
    for (wire_case const& item : cases) { expect_reaction(bytes{}, item); }
}

TEST(MqttBroker, ConnectRejectsInvalidFlagCombinations)
{
    std::vector<wire_case> const cases{
        {"reserved bit", connect4("a", 0x03U), outcome::refused, {}},
        {"will QoS without will", connect4("a", 0x0AU), outcome::refused, {}},
        {"will retain without will", connect4("a", 0x22U), outcome::refused, {}},
        {"will QoS 3", connect4("a", 0x1EU), outcome::refused, {}},
        {"password without user name", connect4("a", 0x42U), outcome::refused, {}},
    };
    for (wire_case const& item : cases) { expect_reaction(bytes{}, item); }
}

TEST(MqttBroker, TruncatedConnectPacketsCloseWithoutConnack)
{
    bytes const body = connect_body("MQTT", 4U, 0x02U, "a");
    std::vector<size_t> const lengths{0U, 1U, 6U, 7U, 8U, 9U};
    for (size_t length : lengths)
    {
        expect_reaction(bytes{}, wire_case{"truncated", packet(0x10U, prefix(body, length)), outcome::closed, {}});
    }
    expect_reaction(bytes{}, wire_case{"missing client identifier", packet(0x10U, prefix(body, 10U)), outcome::refused, {}});
    bytes trailing = body;
    trailing.push_back(0x00U);
    expect_reaction(bytes{}, wire_case{"trailing byte", packet(0x10U, trailing), outcome::closed, {}});
}

TEST(MqttBroker, ConnectValidatesIdentifierWillAndCredentialFields)
{
    std::string forty_eight(48U, 'c');
    std::vector<wire_case> const cases{
        {"empty client identifier without clean session", connect4("", 0x00U), outcome::refused, {}},
        {"client identifier at capacity", connect4(forty_eight.c_str()), outcome::refused, {}},
        {"will topic with wildcard", connect4("a", 0x0EU, will_fields("will/#", "x")), outcome::closed, {}},
        {"empty will topic", connect4("a", 0x0EU, will_fields("", "x")), outcome::closed, {}},
        {"will payload beyond capacity", connect4("a", 0x0EU, concat_tail(text("w"), binary(filled(65U)))), outcome::closed, {}},
        {"truncated will topic", connect4("a", 0x0EU, bytes{0x00U, 0x09U, 0x61U}), outcome::closed, {}},
        {"user name at capacity", connect4("a", 0x82U, text(forty_eight.c_str())), outcome::closed, {}},
        {"password beyond capacity", connect4("a", 0xC2U, concat_tail(text("u"), binary(filled(49U)))), outcome::closed, {}},
        {"user name longer than body", connect4("a", 0x82U, bytes{0x00U, 0x09U, 0x61U}), outcome::closed, {}},
        {"malformed connect properties", connect_with_bad_properties(), outcome::closed, {}},
        {"malformed will properties", connect5("a", 0x06U, bytes{0x05U}), outcome::closed, {}},
    };
    for (wire_case const& item : cases) { expect_reaction(bytes{}, item); }
}

TEST(MqttBroker, EmptyClientIdentifierIsAssignedWithCleanSession)
{
    bytes assigned_v5{0x20U, 0x0EU, 0x00U, 0x00U, 0x0BU, 0x12U};
    bytes assigned_id = text("castle-0");
    assigned_v5.insert(assigned_v5.end(), assigned_id.begin(), assigned_id.end());
    expect_reaction(bytes{}, wire_case{"empty client identifier v3.1.1", connect4(""), outcome::accepted, kConnackV4});
    expect_reaction(bytes{}, wire_case{"empty client identifier v5.0", connect5(""), outcome::accepted, assigned_v5});

    harness<broker_type> h;
    raw_peer first;
    raw_peer second;
    bytes reply;
    ASSERT_TRUE(login(h, first, connect4(""), reply));
    EXPECT_EQ(reply, kConnackV4);
    ASSERT_TRUE(login(h, second, connect4(""), reply));
    EXPECT_EQ(reply, kConnackV4);
    EXPECT_EQ(h.broker.client_count(), 2U);
}

TEST(MqttBroker, RejectsTrafficBeforeConnectAndRepeatedConnect)
{
    std::vector<wire_case> const cases{
        {"ping before connect", kPingreq, outcome::closed, {}},
        {"publish before connect", publish("a", 0, false, 0U, raw("x")), outcome::closed, {}},
        {"subscribe before connect", subscribe(1U, filter("a")), outcome::closed, {}},
    };
    for (wire_case const& item : cases) { expect_reaction(bytes{}, item); }
    expect_reaction(connect4("a"), wire_case{"second connect", connect4("b"), outcome::closed, {}});
}

TEST(MqttBroker, PingAndDisconnectFollowTheProtocol)
{
    harness<broker_type> h;
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect4("p"), reply));
    ASSERT_TRUE(send_and_pump(h, peer, kPingreq));
    expect_next(peer, kPingresp);
    ASSERT_TRUE(send_and_pump(h, peer, kDisconnect));
    EXPECT_TRUE(peer.wait_closed());
    EXPECT_EQ(h.broker.client_count(), 0U);
}

TEST(MqttBroker, DisconnectAndPingPayloadsAreValidatedByVersion)
{
    bytes reason_and_properties{0x00U};
    append(reason_and_properties, block(session_expiry(1U)));
    std::vector<wire_case> const version4{
        {"ping with payload", packet(0xC0U, bytes{0x00U}), outcome::closed, {}},
        {"disconnect with payload", packet(0xE0U, bytes{0x00U}), outcome::closed, {}},
    };
    for (wire_case const& item : version4) { expect_reaction(connect4("d"), item); }
    std::vector<wire_case> const version5{
        {"disconnect with reason", packet(0xE0U, bytes{0x04U}), outcome::closed, {}},
        {"disconnect with properties", packet(0xE0U, reason_and_properties), outcome::closed, {}},
    };
    for (wire_case const& item : version5) { expect_reaction(connect5("d"), item); }
}

TEST(MqttBroker, ReceiveBufferReassemblesSplitAndCoalescedPackets)
{
    harness<broker_type> h;
    raw_peer peer;
    bytes reply;
    bytes const connect_packet = connect4("r");
    ASSERT_TRUE(open_connection(h, peer));
    ASSERT_TRUE(send_and_pump(h, peer, prefix(connect_packet, 5U)));
    EXPECT_TRUE(peer.silent());
    ASSERT_TRUE(send_and_pump(h, peer, bytes(connect_packet.begin() + 5, connect_packet.end())));
    ASSERT_TRUE(peer.read_packet(reply));
    EXPECT_EQ(reply, kConnackV4);

    bytes burst = kPingreq;
    append(burst, kPingreq);
    burst.push_back(0xC0U);
    ASSERT_TRUE(send_and_pump(h, peer, burst));
    expect_next(peer, kPingresp);
    expect_next(peer, kPingresp);
    EXPECT_TRUE(peer.silent());
    ASSERT_TRUE(send_and_pump(h, peer, bytes{0x00U}));
    expect_next(peer, kPingresp);

    raw_peer closer;
    ASSERT_TRUE(login(h, closer, connect4("c"), reply));
    bytes leftover;
    ASSERT_TRUE(send_and_pump(h, closer, bytes{0xE0U, 0x00U, 0xC0U, 0x00U}));
    EXPECT_TRUE(closer.wait_closed(leftover));
    EXPECT_TRUE(leftover.empty());
}

TEST(MqttBroker, MalformedFramesCloseTheConnection)
{
    std::vector<wire_case> const cases{
        {"reserved packet type", bytes{0x00U, 0x00U}, outcome::closed, {}},
        {"PINGREQ with flags", bytes{0xC1U, 0x00U}, outcome::closed, {}},
        {"SUBSCRIBE with wrong flags", bytes{0x80U, 0x02U, 0x00U, 0x01U}, outcome::closed, {}},
        {"CONNACK from client", kConnackV4, outcome::closed, {}},
        {"SUBACK from client", suback(1U, bytes{0x00U}), outcome::closed, {}},
        {"PINGRESP from client", kPingresp, outcome::closed, {}},
        {"AUTH packet", bytes{0xF0U, 0x00U}, outcome::closed, {}},
        {"PUBLISH QoS 3", bytes{0x36U, 0x00U}, outcome::closed, {}},
        {"remaining length above capacity", bytes{0x30U, 0xFFU, 0x03U}, outcome::closed, {}},
        {"overlong remaining length", bytes{0x30U, 0x80U, 0x80U, 0x80U, 0x80U, 0x01U}, outcome::closed, {}},
    };
    for (wire_case const& item : cases) { expect_reaction(connect4("m"), item); }

    harness<broker_type> h;
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect4("m"), reply));
    ASSERT_TRUE(send_and_pump(h, peer, bytes{0x30U, 0x80U}));
    EXPECT_EQ(h.broker.client_count(), 1U);
    EXPECT_TRUE(peer.silent());
}

TEST(MqttBroker, QosZeroFanoutAndRetainedLifecycle)
{
    harness<wide_type> h;
    raw_peer subscriber;
    raw_peer publisher;
    raw_peer observer;
    bytes reply;
    ASSERT_TRUE(login(h, subscriber, connect4("s"), reply));
    ASSERT_TRUE(send_and_pump(h, subscriber, subscribe(1U, filter("sensor/#"))));
    expect_next(subscriber, suback(1U, bytes{0x00U}));
    ASSERT_TRUE(login(h, publisher, connect4("p"), reply));

    ASSERT_TRUE(send_and_pump(h, publisher, publish("sensor/a", 0, false, 0U, raw("1"))));
    expect_next(subscriber, publish("sensor/a", 0, false, 0U, raw("1")));
    ASSERT_TRUE(send_and_pump(h, publisher, publish("sensor/r", 0, true, 0U, raw("keep"))));
    expect_next(subscriber, publish("sensor/r", 0, false, 0U, raw("keep")));
    EXPECT_EQ(h.broker.retained_count(), 1U);

    ASSERT_TRUE(login(h, observer, connect5("r5"), reply));
    EXPECT_EQ(reply, kConnackV5);
    ASSERT_TRUE(send_and_pump(h, observer, subscribe(2U, filter("sensor/#", 1U), true)));
    expect_next(observer, suback(2U, bytes{0x01U}, true));
    expect_next(observer, publish("sensor/r", 0, true, 0U, raw("keep"), true));

    ASSERT_TRUE(send_and_pump(h, publisher, publish("sensor/r", 0, true, 0U, bytes{})));
    EXPECT_EQ(h.broker.retained_count(), 0U);
    expect_next(subscriber, publish("sensor/r", 0, false, 0U, bytes{}));
    expect_next(observer, publish("sensor/r", 0, false, 0U, bytes{}, true));

    ASSERT_TRUE(send_and_pump(h, publisher, publish("sensor/none", 0, true, 0U, bytes{})));
    EXPECT_EQ(h.broker.retained_count(), 0U);
    expect_next(subscriber, publish("sensor/none", 0, false, 0U, bytes{}));
    expect_next(observer, publish("sensor/none", 0, false, 0U, bytes{}, true));
}

TEST(MqttBroker, RetainedStoreIsBoundedAndUpdatesInPlace)
{
    harness<broker_type> h;
    raw_peer publisher;
    raw_peer subscriber;
    bytes reply;
    ASSERT_TRUE(login(h, publisher, connect4("p"), reply));
    for (char topic = '1'; topic <= '5'; ++topic)
    {
        char const name[] = {'r', '/', topic, '\0'};
        ASSERT_TRUE(send_and_pump(h, publisher, publish(name, 0, true, 0U, raw("v"))));
    }
    EXPECT_EQ(h.broker.retained_count(), 4U);
    ASSERT_TRUE(send_and_pump(h, publisher, publish("r/1", 0, true, 0U, raw("new"))));
    EXPECT_EQ(h.broker.retained_count(), 4U);
    ASSERT_TRUE(send_and_pump(h, publisher, publish("r/9", 0, true, 0U, bytes{})));
    EXPECT_EQ(h.broker.retained_count(), 4U);

    ASSERT_TRUE(login(h, subscriber, connect4("s"), reply));
    ASSERT_TRUE(send_and_pump(h, subscriber, subscribe(1U, filter("r/#"))));
    expect_next(subscriber, suback(1U, bytes{0x00U}));
    expect_next(subscriber, publish("r/1", 0, true, 0U, raw("new")));
    expect_next(subscriber, publish("r/2", 0, true, 0U, raw("v")));
    expect_next(subscriber, publish("r/3", 0, true, 0U, raw("v")));
    expect_next(subscriber, publish("r/4", 0, true, 0U, raw("v")));
    EXPECT_TRUE(subscriber.silent());
}

TEST(MqttBroker, SubscribeDeliversRetainedMessagesAtTheLowerQos)
{
    harness<wide_type> h;
    raw_peer publisher;
    raw_peer first;
    raw_peer second;
    bytes reply;
    ASSERT_TRUE(login(h, publisher, connect4("p"), reply));
    ASSERT_TRUE(send_and_pump(h, publisher, publish("r/2", 2, true, 3U, raw("two"))));
    expect_next(publisher, ack(0x50U, 3U));
    ASSERT_TRUE(send_and_pump(h, publisher, ack(0x62U, 3U)));
    expect_next(publisher, ack(0x70U, 3U));
    ASSERT_TRUE(send_and_pump(h, publisher, publish("r/0", 0, true, 0U, raw("zero"))));

    ASSERT_TRUE(login(h, first, connect4("s"), reply));
    ASSERT_TRUE(send_and_pump(h, first, subscribe(9U, filter("r/#", 1U))));
    expect_next(first, suback(9U, bytes{0x01U}));
    expect_next(first, publish("r/2", 1, true, 1U, raw("two")));
    expect_next(first, publish("r/0", 0, true, 0U, raw("zero")));

    ASSERT_TRUE(login(h, second, connect4("t"), reply));
    ASSERT_TRUE(send_and_pump(h, second, subscribe(4U, filter("r/2", 2U))));
    expect_next(second, suback(4U, bytes{0x02U}));
    expect_next(second, publish("r/2", 2, true, 1U, raw("two")));
}

TEST(MqttBroker, InboundQosOneAndTwoFlowsAcknowledgeAndDeliverOnce)
{
    harness<wide_type> h;
    raw_peer subscriber;
    raw_peer publisher;
    bytes reply;
    ASSERT_TRUE(login(h, subscriber, connect4("s"), reply));
    ASSERT_TRUE(send_and_pump(h, subscriber, subscribe(1U, filter("t/#"))));
    expect_next(subscriber, suback(1U, bytes{0x00U}));
    ASSERT_TRUE(login(h, publisher, connect4("p"), reply));

    ASSERT_TRUE(send_and_pump(h, publisher, publish("t/1", 1, false, 7U, raw("a"))));
    expect_next(publisher, ack(0x40U, 7U));
    expect_next(subscriber, publish("t/1", 0, false, 0U, raw("a")));

    ASSERT_TRUE(send_and_pump(h, publisher, publish("t/2", 2, false, 8U, raw("b"))));
    expect_next(publisher, ack(0x50U, 8U));
    EXPECT_TRUE(subscriber.silent());
    ASSERT_TRUE(send_and_pump(h, publisher, publish("t/2", 2, false, 8U, raw("b"))));
    expect_next(publisher, ack(0x50U, 8U));
    EXPECT_TRUE(subscriber.silent());
    ASSERT_TRUE(send_and_pump(h, publisher, ack(0x62U, 8U)));
    expect_next(publisher, ack(0x70U, 8U));
    expect_next(subscriber, publish("t/2", 0, false, 0U, raw("b")));
    EXPECT_TRUE(subscriber.silent());

    ASSERT_TRUE(send_and_pump(h, publisher, ack(0x62U, 99U)));
    expect_next(publisher, ack(0x70U, 99U));
    ASSERT_TRUE(send_and_pump(h, publisher, publish("t/3", 0, false, 0U, raw("c"))));
    expect_next(subscriber, publish("t/3", 0, false, 0U, raw("c")));
}

TEST(MqttBroker, InboundQosTwoStoreRefusesOverflowAndClosesPeer)
{
    harness<wide_type> h;
    raw_peer publisher;
    bytes reply;
    ASSERT_TRUE(login(h, publisher, connect4("p"), reply));
    bytes burst = publish("x/y", 2, false, 21U, raw("x"));
    append(burst, publish("x/y", 2, false, 22U, raw("x")));
    append(burst, publish("x/y", 2, false, 23U, raw("x")));
    ASSERT_TRUE(send_and_pump(h, publisher, burst));
    expect_next(publisher, ack(0x50U, 21U));
    expect_next(publisher, ack(0x50U, 22U));
    EXPECT_TRUE(publisher.wait_closed());
    EXPECT_EQ(h.broker.client_count(), 0U);
}

TEST(MqttBroker, InvalidPublishesCloseTheConnection)
{
    std::string forty_eight(48U, 't');
    bytes missing_identifier = packet(0x32U, text("a"));
    bytes bad_properties = text("a");
    bad_properties.push_back(0x05U);
    std::vector<wire_case> const cases{
        {"wildcard topic", publish("a/+/b", 0, false, 0U, raw("x"), true), outcome::closed, {}},
        {"empty topic", publish("", 0, false, 0U, raw("x"), true), outcome::closed, {}},
        {"topic at capacity", publish(forty_eight.c_str(), 0, false, 0U, raw("x"), true), outcome::closed, {}},
        {"invalid UTF-8 topic", publish("\xC3\x28", 0, false, 0U, raw("x"), true), outcome::closed, {}},
        {"QoS 1 identifier zero", publish("a", 1, false, 0U, raw("x"), true), outcome::closed, {}},
        {"QoS 1 without identifier", missing_identifier, outcome::closed, {}},
        {"payload beyond capacity", publish("a", 0, false, 0U, filled(65U), true), outcome::closed, {}},
        {"malformed properties", packet(0x30U, bad_properties), outcome::closed, {}},
    };
    for (wire_case const& item : cases) { expect_reaction(connect5("v"), item); }
}

TEST(MqttBroker, Version5InboundPublishAcknowledgementsAndPubrel)
{
    harness<broker_type> h;
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect5("v"), reply));
    EXPECT_EQ(reply, kConnackV5);
    ASSERT_TRUE(send_and_pump(h, peer, publish("t", 1, false, 5U, raw("p"), true, bytes{0x01U, 0x01U})));
    expect_next(peer, ack(0x40U, 5U));
    ASSERT_TRUE(send_and_pump(h, peer, publish("t", 2, false, 6U, raw("q"), true)));
    expect_next(peer, ack(0x50U, 6U));
    ASSERT_TRUE(send_and_pump(h, peer, ack(0x62U, 6U, bytes{0x00U, 0x00U})));
    expect_next(peer, ack(0x70U, 6U));
    ASSERT_TRUE(send_and_pump(h, peer, ack(0x62U, 9U, bytes{0x00U})));
    expect_next(peer, ack(0x70U, 9U));
    EXPECT_TRUE(peer.silent());
}

TEST(MqttBroker, SubscribeRejectsMalformedRequests)
{
    bytes nine;
    for (char name = '0'; name <= '8'; ++name)
    {
        char const topic[] = {'f', name, '\0'};
        append(nine, filter(topic));
    }
    std::vector<wire_case> const version4{
        {"missing identifier", packet(0x82U, bytes{}), outcome::closed, {}},
        {"zero identifier", subscribe(0U, filter("a")), outcome::closed, {}},
        {"no filters", subscribe(1U, bytes{}), outcome::closed, {}},
        {"filter without options", subscribe(1U, text("a")), outcome::closed, {}},
        {"multi-level wildcard inside filter", subscribe(1U, filter("a/#/b")), outcome::closed, {}},
        {"empty filter", subscribe(1U, filter("")), outcome::closed, {}},
        {"single-level wildcard inside level", subscribe(1U, filter("a+/b")), outcome::closed, {}},
        {"requested QoS 3", subscribe(1U, filter("a", 0x03U)), outcome::closed, {}},
        {"reserved option bits", subscribe(1U, filter("a", 0x04U)), outcome::closed, {}},
        {"more filters than capacity", subscribe(1U, nine), outcome::closed, {}},
        {"truncated filter", packet(0x82U, concat_tail(bytes{0x00U, 0x01U}, bytes{0x00U, 0x05U, 0x61U})), outcome::closed, {}},
    };
    for (wire_case const& item : version4) { expect_reaction(connect4("s"), item); }
    std::vector<wire_case> const version5{
        {"reserved option bits 6-7", subscribe(1U, filter("a", 0xC0U), true), outcome::closed, {}},
        {"requested QoS 3 in version 5", subscribe(1U, filter("a", 0x03U), true), outcome::closed, {}},
        {"retain handling 3", subscribe(1U, filter("a", 0x30U), true), outcome::closed, {}},
        {"malformed properties", packet(0x82U, bytes{0x00U, 0x01U, 0x05U}), outcome::closed, {}},
    };
    for (wire_case const& item : version5) { expect_reaction(connect5("s"), item); }
}

TEST(MqttBroker, SubscribeReportsCapacityRefusalsPerFilter)
{
    harness<broker_type> h;
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect4("c"), reply));
    bytes filters;
    for (char name = '1'; name <= '8'; ++name)
    {
        char const topic[] = {'c', '/', name, '\0'};
        append(filters, filter(topic));
    }
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(1U, filters)));
    expect_next(peer, suback(1U, zeros(8U)));
    EXPECT_EQ(h.broker.subscription_count(), 8U);
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(2U, filter("c/9"))));
    expect_next(peer, suback(2U, bytes{0x80U}));
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(3U, filter("c/1", 0x01U))));
    expect_next(peer, suback(3U, bytes{0x01U}));
    EXPECT_EQ(h.broker.subscription_count(), 8U);
    EXPECT_TRUE(peer.silent());

    harness<broker_type> v5h;
    raw_peer v5peer;
    ASSERT_TRUE(login(v5h, v5peer, connect5("d"), reply));
    bytes v5filters;
    for (char name = '1'; name <= '8'; ++name)
    {
        char const topic[] = {'d', '/', name, '\0'};
        append(v5filters, filter(topic));
    }
    ASSERT_TRUE(send_and_pump(v5h, v5peer, subscribe(1U, v5filters, true)));
    expect_next(v5peer, suback(1U, zeros(8U), true));
    ASSERT_TRUE(send_and_pump(v5h, v5peer, subscribe(2U, filter("d/9"), true)));
    expect_next(v5peer, suback(2U, bytes{0x97U}, true));
}

TEST(MqttBroker, OverlappingSubscriptionsDeliverAtBestQos)
{
    harness<broker_type> h;
    raw_peer subscriber;
    raw_peer publisher;
    bytes reply;
    ASSERT_TRUE(login(h, subscriber, connect4("s"), reply));
    bytes filters = filter("o/#");
    append(filters, filter("o/+", 0x01U));
    ASSERT_TRUE(send_and_pump(h, subscriber, subscribe(1U, filters)));
    expect_next(subscriber, suback(1U, bytes{0x00U, 0x01U}));
    ASSERT_TRUE(login(h, publisher, connect4("p"), reply));

    ASSERT_TRUE(send_and_pump(h, publisher, publish("o/x", 1, false, 5U, raw("v"))));
    expect_next(publisher, ack(0x40U, 5U));
    expect_next(subscriber, publish("o/x", 1, false, 1U, raw("v")));
    ASSERT_TRUE(send_and_pump(h, publisher, publish("o/y", 0, false, 0U, raw("w"))));
    expect_next(subscriber, publish("o/y", 0, false, 0U, raw("w")));
    ASSERT_TRUE(send_and_pump(h, publisher, publish("other/z", 0, false, 0U, raw("n"))));
    EXPECT_TRUE(subscriber.silent());

    ASSERT_TRUE(send_and_pump(h, subscriber, subscribe(2U, filter("o/+"))));
    expect_next(subscriber, suback(2U, bytes{0x00U}));
    EXPECT_EQ(h.broker.subscription_count(), 2U);
    ASSERT_TRUE(send_and_pump(h, publisher, publish("o/z", 1, false, 6U, raw("u"))));
    expect_next(publisher, ack(0x40U, 6U));
    expect_next(subscriber, publish("o/z", 0, false, 0U, raw("u")));
}

TEST(MqttBroker, UnsubscribeRemovesFiltersAndReportsResults)
{
    harness<broker_type> h;
    raw_peer version4;
    raw_peer version5;
    bytes reply;
    ASSERT_TRUE(login(h, version4, connect4("u"), reply));
    ASSERT_TRUE(send_and_pump(h, version4, subscribe(1U, filter("u/a"))));
    expect_next(version4, suback(1U, bytes{0x00U}));
    ASSERT_TRUE(send_and_pump(h, version4, unsubscribe(4U, concat_tail(text("u/a"), text("u/zz")))));
    expect_next(version4, unsuback(4U));
    EXPECT_EQ(h.broker.subscription_count(), 0U);
    ASSERT_TRUE(send_and_pump(h, version4, subscribe(5U, filter("u/b"))));
    expect_next(version4, suback(5U, bytes{0x00U}));

    ASSERT_TRUE(login(h, version5, connect5("w"), reply));
    ASSERT_TRUE(send_and_pump(h, version5, subscribe(2U, filter("u/b"), true)));
    expect_next(version5, suback(2U, bytes{0x00U}, true));
    EXPECT_EQ(h.broker.subscription_count(), 2U);
    ASSERT_TRUE(send_and_pump(h, version5, unsubscribe(3U, concat_tail(text("u/b"), text("u/none")), true)));
    expect_next(version5, unsuback(3U, bytes{0x00U, 0x11U}, true));
    EXPECT_EQ(h.broker.subscription_count(), 1U);
}

TEST(MqttBroker, UnsubscribeRejectsMalformedRequests)
{
    bytes nine;
    for (char name = '0'; name <= '8'; ++name)
    {
        char const topic[] = {'f', name, '\0'};
        append(nine, text(topic));
    }
    std::vector<wire_case> const version4{
        {"missing identifier", packet(0xA2U, bytes{}), outcome::closed, {}},
        {"zero identifier", unsubscribe(0U, text("a")), outcome::closed, {}},
        {"no filters", unsubscribe(1U, bytes{}), outcome::closed, {}},
        {"invalid filter", unsubscribe(1U, text("a/#/b")), outcome::closed, {}},
        {"more filters than capacity", unsubscribe(1U, nine), outcome::closed, {}},
        {"truncated filter", packet(0xA2U, bytes{0x00U, 0x01U, 0x00U, 0x05U, 0x61U}), outcome::closed, {}},
    };
    for (wire_case const& item : version4) { expect_reaction(connect4("x"), item); }
    std::vector<wire_case> const version5{
        {"malformed properties", packet(0xA2U, bytes{0x00U, 0x01U, 0x05U}), outcome::closed, {}},
        {"no filters", unsubscribe(1U, bytes{}, true), outcome::closed, {}},
    };
    for (wire_case const& item : version5) { expect_reaction(connect5("x"), item); }
}

TEST(MqttBroker, PersistentSessionsKeepSubscriptionsUntilCleanStart)
{
    harness<wide_type> h;
    raw_peer first;
    raw_peer second;
    raw_peer publisher;
    raw_peer third;
    bytes reply;
    ASSERT_TRUE(login(h, first, connect4("keep", 0x00U), reply));
    EXPECT_EQ(reply, kConnackV4);
    ASSERT_TRUE(send_and_pump(h, first, subscribe(1U, filter("k/#"))));
    expect_next(first, suback(1U, bytes{0x00U}));
    first.close();
    h.timing.pump();
    EXPECT_EQ(h.broker.client_count(), 0U);
    EXPECT_EQ(h.broker.subscription_count(), 1U);

    ASSERT_TRUE(login(h, second, connect4("keep", 0x00U), reply));
    EXPECT_EQ(reply, kConnackV4Session);
    ASSERT_TRUE(login(h, publisher, connect4("p"), reply));
    ASSERT_TRUE(send_and_pump(h, publisher, publish("k/x", 0, false, 0U, raw("hi"))));
    expect_next(second, publish("k/x", 0, false, 0U, raw("hi")));
    ASSERT_TRUE(send_and_pump(h, second, kDisconnect));
    EXPECT_TRUE(second.wait_closed());
    EXPECT_EQ(h.broker.subscription_count(), 1U);

    ASSERT_TRUE(login(h, third, connect4("keep", 0x02U), reply));
    EXPECT_EQ(reply, kConnackV4);
    EXPECT_EQ(h.broker.subscription_count(), 0U);
}

TEST(MqttBroker, PersistentSessionTableRefusesNewIdsWhenFull)
{
    harness<wide_type> h;
    raw_peer peer;
    bytes reply;
    for (char const* id : {"s1", "s2", "s3", "s4"})
    {
        ASSERT_TRUE(login(h, peer, connect4(id, 0x00U), reply));
        EXPECT_EQ(reply, kConnackV4);
        ASSERT_TRUE(send_and_pump(h, peer, kDisconnect));
        peer.close();
    }
    EXPECT_EQ(h.broker.client_count(), 0U);

    ASSERT_TRUE(login(h, peer, connect4("s5", 0x00U), reply));
    EXPECT_TRUE(is_refusal(reply));
    EXPECT_TRUE(peer.wait_closed());
    ASSERT_TRUE(login(h, peer, connect4("s5", 0x02U), reply));
    EXPECT_TRUE(is_refusal(reply));
    EXPECT_TRUE(peer.wait_closed());

    ASSERT_TRUE(login(h, peer, connect4("s2", 0x00U), reply));
    EXPECT_EQ(reply, kConnackV4Session);
    ASSERT_TRUE(send_and_pump(h, peer, kDisconnect));
    peer.close();

    ASSERT_TRUE(login(h, peer, connect4("s3", 0x02U), reply));
    EXPECT_EQ(reply, kConnackV4);
    ASSERT_TRUE(send_and_pump(h, peer, kDisconnect));
    peer.close();

    ASSERT_TRUE(login(h, peer, connect4("s5", 0x00U), reply));
    EXPECT_EQ(reply, kConnackV4);
}

TEST(MqttBroker, TakeoverClosesThePreviousConnectionWithoutPublishingItsWill)
{
    harness<wide_type> h;
    raw_peer idle;
    raw_peer previous;
    raw_peer subscriber;
    raw_peer taker;
    bytes reply;
    ASSERT_TRUE(open_connection(h, idle));
    ASSERT_TRUE(login(h, previous, connect4("dup", 0x04U, will_fields("dup/will", "gone")), reply));
    EXPECT_EQ(reply, kConnackV4);
    ASSERT_TRUE(login(h, subscriber, connect4("s"), reply));
    ASSERT_TRUE(send_and_pump(h, subscriber, subscribe(1U, filter("dup/#"))));
    expect_next(subscriber, suback(1U, bytes{0x00U}));

    ASSERT_TRUE(login(h, taker, connect4("dup"), reply));
    EXPECT_EQ(reply, kConnackV4);
    bytes leftover;
    EXPECT_TRUE(previous.wait_closed(leftover));
    EXPECT_TRUE(leftover.empty());
    ASSERT_TRUE(send_and_pump(h, subscriber, publish("dup/x", 0, false, 0U, raw("n"))));
    expect_next(subscriber, publish("dup/x", 0, false, 0U, raw("n")));
    EXPECT_EQ(h.broker.client_count(), 3U);
}

TEST(MqttBroker, Version5SessionExpiryKeepsThenPrunesSessionState)
{
    harness<broker_type> h;
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect5("e1", 0x00U, {}, session_expiry(2U)), reply));
    EXPECT_EQ(reply, kConnackV5);
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(1U, filter("e/#"), true)));
    expect_next(peer, suback(1U, bytes{0x00U}, true));
    peer.close();
    h.timing.pump();
    EXPECT_EQ(h.broker.subscription_count(), 1U);

    h.timing.advance(1000U);
    ASSERT_TRUE(login(h, peer, connect5("e1", 0x00U, {}, session_expiry(2U)), reply));
    EXPECT_EQ(reply, kConnackV5Session);
    EXPECT_EQ(h.broker.subscription_count(), 1U);
    ASSERT_TRUE(send_and_pump(h, peer, kDisconnect));
    peer.close();

    h.timing.advance(5000U);
    h.timing.pump();
    EXPECT_EQ(h.broker.subscription_count(), 0U);
    ASSERT_TRUE(login(h, peer, connect5("e1", 0x00U, {}, session_expiry(2U)), reply));
    EXPECT_EQ(reply, kConnackV5);
    peer.close();
    h.timing.pump();

    ASSERT_TRUE(login(h, peer, connect5("e0"), reply));
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(2U, filter("z/#"), true)));
    expect_next(peer, suback(2U, bytes{0x00U}, true));
    peer.close();
    h.timing.pump();
    EXPECT_EQ(h.broker.subscription_count(), 0U);
}

TEST(MqttBroker, WillIsPublishedOnlyForAbnormalClose)
{
    harness<wide_type> h;
    raw_peer subscriber;
    raw_peer victim;
    raw_peer reset_victim;
    raw_peer version5_victim;
    raw_peer graceful;
    bytes reply;
    ASSERT_TRUE(login(h, subscriber, connect4("s"), reply));
    ASSERT_TRUE(send_and_pump(h, subscriber, subscribe(1U, filter("w/#"))));
    expect_next(subscriber, suback(1U, bytes{0x00U}));

    ASSERT_TRUE(login(h, victim, connect4("w1", 0x06U, will_fields("w/one", "bye1")), reply));
    victim.close();
    h.timing.pump();
    expect_next(subscriber, publish("w/one", 0, false, 0U, raw("bye1")));

    ASSERT_TRUE(login(h, reset_victim, connect4("w2", 0x2EU, will_fields("w/two", "bye2")), reply));
    reset_victim.reset();
    h.timing.pump();
    expect_next(subscriber, publish("w/two", 0, false, 0U, raw("bye2")));
    EXPECT_EQ(h.broker.retained_count(), 1U);

    bytes version5_tail = block(bytes{0x01U, 0x01U});
    append(version5_tail, will_fields("w/three", "bye3"));
    ASSERT_TRUE(login(h, version5_victim, connect5("w3", 0x06U, version5_tail), reply));
    EXPECT_EQ(reply, kConnackV5);
    version5_victim.close();
    h.timing.pump();
    expect_next(subscriber, publish("w/three", 0, false, 0U, raw("bye3")));

    ASSERT_TRUE(login(h, graceful, connect4("w4", 0x06U, will_fields("w/four", "bye4")), reply));
    ASSERT_TRUE(send_and_pump(h, graceful, kDisconnect));
    EXPECT_TRUE(graceful.wait_closed());
    EXPECT_TRUE(subscriber.silent());
}

TEST(MqttBroker, KeepAliveTimeoutPublishesWillAndClosesSilentClients)
{
    harness<wide_type> h;
    raw_peer subscriber;
    raw_peer silent_client;
    raw_peer no_will;
    raw_peer persistent;
    bytes reply;
    ASSERT_TRUE(login(h, subscriber, connect4("s"), reply));
    ASSERT_TRUE(send_and_pump(h, subscriber, subscribe(1U, filter("k/#"))));
    expect_next(subscriber, suback(1U, bytes{0x00U}));

    ASSERT_TRUE(login(h, silent_client, connect4("k1", 0x06U, will_fields("k/one", "dead"), 1U), reply));
    ASSERT_TRUE(silent_client.send(kPingreq));
    ASSERT_EQ(h.timing.tick(), castle::status::ok);
    uint32_t last_received = h.timing.now();
    expect_next(silent_client, kPingresp);
    EXPECT_EQ(h.timing.poll_at(last_received + 1500U), castle::status::ok);
    EXPECT_TRUE(silent_client.silent());
    EXPECT_EQ(h.timing.poll_at(last_received + 1501U), castle::status::ok);
    expect_next(subscriber, publish("k/one", 0, false, 0U, raw("dead")));
    EXPECT_TRUE(silent_client.wait_closed());

    ASSERT_TRUE(login(h, no_will, connect4("k3", 0x02U, {}, 1U), reply));
    uint32_t base = h.timing.now();
    EXPECT_EQ(h.timing.poll_at(base + 5000U), castle::status::ok);
    EXPECT_TRUE(no_will.wait_closed());
    EXPECT_TRUE(subscriber.silent());

    ASSERT_TRUE(login(h, persistent, connect4("k2", 0x02U, {}, 0U), reply));
    EXPECT_EQ(h.timing.poll_at(h.timing.now() + 1000000U), castle::status::ok);
    EXPECT_EQ(h.broker.client_count(), 2U);
}

TEST(MqttBroker, ExternallyManagedConnectionsResynchronizeOnPoll)
{
    harness<broker_type> h;
    raw_peer first;
    raw_peer second;
    raw_peer third;
    bytes reply;
    ASSERT_TRUE(login(h, first, connect4("x1"), reply));
    handle_type original = h.broker.transport().client_at(0U);
    EXPECT_EQ(h.broker.transport().close_client(original), castle::status::ok);
    EXPECT_EQ(h.broker.client_count(), 0U);

    ASSERT_TRUE(second.connect_to(h.port));
    handle_type reaccepted;
    ASSERT_EQ(h.broker.transport().accept_client(reaccepted, 100), castle::status::ok);
    EXPECT_EQ(h.timing.tick(), castle::status::ok);
    EXPECT_EQ(h.broker.client_count(), 1U);

    ASSERT_TRUE(third.connect_to(h.port));
    handle_type extra;
    ASSERT_EQ(h.broker.transport().accept_client(extra, 100), castle::status::ok);
    EXPECT_EQ(h.timing.tick(), castle::status::ok);
    EXPECT_EQ(h.broker.client_count(), 2U);
    EXPECT_TRUE(first.wait_closed());
}

TEST(MqttBroker, AcceptWaitsForFreeSlotsAndReportsListenerFaults)
{
    harness<broker_type> h;
    raw_peer first;
    raw_peer second;
    raw_peer waiting;
    ASSERT_TRUE(open_connection(h, first));
    ASSERT_TRUE(open_connection(h, second));
    ASSERT_TRUE(waiting.connect_to(h.port));
    EXPECT_EQ(h.timing.tick(), castle::status::ok);
    EXPECT_EQ(h.broker.client_count(), 2U);
    first.close();
    h.timing.pump();
    EXPECT_EQ(h.broker.client_count(), 2U);
    EXPECT_TRUE(waiting.silent());

    broker_type faulty;
    (void)open_loopback(faulty);
    int listener = faulty.transport().native_handle();
    int null_fd = ::open("/dev/null", O_RDONLY);
    ASSERT_GE(null_fd, 0);
    ASSERT_NE(::dup2(null_fd, listener), -1);
    ::close(null_fd);
    castle::status result = faulty.poll(1U, 0);
    EXPECT_NE(result, castle::status::ok);
    EXPECT_NE(result, castle::status::empty);
    EXPECT_NE(result, castle::status::full);
    (void)faulty.close();
}

TEST(MqttBroker, SubackSendFailureClosesTheConnection)
{
    harness<broker_type> h;
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect4("rst"), reply));
    ASSERT_TRUE(peer.send(subscribe(1U, filter("x/#"))));
    peer.reset();
    h.timing.pump();
    EXPECT_EQ(h.broker.client_count(), 0U);
}

TEST(MqttBroker, OversizedOutboundFramesAreDroppedWithoutDisturbingOthers)
{
    harness<tiny_type> h;
    raw_peer publisher;
    raw_peer exact;
    raw_peer qos_one;
    raw_peer version5;
    bytes reply;
    ASSERT_TRUE(login(h, publisher, connect4("p"), reply));
    ASSERT_TRUE(login(h, exact, connect4("a"), reply));
    ASSERT_TRUE(send_and_pump(h, exact, subscribe(1U, filter("t/#"))));
    expect_next(exact, suback(1U, bytes{0x00U}));
    ASSERT_TRUE(login(h, qos_one, connect4("b"), reply));
    ASSERT_TRUE(send_and_pump(h, qos_one, subscribe(1U, filter("t/#", 0x01U))));
    expect_next(qos_one, suback(1U, bytes{0x01U}));
    ASSERT_TRUE(login(h, version5, connect5("c"), reply));
    ASSERT_TRUE(send_and_pump(h, version5, subscribe(1U, filter("t/#", 0x01U), true)));
    expect_next(version5, suback(1U, bytes{0x01U}, true));

    char const topic[] = "t/abcdefghijklmnopqr";
    ASSERT_TRUE(send_and_pump(h, publisher, publish(topic, 1, false, 9U, filled(38U))));
    expect_next(publisher, ack(0x40U, 9U));
    expect_next(exact, publish(topic, 0, false, 0U, filled(38U)));
    expect_next(qos_one, publish(topic, 1, false, 1U, filled(38U)));
    EXPECT_TRUE(version5.silent());

    ASSERT_TRUE(send_and_pump(h, publisher, publish("t/s", 0, false, 0U, raw("ok"))));
    expect_next(exact, publish("t/s", 0, false, 0U, raw("ok")));
    expect_next(qos_one, publish("t/s", 0, false, 0U, raw("ok")));
    expect_next(version5, publish("t/s", 0, false, 0U, raw("ok"), true));

    ASSERT_TRUE(version5.send(subscribe(2U, filter("t/#"), true)));
    h.timing.pump();
    expect_next(version5, suback(2U, bytes{0x00U}, true));
    ASSERT_TRUE(send_and_pump(h, publisher, publish(topic, 0, false, 0U, filled(40U))));
    expect_next(exact, publish(topic, 0, false, 0U, filled(40U)));
    expect_next(qos_one, publish(topic, 0, false, 0U, filled(40U)));
    EXPECT_TRUE(version5.silent());
}

TEST(MqttBroker, OutboundQosOneCapacityDropsOverflowUntilAcknowledged)
{
    harness<broker_type> h;
    raw_peer subscriber;
    raw_peer publisher;
    bytes reply;
    ASSERT_TRUE(login(h, subscriber, connect4("s"), reply));
    ASSERT_TRUE(send_and_pump(h, subscriber, subscribe(1U, filter("b/#", 0x01U))));
    expect_next(subscriber, suback(1U, bytes{0x01U}));
    ASSERT_TRUE(login(h, publisher, connect4("p"), reply));

    bytes burst = publish("b/x", 1, false, 1U, raw("1"));
    append(burst, publish("b/x", 1, false, 2U, raw("2")));
    append(burst, publish("b/x", 1, false, 3U, raw("3")));
    ASSERT_TRUE(send_and_pump(h, publisher, burst));
    expect_next(publisher, ack(0x40U, 1U));
    expect_next(publisher, ack(0x40U, 2U));
    expect_next(publisher, ack(0x40U, 3U));
    expect_next(subscriber, publish("b/x", 1, false, 1U, raw("1")));
    expect_next(subscriber, publish("b/x", 1, false, 2U, raw("2")));
    EXPECT_TRUE(subscriber.silent());

    ASSERT_TRUE(send_and_pump(h, subscriber, ack(0x40U, 1U)));
    ASSERT_TRUE(send_and_pump(h, publisher, publish("b/x", 1, false, 4U, raw("4"))));
    expect_next(publisher, ack(0x40U, 4U));
    expect_next(subscriber, publish("b/x", 1, false, 3U, raw("4")));
}

// Subscribes `peer` to q/# at QoS 2 and publishes to it, leaving outbound QoS 2 identifier 1 awaiting PUBREC.
void arm_outbound_qos2(harness<broker_type>& h, raw_peer& peer, bool v5)
{
    bytes reply;
    ASSERT_TRUE(login(h, peer, v5 ? connect5("q") : connect4("q"), reply));
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(1U, filter("q/#", 0x02U), v5)));
    expect_next(peer, suback(1U, bytes{0x02U}, v5));
    ASSERT_TRUE(send_and_pump(h, peer, publish("q/1", 2, false, 5U, raw("a"), v5)));
    expect_next(peer, ack(0x50U, 5U));
    ASSERT_TRUE(send_and_pump(h, peer, ack(0x62U, 5U, v5 ? bytes{0x00U, 0x00U} : bytes{})));
    expect_next(peer, publish("q/1", 2, false, 1U, raw("a"), v5));
    expect_next(peer, ack(0x70U, 5U));
}

TEST(MqttBroker, OutboundQosTwoHandshakeFollowsThePeerAcknowledgements)
{
    harness<broker_type> h;
    raw_peer peer;
    arm_outbound_qos2(h, peer, false);
    ASSERT_TRUE(send_and_pump(h, peer, ack(0x50U, 1U)));
    expect_next(peer, ack(0x62U, 1U));
    ASSERT_TRUE(send_and_pump(h, peer, ack(0x70U, 1U)));
    EXPECT_TRUE(peer.silent());

    harness<broker_type> v5h;
    raw_peer v5peer;
    arm_outbound_qos2(v5h, v5peer, true);
    ASSERT_TRUE(send_and_pump(v5h, v5peer, ack(0x50U, 1U)));
    expect_next(v5peer, ack(0x62U, 1U, bytes{0x00U, 0x00U}));
    ASSERT_TRUE(send_and_pump(v5h, v5peer, ack(0x70U, 1U)));
    EXPECT_TRUE(v5peer.silent());
}

TEST(MqttBroker, OutboundQosTwoRejectsOutOfStateAcknowledgements)
{
    std::vector<wire_case> const cases{
        {"PUBACK for QoS 2 delivery", ack(0x40U, 1U), outcome::closed, {}},
        {"PUBCOMP before PUBREC", ack(0x70U, 1U), outcome::closed, {}},
        {"PUBREC for unknown identifier", ack(0x50U, 9U), outcome::closed, {}},
        {"PUBREC without identifier", packet(0x50U, bytes{0x00U}), outcome::closed, {}},
        {"PUBREC with identifier zero", ack(0x50U, 0U), outcome::closed, {}},
        {"version 3 PUBREC with trailing byte", ack(0x50U, 1U, bytes{0x00U}), outcome::closed, {}},
    };
    for (wire_case const& item : cases)
    {
        harness<broker_type> h;
        raw_peer peer;
        arm_outbound_qos2(h, peer, false);
        ASSERT_TRUE(send_and_pump(h, peer, item.wire)) << item.label;
        EXPECT_TRUE(peer.wait_closed()) << item.label;
        EXPECT_EQ(h.broker.client_count(), 0U) << item.label;
    }
}

// Negative version 5 acknowledgements release the outbound slot but must not produce a PUBREL.
TEST(MqttBroker, NegativeVersion5PubrecDoesNotTriggerPubrel)
{
    harness<broker_type> h;
    raw_peer peer;
    arm_outbound_qos2(h, peer, true);
    ASSERT_TRUE(peer.send(ack(0x50U, 1U, bytes{0x80U})));
    h.timing.pump();
    bytes leftover;
    (void)peer.wait_closed(leftover);
    EXPECT_TRUE(leftover.empty());
}

TEST(MqttBroker, OutboundAcknowledgementsCarryVersion5ReasonCodes)
{
    harness<broker_type> h;
    raw_peer subscriber;
    raw_peer publisher;
    bytes reply;
    ASSERT_TRUE(login(h, subscriber, connect5("b"), reply));
    ASSERT_TRUE(send_and_pump(h, subscriber, subscribe(1U, filter("b/#", 0x01U), true)));
    expect_next(subscriber, suback(1U, bytes{0x01U}, true));
    ASSERT_TRUE(login(h, publisher, connect4("p"), reply));

    bytes burst = publish("b/x", 1, false, 1U, raw("1"));
    append(burst, publish("b/x", 1, false, 2U, raw("2")));
    append(burst, publish("b/x", 1, false, 3U, raw("3")));
    ASSERT_TRUE(send_and_pump(h, publisher, burst));
    expect_next(publisher, ack(0x40U, 1U));
    expect_next(publisher, ack(0x40U, 2U));
    expect_next(publisher, ack(0x40U, 3U));
    expect_next(subscriber, publish("b/x", 1, false, 1U, raw("1"), true));
    expect_next(subscriber, publish("b/x", 1, false, 2U, raw("2"), true));
    EXPECT_TRUE(subscriber.silent());

    ASSERT_TRUE(send_and_pump(h, subscriber, ack(0x40U, 1U, bytes{0x00U, 0x00U})));
    ASSERT_TRUE(send_and_pump(h, publisher, publish("b/x", 1, false, 4U, raw("4"))));
    expect_next(publisher, ack(0x40U, 4U));
    expect_next(subscriber, publish("b/x", 1, false, 3U, raw("4"), true));

    ASSERT_TRUE(send_and_pump(h, subscriber, ack(0x40U, 3U, bytes{0x00U, 0x05U})));
    ASSERT_TRUE(subscriber.send(ack(0x40U, 2U, bytes{0x80U})));
    h.timing.pump();
    bytes leftover;
    (void)subscriber.wait_closed(leftover);
    EXPECT_TRUE(leftover.empty());
}

// Covers the decode-failure arms of DISCONNECT, PUBREL and PUBACK property blocks. The outcome is intentionally
// not asserted: such blocks are currently ignored instead of rejected (pre-existing issue, reported separately).
TEST(MqttBroker, MalformedSecondaryPropertyBlocksAreExercisedWithoutAssertingOutcome)
{
    {
        harness<broker_type> h;
        raw_peer peer;
        bytes reply;
        ASSERT_TRUE(login(h, peer, connect5("m1"), reply));
        ASSERT_TRUE(send_and_pump(h, peer, packet(0xE0U, bytes{0x00U, 0x05U})));
    }
    {
        harness<broker_type> h;
        raw_peer peer;
        bytes reply;
        ASSERT_TRUE(login(h, peer, connect5("m2"), reply));
        ASSERT_TRUE(send_and_pump(h, peer, publish("m", 2, false, 4U, raw("x"), true)));
        ASSERT_TRUE(send_and_pump(h, peer, ack(0x62U, 4U, bytes{0x00U, 0x05U})));
    }
    {
        harness<broker_type> h;
        raw_peer peer;
        bytes reply;
        ASSERT_TRUE(login(h, peer, connect5("m3"), reply));
        ASSERT_TRUE(send_and_pump(h, peer, subscribe(1U, filter("m/#", 0x01U), true)));
        ASSERT_TRUE(send_and_pump(h, peer, publish("m/x", 1, false, 4U, raw("x"), true)));
        ASSERT_TRUE(send_and_pump(h, peer, ack(0x40U, 1U, bytes{0x00U, 0x05U})));
    }
}

// Outbound identifiers wrap from 65535 to 1, skipping 0 and the identifier held by the stuck first delivery.
TEST(MqttBroker, QosOneIdentifiersWrapAroundSkippingInUseValues)
{
    harness<broker_type> h;
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect4("w"), reply));
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(1U, filter("w/#", 0x01U))));
    expect_next(peer, suback(1U, bytes{0x01U}));

    bytes const inbound = publish("w/x", 1, false, 1U, raw("p"));
    ASSERT_TRUE(send_and_pump(h, peer, inbound));
    expect_next(peer, publish("w/x", 1, false, 1U, raw("p")));
    expect_next(peer, ack(0x40U, 1U));

    constexpr uint32_t kRounds = 65540U;
    for (uint32_t round = 1U; round <= kRounds; ++round)
    {
        uint32_t expected = round <= 65534U ? round + 1U : round - 65533U;
        ASSERT_TRUE(peer.send(inbound)) << round;
        ASSERT_EQ(h.timing.tick(), castle::status::ok) << round;
        bytes delivered;
        bytes acknowledged;
        ASSERT_TRUE(peer.read_packet(delivered)) << round;
        ASSERT_TRUE(peer.read_packet(acknowledged)) << round;
        ASSERT_EQ(delivered.size(), 10U) << round;
        ASSERT_EQ(static_cast<uint32_t>((delivered[7U] << 8U) | delivered[8U]), expected) << round;
        ASSERT_EQ(acknowledged, ack(0x40U, 1U)) << round;
        ASSERT_TRUE(peer.send(ack(0x40U, static_cast<uint16_t>(expected)))) << round;
        ASSERT_EQ(h.timing.tick(), castle::status::ok) << round;
    }
    EXPECT_EQ(h.broker.client_count(), 1U);
}

TEST(MqttBroker, PubrelRejectsMalformedIdentifiersAndTrailingBytes)
{
    std::vector<wire_case> const version4{
        {"missing identifier", packet(0x62U, bytes{0x00U}), outcome::closed, {}},
        {"zero identifier", ack(0x62U, 0U), outcome::closed, {}},
        {"trailing byte before version 5", ack(0x62U, 3U, bytes{0x00U}), outcome::closed, {}},
    };
    for (wire_case const& item : version4) { expect_reaction(connect4("l"), item); }

    harness<broker_type> h;
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect5("l5"), reply));
    ASSERT_TRUE(send_and_pump(h, peer, publish("l", 2, false, 4U, raw("x"), true)));
    expect_next(peer, ack(0x50U, 4U));
    ASSERT_TRUE(send_and_pump(h, peer, ack(0x62U, 4U)));
    expect_next(peer, ack(0x70U, 4U));
}

TEST(MqttBroker, Version5SubscribeAcceptsRetainHandlingOptions)
{
    harness<broker_type> h;
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect5("v"), reply));
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(1U, filter("v/a", 0x10U), true)));
    expect_next(peer, suback(1U, bytes{0x00U}, true));
    EXPECT_EQ(h.broker.subscription_count(), 1U);
}

// Whole-text comparisons: identifiers and topics that are prefixes of one another must not match.
TEST(MqttBroker, IdentifiersAndTopicsCompareWholeText)
{
    harness<wide_type> h;
    raw_peer peer;
    bytes reply;
    ASSERT_TRUE(login(h, peer, connect4("ab", 0x00U), reply));
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(1U, filter("p/ab"))));
    expect_next(peer, suback(1U, bytes{0x00U}));
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(2U, filter("p/a"))));
    expect_next(peer, suback(2U, bytes{0x00U}));
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(3U, filter("p/abc"))));
    expect_next(peer, suback(3U, bytes{0x00U}));
    EXPECT_EQ(h.broker.subscription_count(), 3U);
    ASSERT_TRUE(send_and_pump(h, peer, subscribe(4U, filter("p/a"))));
    expect_next(peer, suback(4U, bytes{0x00U}));
    EXPECT_EQ(h.broker.subscription_count(), 3U);
    ASSERT_TRUE(send_and_pump(h, peer, unsubscribe(5U, text("p/a"))));
    expect_next(peer, unsuback(5U));
    EXPECT_EQ(h.broker.subscription_count(), 2U);
    ASSERT_TRUE(send_and_pump(h, peer, unsubscribe(6U, text("p/zz"))));
    expect_next(peer, unsuback(6U));
    EXPECT_EQ(h.broker.subscription_count(), 2U);

    ASSERT_TRUE(send_and_pump(h, peer, publish("r/10", 0, true, 0U, raw("x"))));
    ASSERT_TRUE(send_and_pump(h, peer, publish("r/1", 0, true, 0U, raw("y"))));
    EXPECT_EQ(h.broker.retained_count(), 2U);
    ASSERT_TRUE(send_and_pump(h, peer, publish("r/10", 0, true, 0U, raw("z"))));
    EXPECT_EQ(h.broker.retained_count(), 2U);
    peer.close();
    h.timing.pump();

    ASSERT_TRUE(login(h, peer, connect4("a", 0x00U), reply));
    EXPECT_EQ(reply, kConnackV4);
    peer.close();
    h.timing.pump();
    ASSERT_TRUE(login(h, peer, connect4("abc", 0x00U), reply));
    EXPECT_EQ(reply, kConnackV4);
    peer.close();
    h.timing.pump();
    ASSERT_TRUE(login(h, peer, connect4("ab", 0x00U), reply));
    EXPECT_EQ(reply, kConnackV4Session);
    EXPECT_EQ(h.broker.subscription_count(), 2U);
}

// A takeover that happens earlier in the same poll marks the old connection pending; a publish processed
// before the old connection is closed must not be delivered to it.
TEST(MqttBroker, PendingTakeoverVictimReceivesNoDeliveries)
{
    harness<wide_type> h;
    raw_peer padding;
    raw_peer publisher;
    raw_peer victim;
    raw_peer taker;
    bytes reply;
    ASSERT_TRUE(login(h, padding, connect4("pad"), reply));
    ASSERT_TRUE(login(h, publisher, connect4("p"), reply));
    ASSERT_TRUE(login(h, victim, connect4("dup", 0x00U), reply));
    padding.close();
    h.timing.pump();
    ASSERT_TRUE(send_and_pump(h, victim, subscribe(1U, filter("t/#"))));
    expect_next(victim, suback(1U, bytes{0x00U}));

    ASSERT_TRUE(open_connection(h, taker));
    ASSERT_TRUE(taker.send(connect4("dup", 0x00U)));
    ASSERT_TRUE(publisher.send(publish("t/x", 0, false, 0U, raw("v"))));
    h.timing.pump();
    bytes leftover;
    EXPECT_TRUE(victim.wait_closed(leftover));
    EXPECT_TRUE(leftover.empty());
    EXPECT_EQ(h.broker.client_count(), 2U);
}


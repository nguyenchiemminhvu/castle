#include <gtest/gtest.h>
#include "castle_ext/protocols/dns/client.hpp"

namespace
{
using castle::size_type;
using castle::status;
using castle::net::address_family;
using castle::net::endpoint;

// Scripted statuses are consumed one per call, after `skip` ordinary calls have passed.
struct status_script
{
    status values[4]{};
    size_type count = 0U;
    size_type index = 0U;
    size_type skip = 0U;

    bool next(status& out)
    {
        if (skip > 0U)
        {
            --skip;
            return false;
        }
        if (index >= count)
        {
            return false;
        }
        out = values[index++];
        return true;
    }
};

void script_one(status_script& script, status value, size_type skip = 0U)
{
    script.values[0U] = value;
    script.count = 1U;
    script.skip = skip;
}

struct fake_dns_state
{
    uint8_t udp_packet[512U]{};
    size_type udp_size = 0U;
    bool udp_pending = false;
    bool udp_truncated = false;
    bool udp_oversized = false;
    bool udp_oversized_success = false;
    uint8_t udp_rcode = 0U;
    bool drop_udp = false;
    endpoint peer{};
    uint8_t tcp_frame[514U]{};
    size_type tcp_frame_size = 0U;

    status udp_open_error = status::ok;
    status udp_close_error = status::ok;
    status_script udp_send_script;
    bool udp_send_short = false;
    status_script udp_wait_script;
    status_script udp_receive_script;
    bool foreign_next = false;

    status_script tcp_connect_script;
    status_script finish_script;
    status_script tcp_send_script;
    status_script tcp_send_all_script;
    status_script tcp_receive_script;
    status tcp_wait_error = status::ok;
    bool tcp_send_all_short = false;
    bool tcp_peer_closed_next = false;
    size_type tcp_read_limit = 0U;
    size_type tcp_send_limit = 0U;

    bool bad_id = false;
    bool tcp_truncated = false;
    int tcp_prefix_override = -1;
    uint8_t question_alt[64U]{};
    size_type question_alt_size = 0U;

    static void write_u16(uint8_t* p, uint16_t value)
    {
        p[0U] = static_cast<uint8_t>(value >> 8U);
        p[1U] = static_cast<uint8_t>(value & 0xFFU);
    }

    void make_response(uint8_t const* query, size_type query_size, bool truncated)
    {
        // The question is echoed from the query unless a test substitutes its own bytes.
        uint8_t const* question = query + 12U;
        size_type question_size = query_size - 12U;
        if (question_alt_size != 0U)
        {
            question = question_alt;
            question_size = question_alt_size;
        }
        udp_packet[0U] = bad_id ? static_cast<uint8_t>(query[0U] ^ 0xFFU) : query[0U];
        udp_packet[1U] = query[1U];
        udp_packet[2U] = truncated ? 0x83U : 0x81U;
        udp_packet[3U] = static_cast<uint8_t>(0x80U | (udp_rcode & 0x0FU));
        write_u16(udp_packet + 4U, 1U);
        write_u16(udp_packet + 6U, 1U);
        write_u16(udp_packet + 8U, 0U);
        write_u16(udp_packet + 10U, 0U);
        size_type at = 12U;
        for (size_type i = 0U; i < question_size; ++i) { udp_packet[at++] = question[i]; }
        udp_packet[at++] = 0xC0U; udp_packet[at++] = 0x0CU;
        write_u16(udp_packet + at, 1U); at += 2U;
        write_u16(udp_packet + at, 1U); at += 2U;
        udp_packet[at++] = 0U; udp_packet[at++] = 0U; udp_packet[at++] = 0U; udp_packet[at++] = 45U;
        write_u16(udp_packet + at, 4U); at += 2U;
        udp_packet[at++] = 203U; udp_packet[at++] = 0U; udp_packet[at++] = 113U; udp_packet[at++] = 8U;
        udp_size = at;
        udp_pending = !drop_udp;

        // The TCP fallback carries the same message with the TC bit clear unless a test sets it.
        int prefix = tcp_prefix_override >= 0 ? tcp_prefix_override : static_cast<int>(udp_size);
        tcp_frame[0U] = static_cast<uint8_t>(prefix >> 8U);
        tcp_frame[1U] = static_cast<uint8_t>(prefix & 0xFF);
        for (size_type i = 0U; i < udp_size; ++i) { tcp_frame[i + 2U] = udp_packet[i]; }
        tcp_frame[4U] = tcp_truncated ? 0x83U : 0x81U;
        tcp_frame_size = udp_size + 2U;
    }
};

struct fake_udp_socket
{
    fake_dns_state* state = nullptr;
    address_family family_value = address_family::ipv4;
    address_family family() const { return family_value; }
    status wait_readable(int, int*)
    {
        status scripted = status::ok;
        if (state->udp_wait_script.next(scripted))
        {
            return scripted;
        }
        return status::ok;
    }
};

struct fake_udp
{
    explicit fake_udp(fake_dns_state& value) : state(value) { socket_value.state = &value; }
    status open(address_family family, bool nonblocking, int*)
    {
        if (state.udp_open_error != status::ok)
        {
            return state.udp_open_error;
        }
        opened = true; socket_value.family_value = family; nonblocking_value = nonblocking; return status::ok;
    }
    status close(int*)
    {
        if (state.udp_close_error != status::ok)
        {
            if (state.udp_close_error == status::not_configured)
            {
                opened = false;
            }
            return state.udp_close_error;
        }
        opened = false;
        return status::ok;
    }
    bool is_open() const { return opened; }
    status set_nonblocking(bool value, int*) { nonblocking_value = value; return status::ok; }
    fake_udp_socket& socket() { return socket_value; }
    status send_to(void const* data, size_type size, endpoint const& remote, size_type& sent, int*)
    {
        sent = 0U;
        status scripted = status::ok;
        if (state.udp_send_script.next(scripted))
        {
            return scripted;
        }
        state.peer = remote;
        state.make_response(static_cast<uint8_t const*>(data), size, state.udp_truncated);
        sent = state.udp_send_short ? size - 1U : size;
        return status::ok;
    }
    status receive_from(void* output, size_type capacity, size_type& received, endpoint& remote, int*)
    {
        received = 0U;
        status scripted = status::ok;
        if (state.udp_receive_script.next(scripted))
        {
            remote = state.peer;
            return scripted;
        }
        if (state.foreign_next)
        {
            state.foreign_next = false;
            remote = endpoint::ipv4(9U, 9U, 9U, 9U, 53U);
            if (state.udp_oversized)
            {
                state.udp_oversized = false;
                return status::data_loss;
            }
            return status::ok;
        }
        remote = state.peer;
        if (state.udp_oversized) { state.udp_oversized = false; return status::data_loss; }
        if (state.udp_oversized_success)
        {
            received = capacity;
            state.udp_oversized_success = false;
            return status::ok;
        }
        if (!state.udp_pending) { return status::empty; }
        received = state.udp_size < capacity ? state.udp_size : capacity;
        for (size_type i = 0U; i < received; ++i) { static_cast<uint8_t*>(output)[i] = state.udp_packet[i]; }
        state.udp_pending = false;
        return status::ok;
    }
    fake_dns_state& state;
    fake_udp_socket socket_value{};
    bool opened = false;
    bool nonblocking_value = false;
};


struct fake_tcp_socket
{
    fake_dns_state* state = nullptr;
    address_family family_value = address_family::ipv4;
    address_family family() const { return family_value; }
    status finish_connect(int, int*)
    {
        status scripted = status::ok;
        if (state->finish_script.next(scripted))
        {
            return scripted;
        }
        return status::ok;
    }
    status wait_readable(int, int*) { return state->tcp_wait_error; }
};

struct fake_tcp
{
    explicit fake_tcp(fake_dns_state& value) : state(value) { socket_value.state = &value; }
    status connect(endpoint const& remote, int, bool, int*)
    {
        ++connect_calls;
        status scripted = status::ok;
        if (state.tcp_connect_script.next(scripted))
        {
            return scripted;
        }
        opened = true; socket_value.family_value = remote.family(); offset = 0U; return status::ok;
    }
    status close(int*) { opened = false; return status::ok; }
    bool is_open() const { return opened; }
    fake_tcp_socket& socket() { return socket_value; }
    status send_all(void const*, size_type size, size_type& sent, int*)
    {
        sent = 0U;
        status scripted = status::ok;
        if (state.tcp_send_all_script.next(scripted))
        {
            return scripted;
        }
        sent = state.tcp_send_all_short ? size - 1U : size;
        return status::ok;
    }
    status send_some(void const*, size_type size, size_type& sent, int*)
    {
        sent = 0U;
        status scripted = status::ok;
        if (state.tcp_send_script.next(scripted))
        {
            return scripted;
        }
        sent = (state.tcp_send_limit != 0U && size > state.tcp_send_limit) ? state.tcp_send_limit : size;
        return status::ok;
    }
    status receive_some(void* output, size_type capacity, size_type& received, int*, bool* peer_closed)
    {
        if (peer_closed != nullptr) { *peer_closed = false; }
        received = 0U;
        status scripted = status::ok;
        if (state.tcp_receive_script.next(scripted))
        {
            return scripted;
        }
        if (state.tcp_peer_closed_next)
        {
            state.tcp_peer_closed_next = false;
            if (peer_closed != nullptr) { *peer_closed = true; }
            return status::ok;
        }
        size_type wanted = capacity;
        if (state.tcp_read_limit != 0U && wanted > state.tcp_read_limit) { wanted = state.tcp_read_limit; }
        size_type available = state.tcp_frame_size - offset;
        received = available < wanted ? available : wanted;
        for (size_type i = 0U; i < received; ++i) { static_cast<uint8_t*>(output)[i] = state.tcp_frame[offset++]; }
        return received == 0U ? status::empty : status::ok;
    }
    fake_dns_state& state;
    fake_tcp_socket socket_value{};
    size_type offset = 0U;
    size_type connect_calls = 0U;
    bool opened = false;
};

using test_client = castle::protocols::dns::basic_client<fake_udp, fake_tcp, 4U, 512U>;

} // namespace

TEST(DnsClient, BlockingResolveReturnsARecordOverUdp)
{
    fake_dns_state state;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    castle::protocols::dns::response<4U> answer;
    EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 0U), "example.com",
                               castle::protocols::dns::record_type::a, answer), status::ok);
    ASSERT_EQ(answer.record_count, 1U);
    EXPECT_EQ(answer.records[0U].address.address_bytes()[0U], 203U);
    EXPECT_EQ(answer.records[0U].address.address_bytes()[3U], 8U);
    EXPECT_EQ(answer.records[0U].ttl, 45U);
    EXPECT_EQ(tcp.connect_calls, 0U);
}

TEST(DnsClient, BlockingResolveRetriesUdpTimeout)
{
    fake_dns_state state;
    state.drop_udp = true;
    fake_udp udp(state);
    fake_tcp tcp(state);
    castle::protocols::dns::client_config config;
    config.timeout_ms = 1U;
    config.retries = 1U;
    test_client resolver(udp, tcp, config);
    castle::protocols::dns::response<4U> answer;
    EXPECT_EQ(resolver.resolve(endpoint::ipv4(8U, 8U, 8U, 8U, 53U), "example.com",
                               castle::protocols::dns::record_type::a, answer), status::time_out);
}

TEST(DnsClient, BlockingResolveFallsBackToTcpOnTcFlag)
{
    fake_dns_state state;
    state.udp_truncated = true;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    castle::protocols::dns::response<4U> answer;
    EXPECT_EQ(resolver.resolve(endpoint::ipv4(9U, 9U, 9U, 9U, 53U), "example.com",
                               castle::protocols::dns::record_type::a, answer), status::ok);
    EXPECT_EQ(tcp.connect_calls, 1U);
    ASSERT_EQ(answer.record_count, 1U);
    EXPECT_EQ(answer.records[0U].address.address_bytes()[2U], 113U);
}

TEST(DnsClient, BlockingResolveFallsBackWhenUdpDatagramWasLargerThanBuffer)
{
    fake_dns_state state;
    state.udp_oversized = true;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    castle::protocols::dns::response<4U> answer;
    EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 0U, 0U, 1U, 53U), "example.com",
                               castle::protocols::dns::record_type::a, answer), status::ok);
    EXPECT_EQ(tcp.connect_calls, 1U);
}

TEST(DnsClient, AsyncResolveCopiesNameAndInvokesCallback)
{
    fake_dns_state state;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    status callback_status = status::not_configured;
    size_type callback_records = 0U;
    ASSERT_EQ(resolver.set_callback([&](status result, test_client::response_type const& response)
    {
        callback_status = result;
        callback_records = response.record_count;
    }), status::ok);
    char domain[] = "example.com";
    ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), domain,
                                     castle::protocols::dns::record_type::a, 100U), status::ok);
    domain[0U] = 'X'; // the in-flight question/validation must not borrow caller storage
    EXPECT_TRUE(resolver.busy());
    EXPECT_EQ(resolver.poll(101U), status::ok);
    EXPECT_FALSE(resolver.busy());
    EXPECT_EQ(callback_status, status::ok);
    EXPECT_EQ(callback_records, 1U);
    EXPECT_STREQ(resolver.last_response().question_name, "example.com");
}

TEST(DnsClient, AsyncResolveFallsBackToTcp)
{
    fake_dns_state state;
    state.udp_truncated = true;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(8U, 8U, 4U, 4U, 53U), "example.com",
                                     castle::protocols::dns::record_type::a, 0U), status::ok);
    EXPECT_EQ(resolver.poll(1U), status::ok); // UDP TC -> open TCP
    EXPECT_EQ(resolver.poll(2U), status::ok); // send framed query
    EXPECT_EQ(resolver.poll(3U), status::ok); // read TCP length
    EXPECT_EQ(resolver.poll(4U), status::ok); // read TCP body and parse
    EXPECT_FALSE(resolver.busy());
    EXPECT_EQ(resolver.last_status(), status::ok);
    ASSERT_EQ(resolver.last_response().record_count, 1U);
    EXPECT_EQ(tcp.connect_calls, 1U);
}

TEST(DnsClient, AsyncResolveRetriesThenTimesOut)
{
    fake_dns_state state;
    state.drop_udp = true;
    fake_udp udp(state);
    fake_tcp tcp(state);
    castle::protocols::dns::client_config config;
    config.timeout_ms = 10U;
    config.retries = 1U;
    test_client resolver(udp, tcp, config);
    status callback_status = status::ok;
    ASSERT_EQ(resolver.set_callback([&](status result, test_client::response_type const&) { callback_status = result; }), status::ok);
    ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com",
                                     castle::protocols::dns::record_type::a, 100U), status::ok);
    EXPECT_EQ(resolver.poll(109U), status::empty);
    EXPECT_EQ(resolver.poll(110U), status::ok); // retransmit
    EXPECT_TRUE(resolver.busy());
    EXPECT_EQ(resolver.poll(120U), status::time_out);
    EXPECT_FALSE(resolver.busy());
    EXPECT_EQ(callback_status, status::time_out);
}

TEST(DnsClient, RejectsInvalidNameAndConcurrentStart)
{
    fake_dns_state state;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    castle::protocols::dns::response<4U> answer;
    EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "bad..name",
                               castle::protocols::dns::record_type::a, answer), status::invalid_argument);
    ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com",
                                     castle::protocols::dns::record_type::a, 0U), status::ok);
    EXPECT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "other.com",
                                     castle::protocols::dns::record_type::a, 1U), status::invalid_config);
}


TEST(DnsClient, CallbackCanClearOrReplaceItselfWithoutDestroyingRunningCallable)
{
    fake_dns_state state;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    size_type first_calls = 0U;
    size_type replacement_calls = 0U;

    ASSERT_EQ(resolver.set_callback([&](status, test_client::response_type const&)
    {
        ++first_calls;
        EXPECT_TRUE(resolver.busy());
        EXPECT_EQ(resolver.poll(999U), status::invalid_config);
        EXPECT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "reentrant.test",
                                         castle::protocols::dns::record_type::a, 999U), status::invalid_config);
        EXPECT_EQ(resolver.set_callback([&](status, test_client::response_type const&) { ++replacement_calls; }), status::ok);
    }), status::ok);

    ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com",
                                     castle::protocols::dns::record_type::a, 0U), status::ok);
    EXPECT_EQ(resolver.poll(1U), status::ok);
    EXPECT_EQ(first_calls, 1U);
    EXPECT_FALSE(resolver.busy());

    ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com",
                                     castle::protocols::dns::record_type::a, 2U), status::ok);
    EXPECT_EQ(resolver.poll(3U), status::ok);
    EXPECT_EQ(replacement_calls, 1U);
}


TEST(DnsClient, DetectsOversizedUdpPayloadUsingOneByteProbe)
{
    fake_dns_state state;
    state.udp_oversized_success = true;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    test_client::response_type answer;
    EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com",
                               castle::protocols::dns::record_type::a, answer), status::ok);
    EXPECT_EQ(tcp.connect_calls, 1U);
}


TEST(DnsClient, MapsDnsResponseCodesToCastleStatuses)
{
    fake_dns_state state;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    test_client::response_type answer;

    state.udp_rcode = 3U; // NXDOMAIN
    EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "missing.example",
                               castle::protocols::dns::record_type::a, answer), status::not_found);
    EXPECT_EQ(answer.rcode, castle::protocols::dns::response_code::name_error);

    state.udp_rcode = 5U; // REFUSED
    EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "refused.example",
                               castle::protocols::dns::record_type::a, answer), status::unknown_error);
    EXPECT_EQ(answer.rcode, castle::protocols::dns::response_code::refused);
}

namespace
{
using castle::protocols::dns::record_type;

void set_question(fake_dns_state& state, uint8_t const* bytes, size_type size)
{
    for (size_type i = 0U; i < size; ++i) { state.question_alt[i] = bytes[i]; }
    state.question_alt_size = size;
}

// Polls until the operation completes and returns its final status.
status drive(test_client& resolver, uint32_t& now_ms)
{
    for (uint32_t i = 0U; i < 1000U && resolver.busy(); ++i)
    {
        resolver.poll(now_ms++);
    }
    return resolver.last_status();
}
}

TEST(DnsClient, RejectsEmptyCallbackObject)
{
    fake_dns_state state;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    EXPECT_EQ(resolver.set_callback(test_client::callback_type()), status::invalid_callback);
    EXPECT_FALSE(resolver.busy());
}

TEST(DnsClient, ResolveRejectsWhileAsyncRequestIsInFlight)
{
    fake_dns_state state;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    castle::protocols::dns::response<4U> answer;
    ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U), status::ok);
    EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
              status::invalid_config);
    EXPECT_EQ(resolver.poll(1U), status::ok);
}

TEST(DnsClient, ReportsSocketOpenFailuresOnBothEntryPoints)
{
    {
        fake_dns_state state;
        state.udp_open_error = status::unknown_error;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        castle::protocols::dns::response<4U> answer;
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::unknown_error);
    }
    {
        fake_dns_state state;
        state.udp_open_error = status::unknown_error;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::unknown_error);
        EXPECT_FALSE(resolver.busy());
    }
}

TEST(DnsClient, StartResolveRejectsBadNameAndReportsSendFailures)
{
    {
        fake_dns_state state;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "bad..name", record_type::a, 0U),
                  status::invalid_argument);
        EXPECT_FALSE(resolver.busy());
    }
    {
        fake_dns_state state;
        script_one(state.udp_send_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::unknown_error);
        EXPECT_FALSE(resolver.busy());
    }
    {
        fake_dns_state state;
        state.udp_send_short = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::data_loss);
    }
}

TEST(DnsClient, BlockingResolveReportsSendFailureAndShortDatagram)
{
    castle::protocols::dns::response<4U> answer;
    {
        fake_dns_state state;
        script_one(state.udp_send_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::unknown_error);
    }
    {
        fake_dns_state state;
        state.udp_send_short = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::data_loss);
    }
}

TEST(DnsClient, BlockingResolveRetriesWaitTimeoutsAndReportsWaitErrors)
{
    castle::protocols::dns::response<4U> answer;
    {
        fake_dns_state state;
        script_one(state.udp_wait_script, status::time_out);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::ok);
        EXPECT_EQ(answer.record_count, 1U);
    }
    {
        fake_dns_state state;
        script_one(state.udp_wait_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::unknown_error);
    }
}

TEST(DnsClient, BlockingResolveHandlesReceiveErrorsAndForeignDatagrams)
{
    castle::protocols::dns::response<4U> answer;
    {
        fake_dns_state state;
        script_one(state.udp_receive_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::unknown_error);
    }
    {
        fake_dns_state state;
        state.foreign_next = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::ok);
    }
    {
        fake_dns_state state;
        state.foreign_next = true;
        state.udp_oversized = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::ok);
    }
}

TEST(DnsClient, BlockingResolveRejectsMismatchedOrCorruptResponses)
{
    castle::protocols::dns::response<4U> answer;
    {
        fake_dns_state state;
        state.bad_id = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::invalid_argument);
    }
    {
        fake_dns_state state;
        uint8_t const question[] = {7U, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 3U, 'c', 'o', 'm', 0U, 0U, 28U, 0U, 1U};
        set_question(state, question, sizeof(question));
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::invalid_argument);
    }
    {
        fake_dns_state state;
        uint8_t const question[] = {7U, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 3U, 'c', 'o', 'm', 0U, 0U, 1U, 0U, 3U};
        set_question(state, question, sizeof(question));
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::invalid_argument);
    }
    {
        fake_dns_state state;
        uint8_t const question[] = {3U, 'c', 'o', 'm', 0U, 0U, 1U, 0U, 1U};
        set_question(state, question, sizeof(question));
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::invalid_argument);
    }
    {
        fake_dns_state state;
        uint8_t const question[] = {7U, 'e', 'x', 'z', 'm', 'p', 'l', 'e', 3U, 'c', 'o', 'm', 0U, 0U, 1U, 0U, 1U};
        set_question(state, question, sizeof(question));
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::invalid_argument);
    }
}

TEST(DnsClient, BlockingResolveAcceptsMixedCaseAndTrailingDot)
{
    fake_dns_state state;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    castle::protocols::dns::response<4U> answer;
    EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "Example.COM.", record_type::a, answer),
              status::ok);
    EXPECT_STREQ(answer.question_name, "Example.COM");
}

TEST(DnsClient, BlockingResolveReportsTruncatedAndFormatErrorResponses)
{
    castle::protocols::dns::response<4U> answer;
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_truncated = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::data_loss);
    }
    {
        fake_dns_state state;
        state.udp_rcode = 1U; // FORMERR
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::invalid_argument);
    }
}

TEST(DnsClient, BlockingResolveUsesIpv6NameserverAndSwitchesSocketFamily)
{
    uint8_t v6[16U]{};
    v6[0U] = 0x20U;
    v6[1U] = 0x01U;
    v6[15U] = 1U;
    endpoint const ipv6_server = endpoint::ipv6(v6, 53U);
    castle::protocols::dns::response<4U> answer;
    {
        fake_dns_state state;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(ipv6_server, "example.com", record_type::a, answer), status::ok);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::ok);
    }
    {
        fake_dns_state state;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::ok);
        state.udp_close_error = status::unknown_error;
        EXPECT_EQ(resolver.resolve(ipv6_server, "example.com", record_type::a, answer), status::unknown_error);
    }
    {
        fake_dns_state state;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::ok);
        state.udp_close_error = status::not_configured;
        EXPECT_EQ(resolver.resolve(ipv6_server, "example.com", record_type::a, answer), status::ok);
    }
}

TEST(DnsClient, BlockingResolveClampsZeroAndHugeTimeouts)
{
    castle::protocols::dns::response<4U> answer;
    {
        fake_dns_state state;
        fake_udp udp(state);
        fake_tcp tcp(state);
        castle::protocols::dns::client_config config;
        config.timeout_ms = 0U;
        test_client resolver(udp, tcp, config);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::ok);
    }
    {
        fake_dns_state state;
        fake_udp udp(state);
        fake_tcp tcp(state);
        castle::protocols::dns::client_config config;
        config.timeout_ms = 0xFFFFFFFFU;
        test_client resolver(udp, tcp, config);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::ok);
    }
}

TEST(DnsClient, BlockingTcpFallbackHandlesConnectOutcomes)
{
    castle::protocols::dns::response<4U> answer;
    {
        fake_dns_state state;
        state.udp_truncated = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        tcp.opened = true;
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::ok);
        EXPECT_EQ(tcp.connect_calls, 1U);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_connect_script, status::time_out);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::ok);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_connect_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::unknown_error);
    }
}

TEST(DnsClient, BlockingTcpReportsSendFailures)
{
    castle::protocols::dns::response<4U> answer;
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_send_all_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::unknown_error);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_send_all_short = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::data_loss);
    }
}

TEST(DnsClient, BlockingTcpReportsReceiveAndFramingFailures)
{
    castle::protocols::dns::response<4U> answer;
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_wait_error = status::unknown_error;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::unknown_error);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_peer_closed_next = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::data_loss);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_receive_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::unknown_error);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_receive_script, status::unknown_error, 1U);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::unknown_error);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_receive_script, status::empty, 1U);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::time_out);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_receive_script, status::empty);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::time_out);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_prefix_override = 5;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::full);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_prefix_override = 600;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer),
                  status::full);
    }
}

TEST(DnsClient, BlockingTcpKeepsWaitingAfterZeroByteSuccess)
{
    fake_dns_state state;
    state.udp_truncated = true;
    script_one(state.tcp_receive_script, status::ok, 1U); // body read reports success without octets
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    castle::protocols::dns::response<4U> answer;
    EXPECT_EQ(resolver.resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, answer), status::ok);
    EXPECT_EQ(answer.record_count, 1U);
}

TEST(DnsClient, AsyncPollWithoutStartReportsNotConfigured)
{
    fake_dns_state state;
    fake_udp udp(state);
    fake_tcp tcp(state);
    test_client resolver(udp, tcp);
    EXPECT_EQ(resolver.poll(0U), status::not_configured);
}

TEST(DnsClient, AsyncRetransmitAndReceiveFailuresCompleteWithStatus)
{
    {
        fake_dns_state state;
        state.drop_udp = true;
        script_one(state.udp_send_script, status::unknown_error, 1U); // first send succeeds, retransmit fails
        fake_udp udp(state);
        fake_tcp tcp(state);
        castle::protocols::dns::client_config config;
        config.timeout_ms = 10U;
        config.retries = 1U;
        test_client resolver(udp, tcp, config);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 100U),
                  status::ok);
        EXPECT_EQ(resolver.poll(109U), status::empty);
        EXPECT_EQ(resolver.poll(110U), status::unknown_error);
        EXPECT_FALSE(resolver.busy());
    }
    {
        fake_dns_state state;
        script_one(state.udp_receive_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        EXPECT_EQ(resolver.poll(1U), status::unknown_error);
        EXPECT_FALSE(resolver.busy());
    }
}

TEST(DnsClient, AsyncForeignDatagramsAndOversizedResponsesFallBackToTcp)
{
    {
        fake_dns_state state;
        state.foreign_next = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        EXPECT_EQ(resolver.poll(1U), status::empty);
        EXPECT_EQ(resolver.poll(2U), status::ok);
    }
    {
        fake_dns_state state;
        state.foreign_next = true;
        state.udp_oversized = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        EXPECT_EQ(resolver.poll(1U), status::empty);
        EXPECT_EQ(resolver.poll(2U), status::ok);
    }
    {
        fake_dns_state state;
        state.udp_oversized = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        EXPECT_EQ(resolver.poll(1U), status::ok);
        uint32_t now = 2U;
        EXPECT_EQ(drive(resolver, now), status::ok);
        EXPECT_EQ(tcp.connect_calls, 1U);
    }
    {
        fake_dns_state state;
        state.udp_oversized_success = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        EXPECT_EQ(resolver.poll(1U), status::ok);
        uint32_t now = 2U;
        EXPECT_EQ(drive(resolver, now), status::ok);
    }
}

TEST(DnsClient, AsyncTcpConnectStateMachineHandlesBusyAndTimeouts)
{
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_connect_script, status::time_out);
        state.finish_script.values[0U] = status::time_out;
        state.finish_script.values[1U] = status::full;
        state.finish_script.values[2U] = status::empty;
        state.finish_script.count = 3U;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        EXPECT_EQ(resolver.poll(1U), status::ok);
        EXPECT_EQ(resolver.poll(2U), status::empty);
        EXPECT_EQ(resolver.poll(3U), status::empty);
        EXPECT_EQ(resolver.poll(4U), status::empty);
        uint32_t now = 5U;
        EXPECT_EQ(drive(resolver, now), status::ok);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_connect_script, status::time_out);
        script_one(state.finish_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        EXPECT_EQ(resolver.poll(1U), status::ok);
        EXPECT_EQ(resolver.poll(2U), status::unknown_error);
        EXPECT_FALSE(resolver.busy());
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_connect_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        EXPECT_EQ(resolver.poll(1U), status::unknown_error);
        EXPECT_FALSE(resolver.busy());
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_connect_script, status::time_out);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        EXPECT_EQ(resolver.poll(1U), status::ok);
        EXPECT_EQ(resolver.poll(1002U), status::time_out);
        EXPECT_FALSE(resolver.busy());
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        tcp.opened = true;
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::ok);
        EXPECT_EQ(tcp.connect_calls, 1U);
    }
}

TEST(DnsClient, AsyncTcpSendAndReceiveFailuresAndPartialTransfers)
{
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_send_script.values[0U] = status::full;
        state.tcp_send_script.values[1U] = status::empty;
        state.tcp_send_script.count = 2U;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        EXPECT_EQ(resolver.poll(1U), status::ok);
        EXPECT_EQ(resolver.poll(2U), status::empty);
        EXPECT_EQ(resolver.poll(3U), status::empty);
        uint32_t now = 4U;
        EXPECT_EQ(drive(resolver, now), status::ok);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_send_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::unknown_error);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_send_limit = 1U;
        state.tcp_read_limit = 1U;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::ok);
        EXPECT_EQ(resolver.last_response().record_count, 1U);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_receive_script, status::empty);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::ok);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_receive_script, status::full);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::ok);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_receive_script, status::unknown_error);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::unknown_error);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_peer_closed_next = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::data_loss);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_receive_script, status::empty, 1U);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::ok);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        script_one(state.tcp_receive_script, status::unknown_error, 1U);
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::unknown_error);
    }
}

TEST(DnsClient, AsyncTcpRejectsFramingTruncationAndCorruptReplies)
{
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_prefix_override = 5;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::full);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_prefix_override = 600;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::full);
    }
    {
        fake_dns_state state;
        state.udp_truncated = true;
        state.tcp_truncated = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        uint32_t now = 1U;
        EXPECT_EQ(drive(resolver, now), status::data_loss);
    }
    {
        fake_dns_state state;
        state.bad_id = true;
        fake_udp udp(state);
        fake_tcp tcp(state);
        test_client resolver(udp, tcp);
        ASSERT_EQ(resolver.start_resolve(endpoint::ipv4(1U, 1U, 1U, 1U, 53U), "example.com", record_type::a, 0U),
                  status::ok);
        EXPECT_EQ(resolver.poll(1U), status::invalid_argument);
        EXPECT_FALSE(resolver.busy());
    }
}


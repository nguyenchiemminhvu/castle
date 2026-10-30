// Online CoAP client sample. Start a CoAP server on 127.0.0.1:5683 first, for example:
//   coap-server-notls -v 9 -d 10
// Then run this sample. It sends PUT /temperature with "30.0", then GET /temperature and
// checks that the stored value is returned.
#include "sample_support.hpp"
#include "castle/chrono/chrono.hpp"
#include "castle_ext/protocols/coap/client.hpp"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

namespace
{
using client_type = castle::protocols::coap::client;

static CASTLE_CONSTEXPR castle::size_type payload_capacity = 64U;

struct exchange_result
{
    bool completed = false;
    bool received = false;
    uint8_t code_value = 0U;
    uint8_t payload[payload_capacity]{};
    castle::size_type payload_size = 0U;
};

uint32_t elapsed_ms(castle::chrono::steady_clock::time_point start)
{
    return static_cast<uint32_t>(castle::chrono::duration_cast<castle::chrono::milliseconds>(
        castle::chrono::steady_clock::now() - start).count());
}

bool send_and_wait(client_type& client, castle::net::endpoint const& peer,
                   client_type::message_type_t const& request, exchange_result& result,
                   castle::chrono::steady_clock::time_point start)
{
    result = exchange_result{};
    client_type::exchange_handle exchange;
    if (client.send_request(peer, request, exchange, elapsed_ms(start)) != castle::status::ok)
    {
        return false;
    }
    const castle::chrono::steady_clock::time_point deadline = castle::chrono::steady_clock::now() + castle::chrono::seconds(5);
    while (!result.completed && castle::chrono::steady_clock::now() < deadline)
    {
        const castle::status status = client.process_once(elapsed_ms(start), 50);
        if (status != castle::status::ok && status != castle::status::empty && status != castle::status::not_found)
        {
            return false;
        }
    }
    return result.completed && result.received;
}

void print_code(const char* label, uint8_t code_value)
{
    printf("%s -> CoAP code %u.%02u\n", label, static_cast<unsigned>(code_value >> 5U),
        static_cast<unsigned>(code_value & 0x1FU));
}
}

int main()
{
    using namespace castle::protocols::coap;

    client_type client;
    const castle::net::endpoint peer = castle::net::endpoint::loopback_ipv4(default_port);
    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    exchange_result result;

    CASTLE_SAMPLE_CHECK(client.set_retransmission(500U, 2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client.set_response_callback([&result](client_type::exchange_handle, castle::status status,
        client_type::message_type_t const* response, castle::net::endpoint const& source) {
        result.completed = true;
        if (status != castle::status::ok || response == nullptr || source.port() != default_port)
        {
            return;
        }
        result.received = true;
        result.code_value = response->code_value;
        result.payload_size = response->payload_size < payload_capacity ? response->payload_size : payload_capacity;
        for (castle::size_type i = 0U; i < result.payload_size; ++i)
        {
            result.payload[i] = response->payload[i];
        }
    }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client.open(peer.family()) == castle::status::ok);

    const uint8_t token[1U] = {0x31U};
    const char value[] = "30.0";
    client_type::message_type_t put;
    put.type = message_type::confirmable;
    put.code_value = static_cast<uint8_t>(code::put);
    put.message_id = 0x7101U;
    CASTLE_SAMPLE_CHECK(put.set_token(token, sizeof(token)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(append_uri_path(put, "temperature") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(put.append_uint_option(static_cast<uint16_t>(option_number::content_format), 0U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(put.set_payload(reinterpret_cast<uint8_t const*>(value), sizeof(value) - 1U) == castle::status::ok);

    if (!send_and_wait(client, peer, put, result, start))
    {
        fprintf(stderr, "No response to PUT /temperature from 127.0.0.1:%u. Start the CoAP server first.\n",
            static_cast<unsigned>(default_port));
        (void)client.close();
        return 1;
    }
    print_code("PUT /temperature \"30.0\"", result.code_value);
    const bool put_ok = result.code_value == static_cast<uint8_t>(code::created) ||
                        result.code_value == static_cast<uint8_t>(code::changed);

    const uint8_t get_token[1U] = {0x32U};
    client_type::message_type_t get;
    get.type = message_type::confirmable;
    get.code_value = static_cast<uint8_t>(code::get);
    get.message_id = 0x7102U;
    CASTLE_SAMPLE_CHECK(get.set_token(get_token, sizeof(get_token)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(append_uri_path(get, "temperature") == castle::status::ok);

    if (!send_and_wait(client, peer, get, result, start))
    {
        fprintf(stderr, "No response to GET /temperature from 127.0.0.1:%u.\n", static_cast<unsigned>(default_port));
        (void)client.close();
        return 1;
    }
    print_code("GET /temperature", result.code_value);
    const bool get_ok = result.code_value == static_cast<uint8_t>(code::content) &&
                        result.payload_size == sizeof(value) - 1U &&
                        memcmp(result.payload, value, sizeof(value) - 1U) == 0;
    if (get_ok)
    {
        printf("GET /temperature payload: %.*s\n", static_cast<int>(result.payload_size),
            reinterpret_cast<const char*>(result.payload));
    }

    (void)client.close();
    const bool all_ok = put_ok && get_ok;
    printf("%s\n", all_ok ? "Online CoAP client sample succeeded." : "Online CoAP client sample failed.");
    return all_ok ? 0 : 1;
}

// Online integration sample: start a compatible external server first, e.g.:
//   coap-server -p 5683
// Keep that server running until this sample exits; stop it with Ctrl-C.
// Then run this sample. The same server can be exercised with libcoap tools:
//   coap-client -m get coap://127.0.0.1:5683/.well-known/core
//   coap-client -m post -e hello coap://127.0.0.1:5683/test
//   coap-client -m put -e hello coap://127.0.0.1:5683/test
//   coap-client -m delete coap://127.0.0.1:5683/test
// CASTLE uses endpoint::loopback_ipv4() because this repository's endpoint class
// does not provide the string/port aggregate constructor used by some APIs.
#include "sample_support.hpp"
#include "castle/chrono/chrono.hpp"
#include "castle_ext/protocols/coap/client.hpp"

#include <stdint.h>

int main()
{
    using namespace castle::protocols::coap;
    using client_type = basic_client<1152U, 4U, 16U, 64U, 1024U, 8U, 64U>;
    client_type client;
    const castle::net::endpoint peer = castle::net::endpoint::loopback_ipv4(default_port);
    bool all_valid = true;
    size_t responses = 0U;
    uint16_t expected_ids[4U]{};
    uint8_t expected_tokens[4U]{};

    CASTLE_SAMPLE_CHECK(client.set_retransmission(500U, 2U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client.set_response_callback([&](client_type::exchange_handle handle, castle::status result,
        client_type::message_type_t const* response, castle::net::endpoint const& source) {
        if (result != castle::status::ok || response == nullptr || handle.index >= 4U)
        { all_valid = false; return; }
        if (response->token_length != 1U || response->token[0U] != expected_tokens[handle.index]) { all_valid = false; }
        if (!is_response_code(response->code_value) || source.port() != default_port) { all_valid = false; }
        if (response->type == message_type::acknowledgement && response->message_id != expected_ids[handle.index])
        { all_valid = false; }
        if (handle.index == 0U)
        {
            const auto* content_format = response->find_option(static_cast<uint16_t>(option_number::content_format));
            uint32_t parsed_format = 0U;
            if (response->code_value != static_cast<uint8_t>(code::content) || content_format == nullptr ||
                decode_uint_option(content_format->value, content_format->length, parsed_format) != castle::status::ok)
            { all_valid = false; }
        }
        for (castle::size_type i = 0U; i < response->option_count; ++i)
        {
            if (response->options[i].number == static_cast<uint16_t>(option_number::content_format))
            {
                uint32_t format = 0U;
                if (decode_uint_option(response->options[i].value, response->options[i].length, format) != castle::status::ok)
                { all_valid = false; }
            }
        }
        ++responses;
    }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client.open(peer.family()) == castle::status::ok);

    const uint8_t methods[4U] = {
        static_cast<uint8_t>(code::get), static_cast<uint8_t>(code::post),
        static_cast<uint8_t>(code::put), static_cast<uint8_t>(code::delete_)
    };
    for (castle::size_type i = 0U; i < 4U; ++i)
    {
        client_type::message_type_t request;
        request.type = (i == 0U || i == 1U) ? message_type::confirmable : message_type::non_confirmable;
        request.code_value = methods[i];
        request.message_id = static_cast<uint16_t>(0x5100U + i);
        expected_ids[i] = request.message_id;
        expected_tokens[i] = static_cast<uint8_t>(0xA0U + i);
        (void)request.set_token(&expected_tokens[i], 1U);
        (void)append_uri_path(request, ".well-known/core");
        if (i == 1U || i == 2U)
        {
            const char body[] = "castle online probe";
            (void)request.set_payload(reinterpret_cast<uint8_t const*>(body), sizeof(body) - 1U);
            (void)request.append_uint_option(static_cast<uint16_t>(option_number::content_format), 0U);
        }
        client_type::exchange_handle exchange;
        CASTLE_SAMPLE_CHECK(client.send_request(peer, request, exchange, 0U) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(exchange.index == i);
    }

    const castle::chrono::steady_clock::time_point client_start = castle::chrono::steady_clock::now();
    const castle::chrono::steady_clock::time_point deadline = client_start + castle::chrono::seconds(8);
    while (responses < 4U && castle::chrono::steady_clock::now() < deadline)
    {
        const uint32_t now_ms = static_cast<uint32_t>(castle::chrono::duration_cast<castle::chrono::milliseconds>(
            castle::chrono::steady_clock::now() - client_start).count());
        const castle::status result = client.process_once(now_ms, 50);
        if (result != castle::status::ok && result != castle::status::empty && result != castle::status::not_found)
        { all_valid = false; break; }
    }
    (void)client.close();
    return all_valid && responses == 4U ? 0 : 2;
}

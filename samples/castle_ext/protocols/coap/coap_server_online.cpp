// Online CoAP server sample. Run this executable and keep it running; press Ctrl-C to stop.
// It listens on 127.0.0.1:5683 and serves 10 topics: temperature, topic2 ... topic10.
// PUT stores a value (2.01 Created on first write, 2.04 Changed afterwards), GET returns it
// (4.04 while empty), and DELETE clears it (2.02 Deleted). From another console, try:
//   coap-client-notls -m put -e "30.0" coap://127.0.0.1/temperature
//   coap-client-notls -m get coap://127.0.0.1/temperature
//   coap-client-notls -m delete coap://127.0.0.1/temperature
// The UDP transport remembers each client source port for the whole run, so at most 64 distinct
// clients are served. Restart the server after that limit is reached.
#include "sample_support.hpp"
#include "castle/chrono/chrono.hpp"
#include "castle_ext/protocols/coap/server.hpp"

#include <signal.h>
#include <stdint.h>
#include <stdio.h>

namespace
{
using server_type = castle::protocols::coap::basic_server<1152U, 64U, 12U, 16U, 4U, 16U, 64U, 1024U, 96U>;

static CASTLE_CONSTEXPR castle::size_type topic_count = 10U;
static CASTLE_CONSTEXPR castle::size_type topic_value_capacity = 64U;

const char* const topic_paths[topic_count] = {
    "temperature", "topic2", "topic3", "topic4", "topic5",
    "topic6", "topic7", "topic8", "topic9", "topic10"
};

struct topic
{
    uint8_t value[topic_value_capacity]{};
    castle::size_type length = 0U;
    bool has_value = false;
};

volatile sig_atomic_t stop_requested = 0;
void on_interrupt(int) { stop_requested = 1; }

uint32_t elapsed_ms(castle::chrono::steady_clock::time_point start)
{
    return static_cast<uint32_t>(castle::chrono::duration_cast<castle::chrono::milliseconds>(
        castle::chrono::steady_clock::now() - start).count());
}
}

int main()
{
    using namespace castle::protocols::coap;
    signal(SIGINT, on_interrupt);

    topic topics[topic_count]{};
    server_type server;
    for (castle::size_type i = 0U; i < topic_count; ++i)
    {
        CASTLE_SAMPLE_CHECK(server.register_resource(topic_paths[i], allow_get | allow_put | allow_delete,
            [&slot = topics[i]](server_type::message_type_t const& request, server_type::message_type_t& response) {
                if (request.code_value == static_cast<uint8_t>(code::get))
                {
                    if (!slot.has_value)
                    {
                        response.code_value = static_cast<uint8_t>(code::not_found);
                        return;
                    }
                    response.code_value = static_cast<uint8_t>(code::content);
                    (void)response.append_uint_option(static_cast<uint16_t>(option_number::content_format), 0U);
                    (void)response.set_payload(slot.value, slot.length);
                }
                else if (request.code_value == static_cast<uint8_t>(code::put))
                {
                    if (request.payload_size > topic_value_capacity)
                    {
                        response.code_value = static_cast<uint8_t>(code::request_entity_too_large);
                        return;
                    }
                    response.code_value = static_cast<uint8_t>(slot.has_value ? code::changed : code::created);
                    for (castle::size_type j = 0U; j < request.payload_size; ++j)
                    {
                        slot.value[j] = request.payload[j];
                    }
                    slot.length = request.payload_size;
                    slot.has_value = true;
                }
                else
                {
                    if (!slot.has_value)
                    {
                        response.code_value = static_cast<uint8_t>(code::not_found);
                        return;
                    }
                    slot.has_value = false;
                    slot.length = 0U;
                    response.code_value = static_cast<uint8_t>(code::deleted);
                }
            }) == castle::status::ok);
    }

    CASTLE_SAMPLE_CHECK(server.open(castle::net::endpoint::loopback_ipv4(default_port)) == castle::status::ok);
    printf("CoAP server listening on 127.0.0.1:%u with %u topics. Press Ctrl-C to stop.\n",
        static_cast<unsigned>(default_port), static_cast<unsigned>(topic_count));
    fflush(stdout);

    const castle::chrono::steady_clock::time_point start = castle::chrono::steady_clock::now();
    int exit_code = 0;
    bool peer_table_full_reported = false;
    while (stop_requested == 0)
    {
        const castle::status result = server.process_once(elapsed_ms(start), 100);
        if (result == castle::status::full && !peer_table_full_reported)
        {
            fprintf(stderr, "Warning: all %u client slots are in use; new CoAP clients are dropped until restart.\n",
                static_cast<unsigned>(server_type::transport_type::client_capacity()));
            peer_table_full_reported = true;
        }
        if (stop_requested == 0 && result != castle::status::ok && result != castle::status::empty &&
            result != castle::status::full)
        {
            fprintf(stderr, "CoAP server stopped with status %d\n", static_cast<int>(result));
            exit_code = 1;
            break;
        }
    }

    (void)server.close();
    printf("CoAP server stopped.\n");
    return exit_code;
}

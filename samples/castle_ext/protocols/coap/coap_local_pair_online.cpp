// Real localhost integration: a CASTLE UDP resource server and CASTLE UDP client
// communicate in this process. Compile/link with the platform thread library
// (for example: c++ -std=c++17 -pthread ...).
#include "sample_support.hpp"
#include "castle/chrono/chrono.hpp"
#include "castle/sync/atomic.hpp"
#include "castle_ext/protocols/coap/coap.hpp"

#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <time.h>

namespace
{
using server_type = castle::protocols::coap::basic_server<512U, 4U, 4U, 12U, 2U, 12U, 48U, 256U, 64U, 96U>;
using client_type = castle::protocols::coap::basic_client<512U, 8U, 12U, 48U, 256U, 12U, 96U>;

volatile sig_atomic_t stop_requested = 0;
void on_interrupt(int) { stop_requested = 1; }
bool is_success_response(uint8_t response_code)
{ return (response_code >> 5U) == 2U; }

struct server_context
{
    server_type& server;
    castle::atomic<bool>& running;
};

void* server_main(void* arg)
{
    auto& context = *static_cast<server_context*>(arg);
    const auto start = castle::chrono::steady_clock::now();
    while (context.running.load() && stop_requested == 0)
    {
        const uint32_t now_ms = static_cast<uint32_t>(castle::chrono::duration_cast<castle::chrono::milliseconds>(
            castle::chrono::steady_clock::now() - start).count());
        const castle::status result = context.server.process_once(now_ms, 10);
        if (result != castle::status::ok && result != castle::status::empty && result != castle::status::full)
        { context.running.store(false); }
    }
    return nullptr;
}

struct client_context
{
    castle::net::endpoint server_endpoint;
    bool success;
};

void* client_main(void* arg)
{
    using namespace castle::protocols::coap;
    auto& context = *static_cast<client_context*>(arg);
    castle::net::endpoint const& actual_server = context.server_endpoint;
    bool& client_success = context.success;

    client_type client;
    size_t responses = 0U;
    uint8_t tokens[6U] = {0x21U, 0x22U, 0x23U, 0x24U, 0x25U, 0x26U};
    CASTLE_SAMPLE_CHECK(client.set_response_callback([&](client_type::exchange_handle handle, castle::status result,
        client_type::message_type_t const* response, castle::net::endpoint const& source) {
        if (result != castle::status::ok || response == nullptr || handle.index >= 6U ||
            response->token_length != 1U || response->token[0U] != tokens[handle.index] ||
            !is_response_code(response->code_value) || source.port() != actual_server.port())
        { client_success = false; return; }
        const auto* max_age_option = response->find_option(static_cast<uint16_t>(option_number::max_age));
        uint32_t max_age = 0U;
        if (max_age_option == nullptr ||
            decode_uint_option(max_age_option->value, max_age_option->length, max_age) != castle::status::ok ||
            max_age == 0U)
        { client_success = false; return; }
        if (handle.index == 5U && response->code_value != static_cast<uint8_t>(code::service_unavailable))
        { client_success = false; return; }
        if (handle.index != 5U && !is_success_response(response->code_value))
        { client_success = false; return; }
        ++responses;
    }) == castle::status::ok);
    if (client.open(actual_server.family()) != castle::status::ok) { client_success = false; return nullptr; }
    const auto client_start = castle::chrono::steady_clock::now();
    const uint8_t methods[6U] = {1U, 1U, 2U, 3U, 4U, 1U};
    const char* paths[6U] = {"sensors/temp", "sensors/temp", "actuators/led", "actuators/led", "actuators/led", "sensors/temp"};
    for (size_t i = 0U; i < 6U; ++i)
    {
        client_type::message_type_t request;
        request.type = (i == 1U || i == 3U) ? message_type::non_confirmable : message_type::confirmable;
        request.code_value = methods[i];
        request.message_id = static_cast<uint16_t>(0x6100U + i);
        (void)request.set_token(&tokens[i], 1U);
        if (append_uri_path(request, paths[i]) != castle::status::ok) { client_success = false; break; }
        if (i >= 2U)
        {
            const char payload[] = "1";
            (void)request.set_payload(reinterpret_cast<uint8_t const*>(payload), sizeof(payload) - 1U);
        }
        client_type::exchange_handle exchange;
        const uint32_t now_ms = static_cast<uint32_t>(castle::chrono::duration_cast<castle::chrono::milliseconds>(
            castle::chrono::steady_clock::now() - client_start).count());
        if (client.send_request(actual_server, request, exchange, now_ms) != castle::status::ok || exchange.index != i)
        { client_success = false; break; }
    }
    const auto deadline = castle::chrono::steady_clock::now() + castle::chrono::seconds(5);
    while (client_success && responses < 6U && castle::chrono::steady_clock::now() < deadline)
    {
        const uint32_t now_ms = static_cast<uint32_t>(castle::chrono::duration_cast<castle::chrono::milliseconds>(
            castle::chrono::steady_clock::now() - client_start).count());
        const castle::status result = client.process_once(now_ms, 10);
        if (result != castle::status::ok && result != castle::status::empty && result != castle::status::not_found)
        { client_success = false; }
    }
    if (responses != 6U) { client_success = false; }
    (void)client.close();
    return nullptr;
}
}

int main()
{
    signal(SIGINT, on_interrupt);
    using namespace castle::protocols::coap;

    server_type server;
    int led_state = 0;
    CASTLE_SAMPLE_CHECK(server.register_resource("sensors/temp", allow_get,
        [](server_type::message_type_t const&, server_type::message_type_t& response) {
            response.code_value = static_cast<uint8_t>(code::content);
            (void)response.append_uint_option(static_cast<uint16_t>(option_number::content_format), 0U);
            const char value[] = "23.75 C";
            (void)response.set_payload(reinterpret_cast<uint8_t const*>(value), sizeof(value) - 1U);
        }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server.set_resource_max_age("sensors/temp", 10U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server.register_resource("actuators/led", allow_post | allow_put | allow_delete,
        [&led_state](server_type::message_type_t const& request, server_type::message_type_t& response) {
            if (request.code_value == static_cast<uint8_t>(code::post))
            {
                led_state = request.payload_size != 0U ? 1 : 0;
                response.code_value = static_cast<uint8_t>(code::created);
                (void)response.append_string_option(static_cast<uint16_t>(option_number::location_path), "actuators");
                (void)response.append_string_option(static_cast<uint16_t>(option_number::location_path), "led");
            }
            else if (request.code_value == static_cast<uint8_t>(code::put))
            { led_state = request.payload_size != 0U && request.payload[0U] == '1' ? 1 : 0; response.code_value = static_cast<uint8_t>(code::changed); }
            else
            { led_state = 0; response.code_value = static_cast<uint8_t>(code::deleted); }
        }) == castle::status::ok);

    CASTLE_SAMPLE_CHECK(server.configure_rate_limit(5U, 60000U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server.open(castle::net::endpoint::loopback_ipv4(0U)) == castle::status::ok);
    castle::net::endpoint actual_server;
    CASTLE_SAMPLE_CHECK(server.transport().socket().local_endpoint(actual_server) == castle::status::ok);

    castle::atomic<bool> server_running{true};
    server_context server_ctx{server, server_running};
    pthread_t server_thread{};
    CASTLE_SAMPLE_CHECK(pthread_create(&server_thread, nullptr, &server_main, &server_ctx) == 0);

    client_context client_ctx{actual_server, true};
    pthread_t client_thread{};
    CASTLE_SAMPLE_CHECK(pthread_create(&client_thread, nullptr, &client_main, &client_ctx) == 0);
    CASTLE_SAMPLE_CHECK(pthread_join(client_thread, nullptr) == 0);
    // Keep the UDP server available for interactive testing until the user presses Ctrl-C.
    const timespec poll_interval{0, 50000000L};
    while (stop_requested == 0 && server_running.load())
    { (void)nanosleep(&poll_interval, nullptr); }
    server_running.store(false);
    CASTLE_SAMPLE_CHECK(pthread_join(server_thread, nullptr) == 0);
    (void)server.close();
    if (led_state != 0) { client_ctx.success = false; }
    return client_ctx.success ? 0 : 3;
}

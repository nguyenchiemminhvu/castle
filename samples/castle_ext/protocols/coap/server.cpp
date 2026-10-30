#include "sample_support.hpp"
#include "castle_ext/protocols/coap/server.hpp"

int main()
{
    using namespace castle::protocols::coap;
    basic_server<256U, 2U, 4U, 4U, 2U, 8U, 32U, 128U, 48U, 64U> server;
    int led_state = 0;
    CASTLE_SAMPLE_CHECK(server.register_resource("sensors/temp", allow_get,
        [](message<8U, 32U, 128U> const&, message<8U, 32U, 128U>& response) {
            response.code_value = static_cast<uint8_t>(code::content);
            const char value[] = "21.5";
            (void)response.set_payload(reinterpret_cast<uint8_t const*>(value), sizeof(value) - 1U);
        }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server.set_resource_max_age("sensors/temp", 10U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server.configure_rate_limit(32U, 1000U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server.register_resource("actuators/led", allow_get | allow_put,
        [&led_state](message<8U, 32U, 128U> const& request, message<8U, 32U, 128U>& response) {
            if (request.code_value == static_cast<uint8_t>(code::put)) { led_state = 1; response.code_value = static_cast<uint8_t>(code::changed); }
            else { response.code_value = static_cast<uint8_t>(code::content); }
        }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server.resource_count() == 2U);
    CASTLE_SAMPLE_CHECK(server.unregister_resource("/sensors/temp") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(server.unregister_resource("missing") == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(server.resource_count() == 1U);
    (void)led_state;
    return 0;
}

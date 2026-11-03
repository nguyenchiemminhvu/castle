#include "sample_support.hpp"
#include "castle_ext/io/rpmsg/name_service.hpp"

using namespace castle::io::rpmsg;

int main()
{
    name_service_event event;
    CASTLE_SAMPLE_CHECK(event.name.assign("sensor.service") == castle::status::ok);
    event.endpoint_address = 2048U;
    event.action = name_service_action::create;
    uint8_t wire[name_service_message_size] = {};
    CASTLE_SAMPLE_CHECK(encode_name_service_message(event, wire, sizeof(wire)) == castle::status::ok);
    name_service_event decoded;
    CASTLE_SAMPLE_CHECK(decode_name_service_message(wire, sizeof(wire), decoded) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(decoded.name == event.name && decoded.endpoint_address == 2048U);
    CASTLE_SAMPLE_CHECK(decoded.action == name_service_action::create);
    decoded.action = name_service_action::destroy;
    CASTLE_SAMPLE_CHECK(encode_name_service_message(decoded, wire, sizeof(wire)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(decode_name_service_message(wire, sizeof(wire), event) == castle::status::ok);
    wire[rpmsg_name_size + 7U] = 9U;
    CASTLE_SAMPLE_CHECK(decode_name_service_message(wire, sizeof(wire), event) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(decode_name_service_message(nullptr, sizeof(wire), event) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(decode_name_service_message(wire, sizeof(wire) - 1U, event) == castle::status::invalid_argument);
    return 0;
}

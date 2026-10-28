#include "sample_support.hpp"
#include "castle_ext/protocols/mqtt/types.hpp"

using namespace castle::protocols::mqtt;

int main()
{
    property_list<3U, 24U, 8U> properties;
    CASTLE_SAMPLE_CHECK(properties.add_integer(property_id::session_expiry_interval, 60U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(properties.add_string(property_id::reason_string, "ready") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(properties.add_user_property("node", "sensor-1") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(properties.full());
    CASTLE_SAMPLE_CHECK(properties.find(property_id::session_expiry_interval)->integer == 60U);
    CASTLE_SAMPLE_CHECK(properties.add_integer(property_id::maximum_packet_size, 512U) == castle::status::full);
    CASTLE_SAMPLE_CHECK(to_status(reason_code::success) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(to_status(reason_code::packet_too_large) == castle::status::out_of_range);
    connect_options<> options;
    CASTLE_SAMPLE_CHECK(options.protocol_version == version::v3_1_1 && options.clean_session);
    message_view message{};
    CASTLE_SAMPLE_CHECK(message.quality == qos::at_most_once && message.payload_size == 0U);
    return 0;
}

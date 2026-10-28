#include "sample_support.hpp"
#include "castle_ext/protocols/mqtt/client.hpp"

using namespace castle::protocols::mqtt;

int main()
{
    basic_client<128U, 256U, 2U, 48U, 64U> client;
    castle::size_type received = 0U;
    CASTLE_SAMPLE_CHECK(client.set_message_callback([&received](message_view const& message)
    {
        if (message.topic.size() == 4U && message.payload_size == 2U) { ++received; }
    }) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(client.publish("demo/topic", "x") == castle::status::not_configured);
    uint16_t packet_identifier = 0U;
    CASTLE_SAMPLE_CHECK(client.subscribe("demo/#", qos::at_least_once, packet_identifier) == castle::status::not_configured);
    client.clear_message_callback();
    CASTLE_SAMPLE_CHECK(!client.is_connected() && received == 0U);
    CASTLE_SAMPLE_CHECK(client.force_close() == castle::status::not_configured);
    return 0;
}

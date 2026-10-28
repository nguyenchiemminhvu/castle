#include "sample_support.hpp"
#include "castle_ext/protocols/mqtt/mqtt.hpp"

int main()
{
    castle::protocols::mqtt::client client;
    castle::protocols::mqtt::broker broker;
    CASTLE_SAMPLE_CHECK(!client.is_connected());
    CASTLE_SAMPLE_CHECK(broker.client_count() == 0U);
    CASTLE_SAMPLE_CHECK(castle::protocols::mqtt::topic_matches("plant/#", "plant/room/temperature"));
    return 0;
}

#include <gtest/gtest.h>
#include "castle_ext/protocols/mqtt/mqtt.hpp"

TEST(MqttUmbrella, ExposesClientBrokerCodecAndTopicHelpers)
{
    castle::protocols::mqtt::client client;
    castle::protocols::mqtt::broker broker;
    EXPECT_FALSE(client.is_connected());
    EXPECT_EQ(broker.client_count(), 0U);
    EXPECT_TRUE(castle::protocols::mqtt::topic_matches("telemetry/#", "telemetry/room/temp"));
}

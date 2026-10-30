#include <gtest/gtest.h>
#include "castle_ext/protocols/coap/coap.hpp"

TEST(CoapUmbrella, ExposesClientServerAndCodec)
{
    castle::protocols::coap::client client;
    castle::protocols::coap::server server;
    castle::protocols::coap::message<> message;
    message.code_value = static_cast<uint8_t>(castle::protocols::coap::code::get);
    EXPECT_FALSE(client.is_open());
    EXPECT_FALSE(server.is_open());
    EXPECT_TRUE(castle::protocols::coap::is_request_code(message.code_value));
}

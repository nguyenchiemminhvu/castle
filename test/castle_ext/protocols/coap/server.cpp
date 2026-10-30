#include <gtest/gtest.h>
#include "castle_ext/protocols/coap/server.hpp"

using namespace castle::protocols::coap;

TEST(CoapServer, RegistersResourcesAndMethodMasks)
{
    basic_server<128U, 2U, 2U, 4U, 2U, 4U, 16U, 32U, 32U, 64U> server;
    int reading = 0;
    EXPECT_EQ(server.register_resource(nullptr, allow_get, [](auto const&, auto&) {}), castle::status::invalid_argument);
    EXPECT_EQ(server.register_resource("bad", 0U, [](auto const&, auto&) {}), castle::status::invalid_argument);
    EXPECT_EQ(server.register_resource("sensors/temp", allow_get,
        [&reading](auto const&, auto& response) { ++reading; response.code_value = static_cast<uint8_t>(code::content); }), castle::status::ok);
    EXPECT_EQ(server.register_resource("actuators/led", allow_put,
        [](auto const&, auto& response) { response.code_value = static_cast<uint8_t>(code::changed); }), castle::status::ok);
    EXPECT_EQ(server.resource_count(), 2U);
    EXPECT_EQ(server.set_resource_max_age("/sensors/temp", 5U), castle::status::ok);
    EXPECT_EQ(server.set_resource_max_age("missing", 5U), castle::status::not_found);
    EXPECT_EQ(server.register_resource("actuators/led", allow_put,
        [](auto const&, auto& response) { response.code_value = static_cast<uint8_t>(code::changed); }), castle::status::ok);
    EXPECT_EQ(server.resource_count(), 2U);
    EXPECT_EQ(server.register_resource("third", allow_get, [](auto const&, auto&) {}), castle::status::full);
    EXPECT_EQ(server.unregister_resource("/sensors/temp"), castle::status::ok);
    EXPECT_EQ(server.unregister_resource("/sensors/temp"), castle::status::not_found);
    EXPECT_EQ(server.resource_count(), 1U);
    EXPECT_EQ(reading, 0);
}

TEST(CoapServer, TransportStateAndSeparateResponsePreconditions)
{
    basic_server<128U, 2U, 2U, 4U, 1U, 4U, 16U, 32U, 32U, 64U> server;
    EXPECT_FALSE(server.is_open());
    EXPECT_EQ(server.client_count(), 0U);
    EXPECT_EQ(server.configure_rate_limit(0U, 1000U), castle::status::invalid_argument);
    EXPECT_EQ(server.configure_rate_limit(2U, 0U), castle::status::invalid_argument);
    EXPECT_EQ(server.configure_rate_limit(2U, 1000U), castle::status::ok);
    server.clear_rate_limit();
    EXPECT_EQ(server.process_once(0U, 0), castle::status::not_configured);
    using message_type_t = decltype(server)::message_type_t;
    message_type_t request;
    request.type = message_type::confirmable;
    request.message_id = 3U;
    message_type_t response;
    response.code_value = static_cast<uint8_t>(code::content);
    EXPECT_EQ(server.send_separate_response(castle::net::endpoint::loopback_ipv4(default_port), request, response, 0U),
              castle::status::not_configured);
}

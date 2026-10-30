#include <gtest/gtest.h>
#include "castle_ext/protocols/coap/client.hpp"

using namespace castle::protocols::coap;
using castle::size_type;

TEST(CoapClient, ConfigurationAndEmptyState)
{
    basic_client<128U, 2U, 4U, 16U, 32U, 2U, 64U> client;
    EXPECT_FALSE(client.is_open());
    EXPECT_EQ(client.outstanding(), 0U);
    EXPECT_EQ(client.set_retransmission(0U, 1U), castle::status::invalid_argument);
    EXPECT_EQ(client.set_retransmission(250U, 9U), castle::status::invalid_argument);
    EXPECT_EQ(client.set_retransmission(500U, 2U), castle::status::ok);
    EXPECT_EQ(client.allocate_message_id(), 1U);
    client.set_next_message_id(65535U);
    EXPECT_EQ(client.allocate_message_id(), 65535U);
    EXPECT_EQ(client.allocate_message_id(), 0U);
}

TEST(CoapClient, CallbackCancellationAndClosedSend)
{
    using test_client = basic_client<128U, 2U, 4U, 16U, 32U, 2U, 64U>;
    test_client client;
    bool called = false;
    EXPECT_EQ(client.set_response_callback([&called](test_client::exchange_handle, castle::status,
        test_client::message_type_t const*, castle::net::endpoint const&) { called = true; }), castle::status::ok);
    EXPECT_EQ(client.set_response_callback(static_cast<void(*)(test_client::exchange_handle, castle::status,
        test_client::message_type_t const*, castle::net::endpoint const&)>(nullptr)), castle::status::invalid_callback);
    test_client::message_type_t request;
    request.code_value = static_cast<uint8_t>(code::get);
    test_client::exchange_handle handle;
    EXPECT_EQ(client.send_request(castle::net::endpoint::loopback_ipv4(default_port), request, handle, 0U),
              castle::status::not_configured);
    EXPECT_EQ(client.cancel(handle), castle::status::not_found);
    EXPECT_EQ(client.tick(5000U), castle::status::ok);
    EXPECT_FALSE(called);
}

TEST(CoapClient, ResponseCallbackCanReplaceItselfOnTimeout)
{
    using test_client = basic_client<128U, 2U, 4U, 16U, 32U, 2U, 64U>;
    test_client client;
    ASSERT_EQ(client.open(), castle::status::ok);
    size_type first_calls = 0U;
    size_type second_calls = 0U;
    ASSERT_EQ(client.set_response_callback([&](test_client::exchange_handle, castle::status result,
        test_client::message_type_t const*, castle::net::endpoint const&)
    {
        EXPECT_EQ(result, castle::status::time_out);
        ++first_calls;
        EXPECT_EQ(client.set_response_callback([&second_calls](test_client::exchange_handle, castle::status,
            test_client::message_type_t const*, castle::net::endpoint const&) { ++second_calls; }), castle::status::ok);
    }), castle::status::ok);

    test_client::message_type_t request;
    request.type = message_type::confirmable;
    request.code_value = static_cast<uint8_t>(code::get);
    uint32_t now = 0U;
    auto expire_next_request = [&](size_type& calls)
    {
        test_client::exchange_handle handle;
        ASSERT_EQ(client.send_request(castle::net::endpoint::loopback_ipv4(56831U), request, handle, now), castle::status::ok);
        for (uint32_t step = 0U; step < 10U && calls == 0U; ++step)
        {
            now += 100000U;
            ASSERT_EQ(client.tick(now), castle::status::ok);
        }
    };

    expire_next_request(first_calls);
    EXPECT_EQ(first_calls, 1U);
    EXPECT_EQ(second_calls, 0U);

    expire_next_request(second_calls);
    EXPECT_EQ(first_calls, 1U);
    EXPECT_EQ(second_calls, 1U);
    EXPECT_EQ(client.close(), castle::status::ok);
}

#include <gtest/gtest.h>
#include "castle_ext/io/rpmsg/name_service.hpp"

using namespace castle::io::rpmsg;

TEST(RpmsgNameService, EncodesAndDecodesCreateAndDestroy)
{
    name_service_event event;
    ASSERT_EQ(event.name.assign("service.echo"), castle::status::ok);
    event.endpoint_address = 4096U;
    event.action = name_service_action::create;
    uint8_t wire[name_service_message_size] = {};
    ASSERT_EQ(encode_name_service_message(event, wire, sizeof(wire)), castle::status::ok);
    EXPECT_EQ(wire[0], static_cast<uint8_t>('s'));
    name_service_event decoded;
    ASSERT_EQ(decode_name_service_message(wire, sizeof(wire), decoded), castle::status::ok);
    EXPECT_TRUE(decoded.name == event.name);
    EXPECT_EQ(decoded.endpoint_address, 4096U);
    EXPECT_EQ(decoded.action, name_service_action::create);
    decoded.action = name_service_action::destroy;
    ASSERT_EQ(encode_name_service_message(decoded, wire, sizeof(wire)), castle::status::ok);
    ASSERT_EQ(decode_name_service_message(wire, sizeof(wire), event), castle::status::ok);
    EXPECT_EQ(event.action, name_service_action::destroy);
}

TEST(RpmsgNameService, RejectsMalformedInput)
{
    name_service_event out;
    uint8_t wire[name_service_message_size] = {};
    EXPECT_EQ(decode_name_service_message(nullptr, sizeof(wire), out), castle::status::invalid_argument);
    EXPECT_EQ(decode_name_service_message(wire, sizeof(wire) - 1U, out), castle::status::invalid_argument);
    EXPECT_EQ(decode_name_service_message(wire, sizeof(wire), out), castle::status::invalid_argument); // empty name
    ASSERT_EQ(encode_name_service_message(name_service_event(), wire, sizeof(wire)), castle::status::invalid_argument);
    name_service_event valid;
    ASSERT_EQ(valid.name.assign("x"), castle::status::ok);
    valid.endpoint_address = 1024U;
    EXPECT_EQ(encode_name_service_message(valid, wire, name_service_message_size - 1U), castle::status::out_of_range);
    EXPECT_EQ(encode_name_service_message(valid, nullptr, sizeof(wire)), castle::status::invalid_argument);
    valid.endpoint_address = 9U;
    EXPECT_EQ(encode_name_service_message(valid, wire, sizeof(wire)), castle::status::invalid_argument);
    valid.endpoint_address = rpmsg_addr_any;
    EXPECT_EQ(encode_name_service_message(valid, wire, sizeof(wire)), castle::status::invalid_argument);
    valid.endpoint_address = 1024U;
    valid.action = static_cast<name_service_action>(99U);
    EXPECT_EQ(encode_name_service_message(valid, wire, sizeof(wire)), castle::status::invalid_argument);
    ASSERT_EQ(valid.name.assign("x"), castle::status::ok);
    valid.action = name_service_action::create;
    ASSERT_EQ(encode_name_service_message(valid, wire, sizeof(wire)), castle::status::ok);
    wire[rpmsg_name_size + 7U] = 0x7FU;
    EXPECT_EQ(decode_name_service_message(wire, sizeof(wire), out), castle::status::invalid_argument);
    memset(wire, 'a', rpmsg_name_size);
    EXPECT_EQ(decode_name_service_message(wire, sizeof(wire), out), castle::status::invalid_argument); // no NUL
}

TEST(RpmsgNameService, RejectsReservedAddressAndUnknownFlags)
{
    name_service_event event;
    ASSERT_EQ(event.name.assign("svc"), castle::status::ok);
    event.endpoint_address = 4096U;
    uint8_t wire[name_service_message_size] = {};
    ASSERT_EQ(encode_name_service_message(event, wire, sizeof(wire)), castle::status::ok);
    name_service_event out;
    const uint32_t reserved_address = 35U;
    memcpy(wire + rpmsg_name_size, &reserved_address, sizeof(reserved_address));
    EXPECT_EQ(decode_name_service_message(wire, sizeof(wire), out), castle::status::invalid_argument);
    const uint32_t any_address = rpmsg_addr_any;
    memcpy(wire + rpmsg_name_size, &any_address, sizeof(any_address));
    EXPECT_EQ(decode_name_service_message(wire, sizeof(wire), out), castle::status::invalid_argument);
    const uint32_t valid_address = 4096U;
    memcpy(wire + rpmsg_name_size, &valid_address, sizeof(valid_address));
    const uint32_t unknown_flags = 2U;
    memcpy(wire + rpmsg_name_size + sizeof(valid_address), &unknown_flags, sizeof(unknown_flags));
    EXPECT_EQ(decode_name_service_message(wire, sizeof(wire), out), castle::status::invalid_argument);
    const uint32_t destroy_flags = 1U;
    memcpy(wire + rpmsg_name_size + sizeof(valid_address), &destroy_flags, sizeof(destroy_flags));
    ASSERT_EQ(decode_name_service_message(wire, sizeof(wire), out), castle::status::ok);
    EXPECT_EQ(out.endpoint_address, 4096U);
    EXPECT_EQ(out.action, name_service_action::destroy);
}


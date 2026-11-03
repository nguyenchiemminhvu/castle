#include <gtest/gtest.h>
#include "castle_ext/io/rpmsg/message.hpp"

using namespace castle::io::rpmsg;

TEST(RpmsgMessage, ViewRetainsNonOwningMetadata)
{
    const uint8_t data[] = {1U, 2U, 3U};
    message_view view(0x1001U, 0x1002U, data, sizeof(data));
    EXPECT_FALSE(view.empty());
    EXPECT_EQ(view.source, 0x1001U);
    EXPECT_EQ(view.destination, 0x1002U);
    EXPECT_EQ(view.data, data);
    EXPECT_EQ(view.size, 3U);
    message_view empty;
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.data, nullptr);
}

TEST(RpmsgMessage, TxBufferResetClearsAllFields)
{
    uint8_t bytes[8] = {};
    tx_buffer buffer;
    buffer.data = bytes;
    buffer.capacity = sizeof(bytes);
    buffer.endpoint_token = bytes;
    buffer.active = true;
    buffer.reset();
    EXPECT_FALSE(buffer.active);
    EXPECT_EQ(buffer.data, nullptr);
    EXPECT_EQ(buffer.capacity, 0U);
    EXPECT_EQ(buffer.endpoint_token, nullptr);
}


TEST(RpmsgMessage, TxBufferMovesOwnership)
{
    tx_buffer original;
    uint8_t storage[4U] = {};
    original.data = storage;
    original.capacity = sizeof(storage);
    original.endpoint_token = &storage;
    original.active = true;
    tx_buffer moved(castle::move(original));
    EXPECT_FALSE(original.active);
    EXPECT_EQ(original.data, nullptr);
    EXPECT_TRUE(moved.active);
    EXPECT_EQ(moved.data, storage);
    moved.reset();
    EXPECT_FALSE(moved.active);
}

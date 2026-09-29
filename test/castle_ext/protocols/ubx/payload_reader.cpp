#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/payload_reader.hpp"

namespace
{

using namespace castle_ext::protocols::ubx;
using castle::container::array_view;

TEST(PayloadReader, ReadsAllLittleEndianWidthsAndAdvances)
{
    uint8_t payload[32U] = {};
    write_le16(&payload[0U], 0x1234U);
    write_le16(&payload[2U], 0xFFFEU);
    write_le32(&payload[4U], 0x89ABCDEFU);
    write_le32(&payload[8U], 0xFFFFFFFEU);
    write_le64(&payload[12U], 0x0123456789ABCDEFULL);
    write_le64(&payload[20U], 0xFFFFFFFFFFFFFFFEULL);
    payload[28U] = 0x7EU;
    payload[29U] = 0x80U;
    payload[30U] = 0x81U;
    payload[31U] = 0x82U;

    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    EXPECT_EQ(reader.position(), 0U);
    EXPECT_EQ(reader.remaining(), sizeof(payload));
    EXPECT_EQ(reader.read_u16(), 0x1234U);
    EXPECT_EQ(reader.read_i16(), -2);
    EXPECT_EQ(reader.read_u32(), 0x89ABCDEFU);
    EXPECT_EQ(reader.read_i32(), -2);
    EXPECT_EQ(reader.read_u64(), 0x0123456789ABCDEFULL);
    EXPECT_EQ(reader.read_i64(), -2);
    EXPECT_EQ(reader.read_u8(), 0x7EU);
    EXPECT_EQ(reader.read_i8(), static_cast<int8_t>(0x80U));
    const uint8_t* bytes = reader.read_bytes(2U);
    ASSERT_NE(bytes, nullptr);
    EXPECT_EQ(bytes[0U], 0x81U);
    EXPECT_EQ(bytes[1U], 0x82U);
    EXPECT_EQ(reader.position(), sizeof(payload));
    EXPECT_EQ(reader.remaining(), 0U);
    EXPECT_TRUE(reader.ok());
}

TEST(PayloadReader, FailedReadLatchesErrorAndPreventsFurtherAdvancement)
{
    uint8_t payload[1U] = {0xAAU};
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));

    EXPECT_EQ(reader.read_u16(), 0U);
    EXPECT_FALSE(reader.ok());
    EXPECT_EQ(reader.position(), 0U);
    EXPECT_EQ(reader.remaining(), 1U);
    EXPECT_FALSE(reader.skip(0U));
    EXPECT_EQ(reader.read_u8(), 0U);
    EXPECT_EQ(reader.position(), 0U);
}

TEST(PayloadReader, ExactSkipAndOverskipAreDeterministic)
{
    uint8_t payload[4U] = {1U, 2U, 3U, 4U};
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    EXPECT_TRUE(reader.skip(4U));
    EXPECT_EQ(reader.position(), 4U);
    EXPECT_EQ(reader.remaining(), 0U);
    EXPECT_FALSE(reader.skip(1U));
    EXPECT_FALSE(reader.ok());
}

} // namespace

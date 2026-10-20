#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/detail/esf_data.hpp"
#include "castle/utility/bytes.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

uint32_t read_one(CASTLE_CONST uint8_t* bytes)
{
    return castle::read_le32(bytes);
}

TEST(UbxMessageDetailEsfData, ConstantsDescribeTheWordLayout)
{
    EXPECT_EQ(detail::ESF_DATA_FIELD_MASK, 0x00FFFFFFU);
    EXPECT_EQ(detail::ESF_DATA_FIELD_SIGN, 0x00800000U);
    EXPECT_EQ(detail::ESF_DATA_TYPE_SHIFT, 24U);
    EXPECT_EQ(detail::ESF_DATA_FIELD_MIN, -8388608);
    EXPECT_EQ(detail::ESF_DATA_FIELD_MAX, 8388607);
}

TEST(UbxMessageDetailEsfData, DataFieldSignExtendsTwentyFourBits)
{
    EXPECT_EQ(detail::esf_data_field(0x00000000U), 0);
    EXPECT_EQ(detail::esf_data_field(0x00000001U), 1);
    EXPECT_EQ(detail::esf_data_field(0x007FFFFFU), 8388607);
    EXPECT_EQ(detail::esf_data_field(0x00800000U), -8388608);
    EXPECT_EQ(detail::esf_data_field(0x00800001U), -8388607);
    EXPECT_EQ(detail::esf_data_field(0x00FFFFFFU), -1);
    // Type bits never leak into the value.
    EXPECT_EQ(detail::esf_data_field(0xFF7FFFFFU), 8388607);
    EXPECT_EQ(detail::esf_data_field(0xFF800000U), -8388608);
    EXPECT_EQ(detail::esf_data_field(0x3FFFFFFFU), -1);
    static_assert(detail::esf_data_field(0x00FFFFFFU) == -1, "esf_data_field is usable in constant expressions");
}

TEST(UbxMessageDetailEsfData, ReadWithSixBitMaskIgnoresBitsAboveTheMask)
{
    uint8_t payload[12U] = {};
    castle::write_le32(&payload[0U], 0xC0000000U | (63U << 24U) | 0x007FFFFFU); // bits 31..30 set
    castle::write_le32(&payload[4U], (5U << 24U) | 0x00800000U);
    castle::write_le32(&payload[8U], 0xFF000001U); // dataType 0x3F after masking

    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    uint8_t type = 0xAAU;
    int32_t value = 0x55;
    detail::esf_data_word<0x3FU>(reader, type, value);
    EXPECT_EQ(type, 63U);
    EXPECT_EQ(value, 8388607);
    detail::esf_data_word<0x3FU>(reader, type, value);
    EXPECT_EQ(type, 5U);
    EXPECT_EQ(value, -8388608);
    detail::esf_data_word<0x3FU>(reader, type, value);
    EXPECT_EQ(type, 0x3FU);
    EXPECT_EQ(value, 1);
    EXPECT_TRUE(reader.ok());
    EXPECT_EQ(reader.remaining(), 0U);
}

TEST(UbxMessageDetailEsfData, ReadWithEightBitMaskKeepsFullType)
{
    uint8_t payload[4U] = {};
    castle::write_le32(&payload[0U], 0xFEFFFFFFU);
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    uint8_t type = 0U;
    int32_t value = 0;
    detail::esf_data_word<0xFFU>(reader, type, value);
    EXPECT_EQ(type, 0xFEU);
    EXPECT_EQ(value, -1);
    EXPECT_TRUE(reader.ok());
}

TEST(UbxMessageDetailEsfData, ReadPastEndLatchesReaderFailure)
{
    uint8_t payload[3U] = {0xFFU, 0xFFU, 0xFFU};
    payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
    uint8_t type = 9U;
    int32_t value = 9;
    detail::esf_data_word<0xFFU>(reader, type, value);
    EXPECT_FALSE(reader.ok());
    EXPECT_EQ(type, 0U);
    EXPECT_EQ(value, 0);
}

TEST(UbxMessageDetailEsfData, WriteEncodesTypeAndTwosComplementField)
{
    uint8_t payload[16U] = {};
    payload_writer writer(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    detail::esf_data_word<0x3FU>(writer, static_cast<uint8_t>(63U), static_cast<int32_t>(-8388608));
    detail::esf_data_word<0x3FU>(writer, static_cast<uint8_t>(11U), static_cast<int32_t>(8388607));
    detail::esf_data_word<0xFFU>(writer, static_cast<uint8_t>(0xFFU), static_cast<int32_t>(-1));
    detail::esf_data_word<0xFFU>(writer, static_cast<uint8_t>(0U), static_cast<int32_t>(0));
    ASSERT_TRUE(writer.ok());
    EXPECT_EQ(writer.size(), 16U);
    EXPECT_EQ(read_one(&payload[0U]), 0x3F800000U);
    EXPECT_EQ(read_one(&payload[4U]), 0x0B7FFFFFU);
    EXPECT_EQ(read_one(&payload[8U]), 0xFFFFFFFFU);
    EXPECT_EQ(read_one(&payload[12U]), 0x00000000U);
}

TEST(UbxMessageDetailEsfData, WriteThenReadRoundTripsEdgeValues)
{
    CASTLE_CONST int32_t values[] = {-8388608, -8388607, -1, 0, 1, 8388606, 8388607};
    for (CASTLE_CONST int32_t original : values)
    {
        uint8_t payload[4U] = {};
        payload_writer writer(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
        detail::esf_data_word<0xFFU>(writer, static_cast<uint8_t>(0x81U), original);
        ASSERT_TRUE(writer.ok());

        payload_reader reader(array_view<CASTLE_CONST uint8_t>(payload, sizeof(payload)));
        uint8_t type = 0U;
        int32_t value = 0;
        detail::esf_data_word<0xFFU>(reader, type, value);
        EXPECT_EQ(type, 0x81U);
        EXPECT_EQ(value, original);
    }
}

TEST(UbxMessageDetailEsfData, WriteRejectsTypeWiderThanMask)
{
    uint8_t payload[8U] = {};
    payload_writer writer(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    detail::esf_data_word<0x3FU>(writer, static_cast<uint8_t>(0x40U), static_cast<int32_t>(0));
    EXPECT_EQ(writer.status(), castle::status::out_of_range);
    EXPECT_EQ(writer.size(), 0U);
    // The failure is sticky: a later valid word is not written.
    detail::esf_data_word<0x3FU>(writer, static_cast<uint8_t>(1U), static_cast<int32_t>(1));
    EXPECT_EQ(writer.status(), castle::status::out_of_range);
    EXPECT_EQ(writer.size(), 0U);

    payload_writer wide(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    detail::esf_data_word<0xFFU>(wide, static_cast<uint8_t>(0x40U), static_cast<int32_t>(0));
    EXPECT_TRUE(wide.ok());
    EXPECT_EQ(read_one(&payload[0U]), 0x40000000U);
}

TEST(UbxMessageDetailEsfData, WriteRejectsValuesOutsideSignedTwentyFourBits)
{
    uint8_t payload[4U] = {};
    payload_writer above(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    detail::esf_data_word<0xFFU>(above, static_cast<uint8_t>(1U), static_cast<int32_t>(8388608));
    EXPECT_EQ(above.status(), castle::status::out_of_range);
    EXPECT_EQ(above.size(), 0U);

    payload_writer below(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    detail::esf_data_word<0xFFU>(below, static_cast<uint8_t>(1U), static_cast<int32_t>(-8388609));
    EXPECT_EQ(below.status(), castle::status::out_of_range);
    EXPECT_EQ(below.size(), 0U);

    payload_writer six_bit(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    detail::esf_data_word<0x3FU>(six_bit, static_cast<uint8_t>(1U), static_cast<int32_t>(0x7FFFFFFF));
    EXPECT_EQ(six_bit.status(), castle::status::out_of_range);
}

TEST(UbxMessageDetailEsfData, WriteIntoFullBufferReportsFull)
{
    uint8_t payload[3U] = {};
    payload_writer writer(castle::container::array_view<uint8_t>(payload, sizeof(payload)));
    detail::esf_data_word<0x3FU>(writer, static_cast<uint8_t>(1U), static_cast<int32_t>(1));
    EXPECT_EQ(writer.status(), castle::status::full);
}

} // namespace

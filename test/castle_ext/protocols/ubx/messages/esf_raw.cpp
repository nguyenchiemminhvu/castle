#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/esf_raw.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

esf_raw_sample sample(uint8_t type, int32_t value, uint32_t s_ttag)
{
    esf_raw_sample result{};
    result.data_type = type;
    result.data_value = value;
    result.s_ttag = s_ttag;
    return result;
}

TEST(UbxMessageEsfRaw, ConstantsMatchSpec)
{
    EXPECT_EQ(esf_raw<>::msg_class, 0x10U);
    EXPECT_EQ(esf_raw<>::msg_id, 0x03U);
    EXPECT_EQ(esf_raw<>::header_length, 4U);
    EXPECT_EQ(esf_raw<>::block_length, 8U);
    EXPECT_EQ(esf_raw_sample::DATA_TYPE_MASK, 0xFFU);
}

TEST(UbxMessageEsfRaw, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[20U] = {};
    castle::write_le32(&payload[0U], 0xFFFFFFFFU); // reserved0 is ignored
    castle::write_le32(&payload[4U], (0xFEU << 24U) | 0x00FFFFF0U); // 8-bit dataType, negative value
    castle::write_le32(&payload[8U], 0x00010203U);
    castle::write_le32(&payload[12U], (14U << 24U) | 0x007FFFFFU);
    castle::write_le32(&payload[16U], 0x00FFFFFFU);

    message_view raw = make_view(UBX_CLASS_ESF, UBX_ID_ESF_RAW, payload, 20U);
    esf_raw<4U> out;
    ASSERT_TRUE(esf_raw<4U>::matches(raw));
    ASSERT_TRUE(esf_raw<4U>::decode(raw, out));
    EXPECT_EQ(out.num_samples, 2U);
    ASSERT_EQ(out.samples.size(), 2U);
    EXPECT_EQ(out.samples[0U].data_type, 0xFEU);
    EXPECT_EQ(out.samples[0U].data_value, -16);
    EXPECT_EQ(out.samples[0U].s_ttag, 0x00010203U);
    EXPECT_EQ(out.samples[1U].data_type, 14U);
    EXPECT_EQ(out.samples[1U].data_value, 0x7FFFFF);
    EXPECT_EQ(out.samples[1U].s_ttag, 0x00FFFFFFU);
}

TEST(UbxMessageEsfRaw, RoundTripsEverySampleField)
{
    esf_raw<3U> in{};
    ASSERT_EQ(in.samples.push_back(sample(5U, -0x800000, 0x11111111U)), castle::status::ok);
    ASSERT_EQ(in.samples.push_back(sample(0xFFU, 0x7FFFFF, 0x22222222U)), castle::status::ok);
    ASSERT_EQ(in.samples.push_back(sample(13U, -2, 0x33333333U)), castle::status::ok);
    in.num_samples = 99U; // not written; the count is implied by the length

    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(4U + (3U * 8U)));
    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(castle::read_le32(&payload[0U]), 0U);
    EXPECT_EQ(castle::read_le32(&payload[4U]), (5U << 24U) | 0x00800000U);
    EXPECT_EQ(castle::read_le32(&payload[12U]), 0xFF7FFFFFU);
    EXPECT_EQ(castle::read_le32(&payload[20U]), (13U << 24U) | 0x00FFFFFEU);

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    esf_raw<3U> out;
    ASSERT_TRUE(esf_raw<3U>::decode(raw, out));
    EXPECT_EQ(out.num_samples, 3U);
    ASSERT_EQ(out.samples.size(), 3U);
    for (castle::size_type index = 0U; index < 3U; ++index)
    {
        EXPECT_EQ(out.samples[index].data_type, in.samples[index].data_type);
        EXPECT_EQ(out.samples[index].data_value, in.samples[index].data_value);
        EXPECT_EQ(out.samples[index].s_ttag, in.samples[index].s_ttag);
    }
}

TEST(UbxMessageEsfRaw, EmptyGroupRoundTripsThroughFrame)
{
    esf_raw<2U> in{};
    uint8_t frame[16U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, frame_size(4U));
    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    esf_raw<2U> out;
    out.num_samples = 4U;
    ASSERT_TRUE(esf_raw<2U>::decode(raw, out));
    EXPECT_EQ(out.num_samples, 0U);
    EXPECT_TRUE(out.samples.empty());
}

TEST(UbxMessageEsfRaw, ClampsStoredSamplesButReportsWireCount)
{
    uint8_t payload[28U] = {};
    castle::write_le32(&payload[4U], (1U << 24U) | 1U);
    castle::write_le32(&payload[8U], 10U);
    castle::write_le32(&payload[12U], (2U << 24U) | 2U);
    castle::write_le32(&payload[20U], (3U << 24U) | 3U);
    esf_raw<1U> out;
    ASSERT_TRUE(esf_raw<1U>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_RAW, payload, 28U), out));
    EXPECT_EQ(out.num_samples, 3U);
    ASSERT_EQ(out.samples.size(), 1U);
    EXPECT_EQ(out.samples[0U].data_type, 1U);
    EXPECT_EQ(out.samples[0U].data_value, 1);
    EXPECT_EQ(out.samples[0U].s_ttag, 10U);
}

TEST(UbxMessageEsfRaw, RejectsLengthThatIsNotHeaderPlusWholeBlocks)
{
    uint8_t payload[24U] = {};
    esf_raw<> out;
    out.num_samples = 7U;
    EXPECT_FALSE(esf_raw<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_RAW, payload, 3U), out));
    EXPECT_EQ(out.num_samples, 0U);
    EXPECT_FALSE(esf_raw<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_RAW, payload, 0U), out));
    EXPECT_FALSE(esf_raw<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_RAW, payload, 5U), out));
    EXPECT_FALSE(esf_raw<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_RAW, payload, 11U), out));
    EXPECT_FALSE(esf_raw<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_RAW, payload, 13U), out));
    EXPECT_TRUE(esf_raw<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_RAW, payload, 12U), out));
    EXPECT_EQ(out.num_samples, 1U);
    EXPECT_TRUE(esf_raw<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_RAW, payload, 4U), out));
    EXPECT_EQ(out.num_samples, 0U);
}

TEST(UbxMessageEsfRaw, ResetsOutputWhenPayloadViewIsShorterThanDeclaredLength)
{
    uint8_t payload[12U] = {};
    message_view raw{{UBX_CLASS_ESF, UBX_ID_ESF_RAW, 12U}, array_view<CASTLE_CONST uint8_t>(payload, 8U), checksum{}};
    esf_raw<> out;
    EXPECT_FALSE(esf_raw<>::decode(raw, out));
    EXPECT_EQ(out.num_samples, 0U);
    EXPECT_TRUE(out.samples.empty());
}

TEST(UbxMessageEsfRaw, RejectsWrongClassAndId)
{
    uint8_t payload[4U] = {};
    esf_raw<> out;
    EXPECT_FALSE(esf_raw<>::matches(make_view(UBX_CLASS_NAV, UBX_ID_ESF_RAW, payload, 4U)));
    EXPECT_FALSE(esf_raw<>::decode(make_view(UBX_CLASS_NAV, UBX_ID_ESF_RAW, payload, 4U), out));
    EXPECT_FALSE(esf_raw<>::matches(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 4U)));
    EXPECT_FALSE(esf_raw<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 4U), out));
}

TEST(UbxMessageEsfRaw, EncodeRejectsOutOfRangeDataField)
{
    esf_raw<1U> in{};
    ASSERT_EQ(in.samples.push_back(sample(1U, 0x01000000, 0U)), castle::status::ok);
    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    EXPECT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::out_of_range);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageEsfRaw, EncodeIntoTooSmallBufferReportsFull)
{
    esf_raw<1U> in{};
    ASSERT_EQ(in.samples.push_back(sample(1U, 1, 2U)), castle::status::ok);
    uint8_t frame[frame_size(12U)] = {};
    castle::size_type written = 1U;
    EXPECT_EQ(encode_message(in, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
}

} // namespace

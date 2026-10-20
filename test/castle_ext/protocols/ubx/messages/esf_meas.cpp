#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/esf_meas.hpp"
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

uint16_t flags_with_count(uint16_t bits, uint16_t count)
{
    return static_cast<uint16_t>(bits | (count << 11U));
}

esf_meas_datum datum(uint8_t type, int32_t value)
{
    esf_meas_datum result{};
    result.data_type = type;
    result.data_value = value;
    return result;
}

TEST(UbxMessageEsfMeas, FlagMasksMatchSpecBitPositions)
{
    // The old FLAGS_TIME_TAG_TYPE_MASK (0x0003) gated calibTtag; the spec uses bit 3.
    EXPECT_EQ(esf_meas<>::FLAGS_TIME_MARK_SENT, 0x0003U);
    EXPECT_EQ(esf_meas<>::FLAGS_TIME_MARK_EDGE, 0x0004U);
    EXPECT_EQ(esf_meas<>::FLAGS_CALIB_TTAG_VALID, 0x0008U);
    EXPECT_EQ(esf_meas<>::FLAGS_NUM_MEAS, 0xF800U);
    EXPECT_EQ(esf_meas_datum::DATA_TYPE_MASK, 0x3FU);
    EXPECT_EQ(esf_meas<>::msg_class, 0x10U);
    EXPECT_EQ(esf_meas<>::msg_id, 0x02U);
    EXPECT_EQ(esf_meas<>::header_length, 8U);
    EXPECT_EQ(esf_meas<>::block_length, 4U);
}

TEST(UbxMessageEsfMeas, SignExtendsTwentyFourBitDataField)
{
    EXPECT_EQ(detail::esf_data_field(0x00000000U), 0);
    EXPECT_EQ(detail::esf_data_field(0x00000001U), 1);
    EXPECT_EQ(detail::esf_data_field(0x007FFFFFU), 0x7FFFFF);
    EXPECT_EQ(detail::esf_data_field(0x00800000U), -0x800000);
    EXPECT_EQ(detail::esf_data_field(0x00800001U), -0x7FFFFF);
    EXPECT_EQ(detail::esf_data_field(0x00FFFFFFU), -1);
    EXPECT_EQ(detail::esf_data_field(0xFF000005U), 5);  // type bits never leak into the value
    EXPECT_EQ(detail::esf_data_field(0x3FFFFFFEU), -2);
}

TEST(UbxMessageEsfMeas, DecodesHandBuiltPayloadWithCalibTtagAtSpecOffsets)
{
    uint8_t payload[20U] = {};
    castle::write_le32(&payload[0U], 0x01020304U);
    castle::write_le16(&payload[4U], flags_with_count(0x000EU, 2U)); // EXT1, falling edge, calibTtagValid
    castle::write_le16(&payload[6U], 0xBEEFU);
    castle::write_le32(&payload[8U], (5U << 24U) | 0x00800001U);
    castle::write_le32(&payload[12U], 0xC0000000U | (63U << 24U) | 0x00123456U); // bits 31..30 are not dataType
    castle::write_le32(&payload[16U], 0x55667788U);

    message_view raw = make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 20U);
    esf_meas<8U> out;
    ASSERT_TRUE(esf_meas<8U>::matches(raw));
    ASSERT_TRUE(esf_meas<8U>::decode(raw, out));
    EXPECT_EQ(out.time_tag, 0x01020304U);
    EXPECT_EQ(out.flags, flags_with_count(0x000EU, 2U));
    EXPECT_EQ(out.id, 0xBEEFU);
    EXPECT_EQ(out.num_meas(), 2U);
    ASSERT_EQ(out.data.size(), 2U);
    EXPECT_EQ(out.data[0U].data_type, 5U);
    EXPECT_EQ(out.data[0U].data_value, -0x7FFFFF);
    EXPECT_EQ(out.data[1U].data_type, 63U);
    EXPECT_EQ(out.data[1U].data_value, 0x123456);
    EXPECT_EQ(out.time_mark_sent(), esf_meas<8U>::TIME_MARK_EXT1);
    EXPECT_TRUE(out.time_mark_edge());
    EXPECT_TRUE(out.calib_ttag_valid());
    EXPECT_EQ(out.calib_ttag, 0x55667788U);
}

TEST(UbxMessageEsfMeas, TimeMarkSentDoesNotImplyCalibTtag)
{
    // Corrected expectation: the old decoder treated timeMarkSent (bits 1..0) as the calibTtag flag.
    uint8_t payload[16U] = {};
    castle::write_le32(&payload[0U], 123U);
    castle::write_le16(&payload[4U], flags_with_count(esf_meas<>::FLAGS_TIME_MARK_SENT, 1U));
    castle::write_le16(&payload[6U], 7U);
    castle::write_le32(&payload[8U], (5U << 24U) | 0x00800001U);
    castle::write_le32(&payload[12U], 0x55667788U);

    esf_meas<8U> out;
    EXPECT_FALSE(esf_meas<8U>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 16U), out));
    EXPECT_EQ(out.time_tag, 0U);
    EXPECT_TRUE(out.data.empty());

    ASSERT_TRUE(esf_meas<8U>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 12U), out));
    EXPECT_EQ(out.time_tag, 123U);
    EXPECT_EQ(out.id, 7U);
    EXPECT_EQ(out.time_mark_sent(), 3U);
    EXPECT_FALSE(out.time_mark_edge());
    EXPECT_FALSE(out.calib_ttag_valid());
    EXPECT_EQ(out.calib_ttag, 0U);
    ASSERT_EQ(out.data.size(), 1U);
    EXPECT_EQ(out.data[0U].data_type, 5U);
    EXPECT_EQ(out.data[0U].data_value, -0x7FFFFF);
}

TEST(UbxMessageEsfMeas, AccessorsSetAndClear)
{
    esf_meas<> value{};
    EXPECT_EQ(value.time_mark_sent(), esf_meas<>::TIME_MARK_NONE);
    EXPECT_FALSE(value.time_mark_edge());
    EXPECT_FALSE(value.calib_ttag_valid());
    EXPECT_EQ(value.num_meas(), 0U);
    value.flags = esf_meas<>::TIME_MARK_EXT0;
    EXPECT_EQ(value.time_mark_sent(), esf_meas<>::TIME_MARK_EXT0);
    value.flags = esf_meas<>::FLAGS_TIME_MARK_EDGE;
    EXPECT_TRUE(value.time_mark_edge());
    EXPECT_EQ(value.time_mark_sent(), 0U);
    value.flags = esf_meas<>::FLAGS_CALIB_TTAG_VALID;
    EXPECT_TRUE(value.calib_ttag_valid());
    EXPECT_FALSE(value.time_mark_edge());
    value.flags = esf_meas<>::FLAGS_NUM_MEAS;
    EXPECT_EQ(value.num_meas(), 31U);
    EXPECT_FALSE(value.calib_ttag_valid());
    EXPECT_EQ(value.time_mark_sent(), 0U);
    value.flags = 0x07F0U; // unused bits 10..4 touch nothing
    EXPECT_EQ(value.num_meas(), 0U);
    EXPECT_FALSE(value.calib_ttag_valid());
    EXPECT_FALSE(value.time_mark_edge());
}

TEST(UbxMessageEsfMeas, RoundTripsInputMessageWithCountFromSize)
{
    esf_meas<4U> in{};
    in.time_tag = 0xCAFEBABEU;
    in.flags = flags_with_count(static_cast<uint16_t>(esf_meas<4U>::TIME_MARK_EXT0 | esf_meas<4U>::FLAGS_TIME_MARK_EDGE
                                                      | esf_meas<4U>::FLAGS_CALIB_TTAG_VALID), 17U); // stale count
    in.id = 0x1234U;
    in.calib_ttag = 0x0BADF00DU;
    ASSERT_EQ(in.data.push_back(datum(11U, -0x800000)), castle::status::ok);
    ASSERT_EQ(in.data.push_back(datum(63U, 0x7FFFFF)), castle::status::ok);
    ASSERT_EQ(in.data.push_back(datum(8U, -1)), castle::status::ok);

    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(8U + (3U * 4U) + 4U));

    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(castle::read_le16(&payload[4U]), flags_with_count(0x000DU, 3U));
    EXPECT_EQ(castle::read_le32(&payload[8U]), (11U << 24U) | 0x00800000U);
    EXPECT_EQ(castle::read_le32(&payload[12U]), (63U << 24U) | 0x007FFFFFU);
    EXPECT_EQ(castle::read_le32(&payload[16U]), (8U << 24U) | 0x00FFFFFFU);
    EXPECT_EQ(castle::read_le32(&payload[20U]), 0x0BADF00DU);

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    esf_meas<4U> out;
    ASSERT_TRUE(esf_meas<4U>::decode(raw, out));
    EXPECT_EQ(out.time_tag, in.time_tag);
    EXPECT_EQ(out.id, in.id);
    EXPECT_EQ(out.num_meas(), 3U);
    EXPECT_EQ(out.time_mark_sent(), esf_meas<4U>::TIME_MARK_EXT0);
    EXPECT_TRUE(out.time_mark_edge());
    EXPECT_TRUE(out.calib_ttag_valid());
    EXPECT_EQ(out.calib_ttag, in.calib_ttag);
    ASSERT_EQ(out.data.size(), 3U);
    for (castle::size_type index = 0U; index < 3U; ++index)
    {
        EXPECT_EQ(out.data[index].data_type, in.data[index].data_type);
        EXPECT_EQ(out.data[index].data_value, in.data[index].data_value);
    }
}

TEST(UbxMessageEsfMeas, EncodeOmitsCalibTtagUnlessValid)
{
    esf_meas<2U> in{};
    in.calib_ttag = 0xFFFFFFFFU; // ignored: calibTtagValid clear
    ASSERT_EQ(in.data.push_back(datum(10U, 42)), castle::status::ok);

    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, frame_size(12U));

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    esf_meas<2U> out;
    ASSERT_TRUE(esf_meas<2U>::decode(raw, out));
    EXPECT_EQ(out.num_meas(), 1U);
    EXPECT_FALSE(out.calib_ttag_valid());
    EXPECT_EQ(out.calib_ttag, 0U);
    EXPECT_EQ(out.data[0U].data_value, 42);
}

TEST(UbxMessageEsfMeas, DecodesHeaderOnlyPayloadsWithAndWithoutCalib)
{
    uint8_t payload[12U] = {};
    castle::write_le32(&payload[0U], 9U);
    esf_meas<> out;
    ASSERT_TRUE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 8U), out));
    EXPECT_EQ(out.time_tag, 9U);
    EXPECT_TRUE(out.data.empty());

    castle::write_le16(&payload[4U], esf_meas<>::FLAGS_CALIB_TTAG_VALID);
    castle::write_le32(&payload[8U], 77U);
    ASSERT_TRUE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 12U), out));
    EXPECT_TRUE(out.data.empty());
    EXPECT_EQ(out.calib_ttag, 77U);
}

TEST(UbxMessageEsfMeas, RejectsLengthNotMatchingNumMeasAndCalibFlag)
{
    uint8_t payload[24U] = {};
    castle::write_le32(&payload[0U], 1U);
    castle::write_le16(&payload[4U], flags_with_count(esf_meas<>::FLAGS_CALIB_TTAG_VALID, 2U));
    esf_meas<> out;
    ASSERT_TRUE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 20U), out));

    out.time_tag = 5U;
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 16U), out)); // calibTtag missing
    EXPECT_EQ(out.time_tag, 0U);
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 19U), out));
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 21U), out));
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 24U), out)); // numMeas says 2, not 3
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 7U), out));
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 0U), out));
    EXPECT_TRUE(out.data.empty());

    castle::write_le16(&payload[4U], flags_with_count(0U, 2U));
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 20U), out)); // no calib: 16 expected
    EXPECT_TRUE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 16U), out));
}

TEST(UbxMessageEsfMeas, ZeroNumMeasInfersCountFromLength)
{
    uint8_t payload[16U] = {};
    castle::write_le32(&payload[0U], 0x12345678U);
    castle::write_le16(&payload[4U], esf_meas<>::FLAGS_TIME_MARK_EDGE); // numMeas = 0
    castle::write_le16(&payload[6U], 3U);
    castle::write_le32(&payload[8U], (10U << 24U) | 0x00FFFFFEU);
    castle::write_le32(&payload[12U], (11U << 24U) | 0x00000100U);

    esf_meas<> out;
    ASSERT_TRUE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 16U), out));
    EXPECT_EQ(out.num_meas(), 0U);
    EXPECT_EQ(out.time_tag, 0x12345678U);
    EXPECT_EQ(out.id, 3U);
    EXPECT_TRUE(out.time_mark_edge());
    ASSERT_EQ(out.data.size(), 2U);
    EXPECT_EQ(out.data[0U].data_type, 10U);
    EXPECT_EQ(out.data[0U].data_value, -2);
    EXPECT_EQ(out.data[1U].data_type, 11U);
    EXPECT_EQ(out.data[1U].data_value, 0x100);
    EXPECT_FALSE(out.calib_ttag_valid());

    // Implied count beyond capacity still decodes and clamps.
    esf_meas<1U> clamped;
    ASSERT_TRUE(esf_meas<1U>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 16U), clamped));
    ASSERT_EQ(clamped.data.size(), 1U);
    EXPECT_EQ(clamped.data[0U].data_value, -2);
}

TEST(UbxMessageEsfMeas, ZeroNumMeasRejectsPartialWordRemainder)
{
    uint8_t payload[16U] = {};
    esf_meas<> out;
    out.time_tag = 1U;
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 9U), out));
    EXPECT_EQ(out.time_tag, 0U);
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 14U), out));
    EXPECT_TRUE(out.data.empty());

    // With calibTtagValid the 4-byte tag must still fit after whole words.
    castle::write_le16(&payload[4U], esf_meas<>::FLAGS_CALIB_TTAG_VALID);
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 8U), out));
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 10U), out));
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 15U), out));
}

TEST(UbxMessageEsfMeas, ZeroNumMeasWithCalibTtagAndNoData)
{
    uint8_t payload[16U] = {};
    castle::write_le16(&payload[4U], esf_meas<>::FLAGS_CALIB_TTAG_VALID); // numMeas = 0
    castle::write_le32(&payload[8U], 0x0F0E0D0CU);
    esf_meas<> out;
    ASSERT_TRUE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 12U), out));
    EXPECT_TRUE(out.data.empty());
    EXPECT_TRUE(out.calib_ttag_valid());
    EXPECT_EQ(out.calib_ttag, 0x0F0E0D0CU);

    // Same flags, one word plus the tag: the inferred count is 1 and the tag is the last word.
    castle::write_le32(&payload[8U], (2U << 24U) | 7U);
    castle::write_le32(&payload[12U], 0xA0B0C0D0U);
    ASSERT_TRUE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 16U), out));
    ASSERT_EQ(out.data.size(), 1U);
    EXPECT_EQ(out.data[0U].data_value, 7);
    EXPECT_EQ(out.calib_ttag, 0xA0B0C0D0U);
}

TEST(UbxMessageEsfMeas, ClampsStoredMeasurementsAndStillReadsCalibTtag)
{
    uint8_t payload[20U] = {};
    castle::write_le16(&payload[4U], flags_with_count(esf_meas<>::FLAGS_CALIB_TTAG_VALID, 2U));
    castle::write_le32(&payload[8U], (1U << 24U) | 5U);
    castle::write_le32(&payload[12U], (2U << 24U) | 6U);
    castle::write_le32(&payload[16U], 0xA5A5A5A5U);
    esf_meas<1U> out;
    ASSERT_TRUE(esf_meas<1U>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload, 20U), out));
    EXPECT_EQ(out.num_meas(), 2U);
    ASSERT_EQ(out.data.size(), 1U);
    EXPECT_EQ(out.data[0U].data_type, 1U);
    EXPECT_EQ(out.data[0U].data_value, 5);
    EXPECT_EQ(out.calib_ttag, 0xA5A5A5A5U);
}

TEST(UbxMessageEsfMeas, RejectsWrongClassAndId)
{
    uint8_t payload[8U] = {};
    esf_meas<> out;
    out.time_tag = 1U;
    EXPECT_FALSE(esf_meas<>::matches(make_view(UBX_CLASS_NAV, UBX_ID_ESF_MEAS, payload, 8U)));
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_NAV, UBX_ID_ESF_MEAS, payload, 8U), out));
    EXPECT_EQ(out.time_tag, 0U);
    EXPECT_FALSE(esf_meas<>::matches(make_view(UBX_CLASS_ESF, UBX_ID_ESF_RAW, payload, 8U)));
    EXPECT_FALSE(esf_meas<>::decode(make_view(UBX_CLASS_ESF, UBX_ID_ESF_RAW, payload, 8U), out));
}

TEST(UbxMessageEsfMeas, EncodeRejectsDataThatDoesNotFitItsBitField)
{
    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    esf_meas<1U> in{};
    ASSERT_EQ(in.data.push_back(datum(1U, 0x800000)), castle::status::ok);
    EXPECT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::out_of_range);
    EXPECT_EQ(written, 0U);
    in.data[0U].data_value = -0x800001;
    EXPECT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::out_of_range);
    in.data[0U].data_value = 0;
    in.data[0U].data_type = 64U; // dataType is 6 bits
    EXPECT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::out_of_range);
    in.data[0U].data_type = 63U;
    in.data[0U].data_value = -0x800000;
    EXPECT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
}

TEST(UbxMessageEsfMeas, EncodeIntoTooSmallBufferReportsFull)
{
    esf_meas<2U> in{};
    in.flags = esf_meas<2U>::FLAGS_CALIB_TTAG_VALID;
    ASSERT_EQ(in.data.push_back(datum(1U, 1)), castle::status::ok);
    uint8_t frame[frame_size(16U)] = {};
    castle::size_type written = 1U;
    EXPECT_EQ(encode_message(in, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
}

} // namespace

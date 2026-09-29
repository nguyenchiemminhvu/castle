#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/ubx.hpp"

namespace
{

using namespace castle_ext::protocols::ubx;
using castle::container::array_view;

// ---------------------------------------------------------------------------
// message_view
// ---------------------------------------------------------------------------

TEST(UbxProtocol, MessageViewAccessors)
{
    uint8_t payload_data[2U] = {0x11U, 0x22U};
    message_view view;
    view.header.msg_class = UBX_CLASS_NAV;
    view.header.msg_id = UBX_ID_NAV_PVT;
    view.header.payload_length = 2U;
    view.payload = castle::container::array_view<CASTLE_CONST uint8_t>(payload_data, 2U);
    view.check = checksum{0x01U, 0x02U};

    EXPECT_EQ(view.msg_class(), UBX_CLASS_NAV);
    EXPECT_EQ(view.msg_id(), UBX_ID_NAV_PVT);
    EXPECT_EQ(view.payload_length(), 2U);
    EXPECT_TRUE(view.is(UBX_CLASS_NAV, UBX_ID_NAV_PVT));
    EXPECT_FALSE(view.is(UBX_CLASS_NAV, UBX_ID_NAV_STATUS));
    EXPECT_FALSE(view.is(UBX_CLASS_RXM, UBX_ID_NAV_PVT));
    EXPECT_EQ(view.key(), static_cast<uint16_t>((UBX_CLASS_NAV << 8U) | UBX_ID_NAV_PVT));
}

// ---------------------------------------------------------------------------
// checksum_accumulator
// ---------------------------------------------------------------------------

TEST(UbxProtocol, ChecksumAccumulatorResetUpdateValue)
{
    checksum_accumulator accumulator;
    EXPECT_EQ(accumulator.value().ck_a, 0U);
    EXPECT_EQ(accumulator.value().ck_b, 0U);

    accumulator.update(0x01U);
    accumulator.update(0x02U);
    EXPECT_EQ(accumulator.value().ck_a, 0x03U);
    EXPECT_EQ(accumulator.value().ck_b, 0x04U);

    accumulator.reset();
    EXPECT_EQ(accumulator.value().ck_a, 0U);
    EXPECT_EQ(accumulator.value().ck_b, 0U);
}

// ---------------------------------------------------------------------------
// Little-endian field helpers
// ---------------------------------------------------------------------------

TEST(UbxProtocol, LittleEndianReadHelpers)
{
    uint8_t u16_data[2U] = {0x34U, 0x12U};
    EXPECT_EQ(read_le16(u16_data), 0x1234U);
    EXPECT_EQ(read_le16s(u16_data), static_cast<int16_t>(0x1234));

    uint8_t u32_data[4U] = {0x78U, 0x56U, 0x34U, 0x12U};
    EXPECT_EQ(read_le32(u32_data), 0x12345678U);
    EXPECT_EQ(read_le32s(u32_data), static_cast<int32_t>(0x12345678U));

    uint8_t u64_data[8U] = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U, 0x08U};
    EXPECT_EQ(read_le64(u64_data), 0x0807060504030201ULL);
    EXPECT_EQ(read_le64s(u64_data), static_cast<int64_t>(0x0807060504030201ULL));
}

TEST(UbxProtocol, LittleEndianWriteHelpers)
{
    uint8_t u16_data[2U] = {0U, 0U};
    write_le16(u16_data, 0x1234U);
    EXPECT_EQ(u16_data[0U], 0x34U);
    EXPECT_EQ(u16_data[1U], 0x12U);

    uint8_t u32_data[4U] = {0U, 0U, 0U, 0U};
    write_le32(u32_data, 0x12345678U);
    EXPECT_EQ(u32_data[0U], 0x78U);
    EXPECT_EQ(u32_data[3U], 0x12U);

    uint8_t u64_data[8U] = {0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U};
    write_le64(u64_data, 0x0807060504030201ULL);
    EXPECT_EQ(u64_data[0U], 0x01U);
    EXPECT_EQ(u64_data[7U], 0x08U);
}

// ---------------------------------------------------------------------------
// Checksum computation
// ---------------------------------------------------------------------------

TEST(UbxProtocol, ComputeChecksumEmptyAndNonEmpty)
{
    castle::container::array_view<CASTLE_CONST uint8_t> empty_data(nullptr, 0U);
    checksum empty_result = compute_checksum(empty_data);
    EXPECT_EQ(empty_result.ck_a, 0U);
    EXPECT_EQ(empty_result.ck_b, 0U);

    uint8_t data[3U] = {0x01U, 0x02U, 0x03U};
    checksum result = compute_checksum(castle::container::array_view<CASTLE_CONST uint8_t>(data, 3U));
    EXPECT_EQ(result.ck_a, 0x06U);
    EXPECT_EQ(result.ck_b, 0x0AU);
}

TEST(UbxProtocol, ComputeFrameChecksumMatchesManualAccumulation)
{
    uint8_t payload_data[2U] = {0xAAU, 0xBBU};
    castle::container::array_view<CASTLE_CONST uint8_t> payload(payload_data, 2U);

    checksum frame_checksum = compute_frame_checksum(UBX_CLASS_CFG, UBX_ID_CFG_VALSET, 2U, payload);

    checksum_accumulator accumulator;
    accumulator.update(UBX_CLASS_CFG);
    accumulator.update(UBX_ID_CFG_VALSET);
    accumulator.update(2U);
    accumulator.update(0U);
    accumulator.update(0xAAU);
    accumulator.update(0xBBU);

    EXPECT_EQ(frame_checksum.ck_a, accumulator.value().ck_a);
    EXPECT_EQ(frame_checksum.ck_b, accumulator.value().ck_b);
}

TEST(UbxProtocol, FrameSize)
{
    EXPECT_EQ(frame_size(0U), UBX_FRAME_OVERHEAD);
    EXPECT_EQ(frame_size(10U), UBX_FRAME_OVERHEAD + 10U);
}

// ---------------------------------------------------------------------------
// encode_frame
// ---------------------------------------------------------------------------

TEST(UbxProtocol, EncodeFrameRejectsPayloadLargerThanMaxLen)
{
    castle::container::array_view<CASTLE_CONST uint8_t> oversized_payload(
        nullptr, static_cast<castle::size_type>(UBX_MAX_PAYLOAD_LEN) + 1U);

    uint8_t output[16U];
    castle::size_type written = 123U;
    castle::status result = encode_frame(
        UBX_CLASS_CFG, UBX_ID_CFG_VALSET, oversized_payload, output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::out_of_range);
    EXPECT_EQ(written, 0U);
}

TEST(UbxProtocol, EncodeFrameRejectsNullPayloadDataWithNonZeroSize)
{
    castle::container::array_view<CASTLE_CONST uint8_t> bad_payload(nullptr, 2U);

    uint8_t output[16U];
    castle::size_type written = 0U;
    castle::status result = encode_frame(
        UBX_CLASS_CFG, UBX_ID_CFG_VALSET, bad_payload, output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::invalid_argument);
}

TEST(UbxProtocol, EncodeFrameRejectsNullOutput)
{
    castle::container::array_view<CASTLE_CONST uint8_t> empty_payload(nullptr, 0U);

    castle::size_type written = 0U;
    castle::status result = encode_frame(
        UBX_CLASS_CFG, UBX_ID_CFG_VALSET, empty_payload, nullptr, 16U, written);

    EXPECT_EQ(result, castle::status::invalid_argument);
}

TEST(UbxProtocol, EncodeFrameRejectsUndersizedOutputBuffer)
{
    uint8_t payload_data[2U] = {0x01U, 0x02U};
    castle::container::array_view<CASTLE_CONST uint8_t> payload(payload_data, 2U);

    uint8_t output[4U];
    castle::size_type written = 0U;
    castle::status result = encode_frame(
        UBX_CLASS_CFG, UBX_ID_CFG_VALSET, payload, output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxProtocol, EncodeFrameSucceedsWithEmptyPayload)
{
    castle::container::array_view<CASTLE_CONST uint8_t> empty_payload(nullptr, 0U);

    uint8_t output[UBX_FRAME_OVERHEAD];
    castle::size_type written = 0U;
    castle::status result = encode_frame(
        UBX_CLASS_ACK, UBX_ID_ACK_ACK, empty_payload, output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD);
    EXPECT_EQ(output[0U], UBX_SYNC_CHAR_1);
    EXPECT_EQ(output[1U], UBX_SYNC_CHAR_2);
}

TEST(UbxProtocol, EncodeFrameSucceedsWithNonEmptyPayload)
{
    uint8_t payload_data[2U] = {0x01U, 0x02U};
    castle::container::array_view<CASTLE_CONST uint8_t> payload(payload_data, 2U);

    uint8_t output[UBX_FRAME_OVERHEAD + 2U];
    castle::size_type written = 0U;
    castle::status result = encode_frame(
        UBX_CLASS_CFG, UBX_ID_CFG_VALSET, payload, output, sizeof(output), written);

    EXPECT_EQ(result, castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD + 2U);
    EXPECT_EQ(output[6U], 0x01U);
    EXPECT_EQ(output[7U], 0x02U);
}

// ---------------------------------------------------------------------------
// decode_frame
// ---------------------------------------------------------------------------

TEST(UbxProtocol, DecodeFrameRejectsTooShortFrame)
{
    uint8_t frame[4U] = {UBX_SYNC_CHAR_1, UBX_SYNC_CHAR_2, 0U, 0U};
    message_view view;
    EXPECT_FALSE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, 4U), view));
}

TEST(UbxProtocol, DecodeFrameRejectsBadFirstSyncByte)
{
    uint8_t frame[UBX_FRAME_OVERHEAD] = {0x00U, UBX_SYNC_CHAR_2, 0U, 0U, 0U, 0U, 0U, 0U};
    message_view view;
    EXPECT_FALSE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, sizeof(frame)), view));
}

TEST(UbxProtocol, DecodeFrameRejectsBadSecondSyncByte)
{
    uint8_t frame[UBX_FRAME_OVERHEAD] = {UBX_SYNC_CHAR_1, 0x00U, 0U, 0U, 0U, 0U, 0U, 0U};
    message_view view;
    EXPECT_FALSE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, sizeof(frame)), view));
}

TEST(UbxProtocol, DecodeFrameRejectsSizeMismatch)
{
    // Declares a 10-byte payload (LEN_LO=10) but the buffer is only the 0-payload size.
    uint8_t frame[UBX_FRAME_OVERHEAD] = {UBX_SYNC_CHAR_1, UBX_SYNC_CHAR_2, 0U, 0U, 10U, 0U, 0U, 0U};
    message_view view;
    EXPECT_FALSE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, sizeof(frame)), view));
}

TEST(UbxProtocol, DecodeFrameRejectsChecksumMismatch)
{
    uint8_t payload_data[2U] = {0x01U, 0x02U};
    castle::container::array_view<CASTLE_CONST uint8_t> payload(payload_data, 2U);

    uint8_t frame[UBX_FRAME_OVERHEAD + 2U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(UBX_CLASS_CFG, UBX_ID_CFG_VALSET, payload, frame, sizeof(frame), written),
              castle::status::ok);

    frame[written - 1U] ^= 0xFFU;

    message_view view;
    EXPECT_FALSE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), view));
}

TEST(UbxProtocol, DecodeFrameRoundTripsEmptyAndNonEmptyPayload)
{
    castle::container::array_view<CASTLE_CONST uint8_t> empty_payload(nullptr, 0U);
    uint8_t empty_frame[UBX_FRAME_OVERHEAD];
    castle::size_type empty_written = 0U;
    ASSERT_EQ(encode_frame(UBX_CLASS_ACK, UBX_ID_ACK_NAK, empty_payload, empty_frame, sizeof(empty_frame), empty_written),
              castle::status::ok);

    message_view empty_view;
    ASSERT_TRUE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(empty_frame, empty_written), empty_view));
    EXPECT_EQ(empty_view.payload_length(), 0U);

    uint8_t payload_data[3U] = {0x01U, 0x02U, 0x03U};
    castle::container::array_view<CASTLE_CONST uint8_t> payload(payload_data, 3U);
    uint8_t frame[UBX_FRAME_OVERHEAD + 3U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(UBX_CLASS_NAV, UBX_ID_NAV_PVT, payload, frame, sizeof(frame), written),
              castle::status::ok);

    message_view view;
    ASSERT_TRUE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), view));
    EXPECT_TRUE(view.is(UBX_CLASS_NAV, UBX_ID_NAV_PVT));
    EXPECT_EQ(view.payload_length(), 3U);
    EXPECT_EQ(view.payload[0U], 0x01U);
    EXPECT_EQ(view.payload[2U], 0x03U);
}

// ---------------------------------------------------------------------------
// value_byte_size
// ---------------------------------------------------------------------------

TEST(UbxProtocol, ValueByteSizeCoversAllSizeCodes)
{
    EXPECT_EQ(value_byte_size(0x10000000U), 1U);
    EXPECT_EQ(value_byte_size(0x20000000U), 1U);
    EXPECT_EQ(value_byte_size(0x30000000U), 2U);
    EXPECT_EQ(value_byte_size(0x40000000U), 4U);
    EXPECT_EQ(value_byte_size(0x50000000U), 8U);
    EXPECT_EQ(value_byte_size(0x00000000U), 0U);
    EXPECT_EQ(value_byte_size(0x60000000U), 0U);
    EXPECT_EQ(value_byte_size(0xF0000000U), 0U);
}

// ---------------------------------------------------------------------------
// encode_frame and decode_frame tests
// ---------------------------------------------------------------------------

TEST(UbxProtocol, EncodesAndDecodesAFrameWithKnownPayload)
{
    uint8_t payload[3U] = {0x01U, 0x02U, 0x03U};
    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(UBX_CLASS_NAV, UBX_ID_NAV_DOP,
                           array_view<CASTLE_CONST uint8_t>(payload, 3U),
                           frame, sizeof(frame), written), castle::status::ok);

    message_view view;
    EXPECT_TRUE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), view));
    EXPECT_EQ(view.msg_class(), UBX_CLASS_NAV);
    EXPECT_EQ(view.msg_id(), UBX_ID_NAV_DOP);
    EXPECT_EQ(view.payload_length(), 3U);
    EXPECT_EQ(view.payload[2U], 0x03U);
}

TEST(UbxProtocol, DecodeRejectsFrameSizeAndChecksumViolations)
{
    uint8_t payload[1U] = {0x55U};
    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(UBX_CLASS_MON, UBX_ID_MON_VER,
                           array_view<CASTLE_CONST uint8_t>(payload, 1U),
                           frame, sizeof(frame), written), castle::status::ok);

    message_view view;
    EXPECT_FALSE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written - 1U), view));
    frame[written - 1U] ^= 0xFFU;
    EXPECT_FALSE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), view));
}

TEST(UbxProtocol, EmptyPayloadAndViewAccessorsRemainValid)
{
    uint8_t frame[16U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(UBX_CLASS_ACK, UBX_ID_ACK_ACK,
                           array_view<CASTLE_CONST uint8_t>(nullptr, 0U),
                           frame, sizeof(frame), written), castle::status::ok);
    message_view view;
    ASSERT_TRUE(decode_frame(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written), view));
    EXPECT_EQ(view.payload_length(), 0U);
    EXPECT_EQ(frame_size(0U), UBX_FRAME_OVERHEAD);
}

} // namespace

#include <gtest/gtest.h>

#include "castle_ext/protocols/rtcm/rtcm.hpp"

namespace
{

using namespace castle::protocols::rtcm;
using castle::checksums::crc24q;
using castle::container::array_view;

/**
 * @brief Message 1005 frame (station 1234, ECEF 4027894.0060 / 307045.6000 / 4919474.9100 m),
 * decoded and CRC-checked with pyrtcm.
 */
const uint8_t golden_1005[25U] = {
    0xD3U, 0x00U, 0x13U, 0x3EU, 0xD4U, 0xD2U, 0x3BU, 0x09U, 0x60U, 0xCFU, 0xD9U, 0x9CU, 0x80U,
    0xB7U, 0x03U, 0x70U, 0xC0U, 0x8BU, 0x74U, 0x3CU, 0x4CU, 0xACU, 0x19U, 0xD1U, 0xF1U};

/** @brief Deterministic payload byte `(index * 7 + 3) mod 256`. */
uint8_t pattern_byte(size_t index)
{
    return static_cast<uint8_t>(index * 7U + 3U);
}

void fill_pattern(uint8_t* data, size_t size)
{
    for (size_t i = 0U; i < size; ++i)
    {
        data[i] = pattern_byte(i);
    }
}

uint32_t frame_crc(const uint8_t* frame, size_t size)
{
    return (static_cast<uint32_t>(frame[size - 3U]) << 16U)
           | (static_cast<uint32_t>(frame[size - 2U]) << 8U)
           | static_cast<uint32_t>(frame[size - 1U]);
}

/** @brief Expects header, payload and trailer of a frame built from `payload`. */
void expect_valid_frame(const uint8_t* frame, size_t frame_length, const uint8_t* payload, size_t payload_length)
{
    ASSERT_EQ(frame_length, payload_length + 6U);
    EXPECT_EQ(frame[0U], 0xD3U);
    EXPECT_EQ(frame[1U], static_cast<uint8_t>((payload_length >> 8U) & 0x03U));
    EXPECT_EQ(frame[2U], static_cast<uint8_t>(payload_length & 0xFFU));
    for (size_t i = 0U; i < payload_length; ++i)
    {
        ASSERT_EQ(frame[3U + i], payload[i]) << "payload byte " << i;
    }
    // CRC is computed over header + payload and sent big-endian.
    EXPECT_EQ(frame_crc(frame, frame_length), crc24q::calculate(frame, frame_length - 3U));
    // A correct trailer makes the whole frame a CRC-24Q codeword.
    EXPECT_EQ(crc24q::calculate(frame, frame_length), 0U);
}

TEST(RtcmFraming, ConstantsMatchSpecification)
{
    EXPECT_EQ(RTCM_PREAMBLE, 0xD3U);
    EXPECT_EQ(RTCM_FRAME_INDEX_PREAMBLE, 0U);
    EXPECT_EQ(RTCM_FRAME_INDEX_LENGTH, 1U);
    EXPECT_EQ(RTCM_FRAME_INDEX_PAYLOAD, 3U);
    EXPECT_EQ(RTCM_CRC_SIZE, 3U);
    EXPECT_EQ(RTCM_FRAME_OVERHEAD, 6U);
    EXPECT_EQ(RTCM_MAX_PAYLOAD_LEN, 1023U);
    EXPECT_EQ(RTCM_MAX_FRAME_SIZE, 1029U);
    EXPECT_EQ(RTCM_MESSAGE_NUMBER_BITS, 12U);
    EXPECT_EQ(RTCM_STATION_ID_BITS, 12U);
}

TEST(RtcmFraming, MessageNumbersMatchUbloxInputList)
{
    EXPECT_EQ(RTCM_TYPE_GPS_L1_OBS, 1001U);
    EXPECT_EQ(RTCM_TYPE_GPS_L1_OBS_EXT, 1002U);
    EXPECT_EQ(RTCM_TYPE_GPS_L1L2_OBS, 1003U);
    EXPECT_EQ(RTCM_TYPE_GPS_L1L2_OBS_EXT, 1004U);
    EXPECT_EQ(RTCM_TYPE_STATION_ARP, 1005U);
    EXPECT_EQ(RTCM_TYPE_STATION_ARP_HEIGHT, 1006U);
    EXPECT_EQ(RTCM_TYPE_ANTENNA_DESCRIPTOR, 1007U);
    EXPECT_EQ(RTCM_TYPE_GLO_L1_OBS, 1009U);
    EXPECT_EQ(RTCM_TYPE_GLO_L1_OBS_EXT, 1010U);
    EXPECT_EQ(RTCM_TYPE_GLO_L1L2_OBS, 1011U);
    EXPECT_EQ(RTCM_TYPE_GLO_L1L2_OBS_EXT, 1012U);
    EXPECT_EQ(RTCM_TYPE_RECEIVER_ANTENNA, 1033U);
    EXPECT_EQ(RTCM_TYPE_GLO_BIASES, 1230U);
}

TEST(RtcmFraming, FrameSizeAddsHeaderAndCrc)
{
    static_assert(frame_size(0U) == 6U, "frame_size is usable in constant expressions");
    EXPECT_EQ(frame_size(0U), 6U);
    EXPECT_EQ(frame_size(1U), 7U);
    EXPECT_EQ(frame_size(19U), 25U);
    EXPECT_EQ(frame_size(RTCM_MAX_PAYLOAD_LEN), RTCM_MAX_FRAME_SIZE);
}

TEST(RtcmFraming, EncodeFrameReproducesGolden1005Frame)
{
    // Payload of the golden frame is everything between the header and the CRC.
    array_view<const uint8_t> payload(&golden_1005[3U], 19U);

    uint8_t frame[64U] = {};
    castle::size_type written = 0xDEADU;
    ASSERT_EQ(encode_frame(payload, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, sizeof(golden_1005));
    for (size_t i = 0U; i < sizeof(golden_1005); ++i)
    {
        EXPECT_EQ(frame[i], golden_1005[i]) << "byte " << i;
    }
    EXPECT_EQ(frame_crc(frame, written), 0x19D1F1U);
    expect_valid_frame(frame, written, &golden_1005[3U], 19U);
}

TEST(RtcmFraming, EncodeFrameWritesBytesBeyondFrameNever)
{
    array_view<const uint8_t> payload(&golden_1005[3U], 19U);

    uint8_t buffer[40U];
    for (uint8_t& byte : buffer)
    {
        byte = 0xEEU;
    }
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(payload, buffer, sizeof(buffer), written), castle::status::ok);
    ASSERT_EQ(written, 25U);
    for (size_t i = written; i < sizeof(buffer); ++i)
    {
        ASSERT_EQ(buffer[i], 0xEEU) << "guard byte " << i;
    }
}

TEST(RtcmFraming, EncodeFrameExactFitSucceeds)
{
    array_view<const uint8_t> payload(&golden_1005[3U], 19U);

    uint8_t exact[frame_size(19U)] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(payload, exact, sizeof(exact), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(exact));
    expect_valid_frame(exact, written, &golden_1005[3U], 19U);
}

TEST(RtcmFraming, EncodeFrameOneByteShortReportsFull)
{
    array_view<const uint8_t> payload(&golden_1005[3U], 19U);

    uint8_t buffer[frame_size(19U) - 1U];
    for (uint8_t& byte : buffer)
    {
        byte = 0xEEU;
    }
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(encode_frame(payload, buffer, sizeof(buffer), written), castle::status::full);
    EXPECT_EQ(written, 0U);
    for (uint8_t byte : buffer)
    {
        EXPECT_EQ(byte, 0xEEU); // nothing is written when the frame does not fit
    }

    written = 0xDEADU;
    EXPECT_EQ(encode_frame(payload, buffer, 0U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmFraming, EncodeFrameEmptyPayload)
{
    uint8_t frame[RTCM_FRAME_OVERHEAD] = {};
    castle::size_type written = 0xDEADU;
    ASSERT_EQ(encode_frame(array_view<const uint8_t>(), frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, 6U);

    const uint8_t expected[6U] = {0xD3U, 0x00U, 0x00U, 0x47U, 0xEAU, 0x4BU};
    for (size_t i = 0U; i < sizeof(expected); ++i)
    {
        EXPECT_EQ(frame[i], expected[i]) << "byte " << i;
    }
    expect_valid_frame(frame, written, nullptr, 0U);

    // A null data pointer is allowed when the size is zero; a non-null empty view as well.
    written = 0xDEADU;
    ASSERT_EQ(encode_frame(array_view<const uint8_t>(nullptr, 0U), frame, sizeof(frame), written),
              castle::status::ok);
    EXPECT_EQ(written, 6U);
    written = 0xDEADU;
    ASSERT_EQ(encode_frame(array_view<const uint8_t>(golden_1005, 0U), frame, sizeof(frame), written),
              castle::status::ok);
    EXPECT_EQ(written, 6U);

    // One byte short for even the empty frame.
    written = 0xDEADU;
    EXPECT_EQ(encode_frame(array_view<const uint8_t>(), frame, RTCM_FRAME_OVERHEAD - 1U, written),
              castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmFraming, EncodeFrameSinglePayloadByte)
{
    const uint8_t payload_byte = 0xA5U;
    uint8_t frame[7U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(array_view<const uint8_t>(&payload_byte, 1U), frame, sizeof(frame), written),
              castle::status::ok);
    ASSERT_EQ(written, 7U);

    const uint8_t expected[7U] = {0xD3U, 0x00U, 0x01U, 0xA5U, 0x20U, 0x24U, 0xDAU};
    for (size_t i = 0U; i < sizeof(expected); ++i)
    {
        EXPECT_EQ(frame[i], expected[i]) << "byte " << i;
    }
    expect_valid_frame(frame, written, &payload_byte, 1U);
}

struct length_case
{
    size_t length;
    uint8_t length_hi;
    uint8_t length_lo;
    uint8_t crc_hi;
    uint8_t crc_mid;
    uint8_t crc_lo;
};

TEST(RtcmFraming, LengthFieldSplitsIntoTwoMostSignificantBitsAndLowByte)
{
    // Expected trailers come from an independent bit-by-bit CRC-24Q implementation.
    const length_case cases[] = {
        {255U, 0x00U, 0xFFU, 0x53U, 0x01U, 0xBEU},
        {256U, 0x01U, 0x00U, 0x49U, 0x2DU, 0xCBU},
        {1000U, 0x03U, 0xE8U, 0x52U, 0x7FU, 0x85U},
        {RTCM_MAX_PAYLOAD_LEN, 0x03U, 0xFFU, 0xDEU, 0x10U, 0x5AU},
    };

    static uint8_t payload[RTCM_MAX_PAYLOAD_LEN];
    static uint8_t frame[RTCM_MAX_FRAME_SIZE];
    fill_pattern(payload, sizeof(payload));

    for (const length_case& test_case : cases)
    {
        castle::size_type written = 0U;
        ASSERT_EQ(encode_frame(array_view<const uint8_t>(payload, test_case.length), frame, sizeof(frame), written),
                  castle::status::ok)
            << "length " << test_case.length;
        ASSERT_EQ(written, test_case.length + 6U);
        EXPECT_EQ(frame[0U], 0xD3U);
        EXPECT_EQ(frame[1U], test_case.length_hi) << "length " << test_case.length;
        EXPECT_EQ(frame[2U], test_case.length_lo) << "length " << test_case.length;
        EXPECT_EQ(frame[written - 3U], test_case.crc_hi) << "length " << test_case.length;
        EXPECT_EQ(frame[written - 2U], test_case.crc_mid) << "length " << test_case.length;
        EXPECT_EQ(frame[written - 1U], test_case.crc_lo) << "length " << test_case.length;
        expect_valid_frame(frame, written, payload, test_case.length);
    }
}

TEST(RtcmFraming, EncodeFrameMaxPayloadFillsMaxFrame)
{
    static uint8_t payload[RTCM_MAX_PAYLOAD_LEN];
    static uint8_t frame[RTCM_MAX_FRAME_SIZE];
    fill_pattern(payload, sizeof(payload));

    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(array_view<const uint8_t>(payload, sizeof(payload)), frame, sizeof(frame), written),
              castle::status::ok);
    EXPECT_EQ(written, RTCM_MAX_FRAME_SIZE);

    // One byte short of the maximum frame.
    written = 0xDEADU;
    EXPECT_EQ(encode_frame(array_view<const uint8_t>(payload, sizeof(payload)), frame, sizeof(frame) - 1U, written),
              castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmFraming, EncodeFrameRejectsPayloadOneOverMaximum)
{
    static uint8_t payload[RTCM_MAX_PAYLOAD_LEN + 2U];
    static uint8_t frame[RTCM_MAX_FRAME_SIZE + 64U];
    fill_pattern(payload, sizeof(payload));

    castle::size_type written = 0xDEADU;
    EXPECT_EQ(encode_frame(array_view<const uint8_t>(payload, RTCM_MAX_PAYLOAD_LEN + 1U), frame, sizeof(frame), written),
              castle::status::out_of_range);
    EXPECT_EQ(written, 0U);

    written = 0xDEADU;
    EXPECT_EQ(encode_frame(array_view<const uint8_t>(payload, RTCM_MAX_PAYLOAD_LEN + 2U), frame, sizeof(frame), written),
              castle::status::out_of_range);
    EXPECT_EQ(written, 0U);

    // The size limit takes precedence over the capacity check.
    written = 0xDEADU;
    EXPECT_EQ(encode_frame(array_view<const uint8_t>(payload, RTCM_MAX_PAYLOAD_LEN + 1U), frame, 8U, written),
              castle::status::out_of_range);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmFraming, EncodeFrameRejectsNullArguments)
{
    array_view<const uint8_t> payload(&golden_1005[3U], 19U);
    uint8_t frame[64U];
    for (uint8_t& byte : frame)
    {
        byte = 0xEEU;
    }

    castle::size_type written = 0xDEADU;
    EXPECT_EQ(encode_frame(payload, nullptr, 64U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);

    written = 0xDEADU;
    EXPECT_EQ(encode_frame(array_view<const uint8_t>(), nullptr, 64U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);

    // Non-empty payload with a null data pointer.
    written = 0xDEADU;
    EXPECT_EQ(encode_frame(array_view<const uint8_t>(nullptr, 5U), frame, sizeof(frame), written),
              castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
    for (uint8_t byte : frame)
    {
        EXPECT_EQ(byte, 0xEEU);
    }

    // Argument errors take precedence over the length and capacity checks.
    written = 0xDEADU;
    EXPECT_EQ(encode_frame(array_view<const uint8_t>(nullptr, RTCM_MAX_PAYLOAD_LEN + 1U), frame, sizeof(frame), written),
              castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
    written = 0xDEADU;
    EXPECT_EQ(encode_frame(payload, nullptr, 0U, written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

TEST(RtcmFraming, DetailFinishFrameCompletesInPlacePayload)
{
    uint8_t frame[RTCM_FRAME_OVERHEAD + 19U];
    for (uint8_t& byte : frame)
    {
        byte = 0xEEU;
    }
    for (size_t i = 0U; i < 19U; ++i)
    {
        frame[RTCM_FRAME_INDEX_PAYLOAD + i] = golden_1005[3U + i];
    }

    const castle::size_type total = detail::finish_frame(frame, 19U);
    ASSERT_EQ(total, sizeof(golden_1005));
    for (size_t i = 0U; i < sizeof(golden_1005); ++i)
    {
        EXPECT_EQ(frame[i], golden_1005[i]) << "byte " << i;
    }
}

TEST(RtcmFraming, DetailFinishFrameOverwritesStaleHeaderAndTrailer)
{
    uint8_t frame[RTCM_FRAME_OVERHEAD + 1U];
    for (uint8_t& byte : frame)
    {
        byte = 0xFFU;
    }
    frame[RTCM_FRAME_INDEX_PAYLOAD] = 0xA5U;

    ASSERT_EQ(detail::finish_frame(frame, 1U), 7U);
    const uint8_t expected[7U] = {0xD3U, 0x00U, 0x01U, 0xA5U, 0x20U, 0x24U, 0xDAU};
    for (size_t i = 0U; i < sizeof(expected); ++i)
    {
        EXPECT_EQ(frame[i], expected[i]) << "byte " << i;
    }
}

TEST(RtcmFraming, FrameAlwaysEndsWithBigEndianCrc)
{
    static uint8_t payload[300U];
    static uint8_t frame[300U + RTCM_FRAME_OVERHEAD];
    fill_pattern(payload, sizeof(payload));

    for (size_t length = 0U; length <= sizeof(payload); length += 37U)
    {
        castle::size_type written = 0U;
        ASSERT_EQ(encode_frame(array_view<const uint8_t>(payload, length), frame, sizeof(frame), written),
                  castle::status::ok);
        const uint32_t crc = crc24q::calculate(frame, written - 3U);
        ASSERT_EQ(frame[written - 3U], static_cast<uint8_t>(crc >> 16U));
        ASSERT_EQ(frame[written - 2U], static_cast<uint8_t>(crc >> 8U));
        ASSERT_EQ(frame[written - 1U], static_cast<uint8_t>(crc));
        expect_valid_frame(frame, written, payload, length);
    }
}

} // namespace

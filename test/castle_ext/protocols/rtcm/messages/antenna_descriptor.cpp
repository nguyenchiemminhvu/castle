#include <gtest/gtest.h>

#include "castle_ext/protocols/rtcm/messages/antenna_descriptor.hpp"

#include <string>

namespace
{

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

// Golden frame: station 1234, "TRM59800.00     NONE", setup 3. Decoded and CRC-checked with pyrtcm.
const char* const GOLDEN_1007 = "d300193ef4d21454524d35393830302e303020202020204e4f4e4503f5d214";

// Hand-assembled with an independent Python bit packer + CRC-24Q, confirmed by pyrtcm.
const char* const EMPTY_1007 = "d300053ef000000041778b";
const char* const MAX_1007 = "d300243effff1f4142434445464748494a4b4c4d4e4f505152535455565758595a3031323334ff856406";
const char* const HIGH_BYTES_1007 = "d300083ef0010380ff7e073aeeff";

struct frame_buffer
{
    uint8_t data[64U] = {};
    castle::size_type size = 0U;
};

std::string to_hex(const frame_buffer& frame)
{
    static const char digits[] = "0123456789abcdef";
    std::string text;
    for (castle::size_type index = 0U; index < frame.size; ++index)
    {
        text.push_back(digits[frame.data[index] >> 4U]);
        text.push_back(digits[frame.data[index] & 0x0FU]);
    }
    return text;
}

castle::status encode(const antenna_descriptor& message, frame_buffer& frame)
{
    return encode_message(message, frame.data, sizeof(frame.data), frame.size);
}

void expect_frame(const antenna_descriptor& message, const char* hex)
{
    frame_buffer frame;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(to_hex(frame), std::string(hex));
}

antenna_descriptor golden_message()
{
    antenna_descriptor message{};
    message.station_id = 1234U;
    EXPECT_EQ(message.descriptor.assign("TRM59800.00     NONE"), castle::status::ok);
    message.setup_id = 3U;
    return message;
}

TEST(RtcmMessageAntennaDescriptor, DefaultsAndConstants)
{
    const antenna_descriptor message{};
    EXPECT_EQ(message.station_id, 0U);
    EXPECT_TRUE(message.descriptor.empty());
    EXPECT_EQ(message.setup_id, 0U);
    EXPECT_EQ(RTCM_MAX_DESCRIPTOR_LEN, 31U);
    EXPECT_EQ(descriptor_string{}.capacity(), 31U);
}

TEST(RtcmMessageAntennaDescriptor, AcceptsOnly1007)
{
    static_assert(antenna_descriptor::MESSAGE_TYPE == RTCM_TYPE_ANTENNA_DESCRIPTOR, "1007 model");
    EXPECT_TRUE(antenna_descriptor::accepts(1007U));
    EXPECT_TRUE(antenna_descriptor::accepts(RTCM_TYPE_ANTENNA_DESCRIPTOR));
    EXPECT_FALSE(antenna_descriptor::accepts(1006U));
    EXPECT_FALSE(antenna_descriptor::accepts(1008U));
    EXPECT_FALSE(antenna_descriptor::accepts(1033U));
    EXPECT_FALSE(antenna_descriptor::accepts(0U));
    EXPECT_FALSE(antenna_descriptor::accepts(0xFFFFU));
}

TEST(RtcmMessageAntennaDescriptor, EncodesGoldenFrame)
{
    expect_frame(golden_message(), GOLDEN_1007);
}

TEST(RtcmMessageAntennaDescriptor, EncodesEmptyDescriptorAsZeroCount)
{
    expect_frame(antenna_descriptor{}, EMPTY_1007);

    frame_buffer frame;
    ASSERT_EQ(encode(antenna_descriptor{}, frame), castle::status::ok);
    EXPECT_EQ(frame.size, frame_size(5U));
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 3U], 0U); // DF029 character count
}

TEST(RtcmMessageAntennaDescriptor, EncodesMaximumLengthDescriptorAndFieldBounds)
{
    antenna_descriptor message{};
    message.station_id = 4095U;
    ASSERT_EQ(message.descriptor.assign("ABCDEFGHIJKLMNOPQRSTUVWXYZ01234"), castle::status::ok);
    ASSERT_EQ(message.descriptor.size(), 31U);
    message.setup_id = 255U;
    expect_frame(message, MAX_1007);

    frame_buffer frame;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.size, frame_size(36U));
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 3U], 31U);
}

TEST(RtcmMessageAntennaDescriptor, DescriptorLongerThanLimitCannotBeStored)
{
    antenna_descriptor message{};
    EXPECT_EQ(message.descriptor.assign("ABCDEFGHIJKLMNOPQRSTUVWXYZ012345"), castle::status::full); // 32 chars
    EXPECT_TRUE(message.descriptor.empty());
}

TEST(RtcmMessageAntennaDescriptor, CharactersAreWrittenAsRawBytes)
{
    antenna_descriptor message{};
    message.station_id = 1U;
    ASSERT_EQ(message.descriptor.assign("\x80\xFF~"), castle::status::ok);
    message.setup_id = 7U;
    expect_frame(message, HIGH_BYTES_1007);
}

TEST(RtcmMessageAntennaDescriptor, StationIdBounds)
{
    frame_buffer frame;
    antenna_descriptor message = golden_message();

    message.station_id = 0U;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 1U], 0xF0U); // DF002 low nibble 0xF, DF003 high nibble 0
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 2U], 0x00U);

    message.station_id = 4095U;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 1U], 0xFFU);
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 2U], 0xFFU);

    message.station_id = 4096U;
    frame.size = 99U;
    EXPECT_EQ(encode(message, frame), castle::status::out_of_range);
    EXPECT_EQ(frame.size, 0U);

    message.station_id = 0xFFFFU;
    EXPECT_EQ(encode(message, frame), castle::status::out_of_range);
    EXPECT_EQ(frame.size, 0U);
}

TEST(RtcmMessageAntennaDescriptor, SetupIdBounds)
{
    frame_buffer frame;
    antenna_descriptor message{};

    message.setup_id = 0U;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 4U], 0x00U);

    message.setup_id = 255U;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 4U], 0xFFU);
}

TEST(RtcmMessageAntennaDescriptor, ExactFitAndOneByteShortBuffers)
{
    const antenna_descriptor message = golden_message();
    const castle::size_type exact = frame_size(5U + 20U); // 25 byte payload
    uint8_t buffer[64U] = {};
    castle::size_type written = 99U;

    ASSERT_EQ(encode_message(message, buffer, exact, written), castle::status::ok);
    EXPECT_EQ(written, exact);
    EXPECT_EQ(buffer[2], 25U);

    written = 99U;
    EXPECT_EQ(encode_message(message, buffer, exact - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    EXPECT_EQ(encode_message(message, buffer, 0U, written), castle::status::full);
    EXPECT_EQ(encode_message(message, nullptr, sizeof(buffer), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

} // namespace

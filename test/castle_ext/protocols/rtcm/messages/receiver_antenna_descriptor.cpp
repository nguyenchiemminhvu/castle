#include <gtest/gtest.h>

#include "castle_ext/protocols/rtcm/messages/receiver_antenna_descriptor.hpp"

#include <string>

namespace
{

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

// Golden frame: station 1234, antenna "ANT" setup 2, antenna serial "SN123", receiver "UBLOX ZED-F9P",
// firmware "HPG 1.32", receiver serial "0042". Decoded and CRC-checked with pyrtcm.
const char* const GOLDEN_1033 =
    "d3002a4094d203414e540205534e3132330d55424c4f58205a45442d4639500848504720312e3332043030343230571f";

// Hand-assembled with an independent Python bit packer + CRC-24Q, confirmed by pyrtcm.
const char* const EMPTY_1033 = "d300094090000000000000000116be";
const char* const MAX_1033 =
    "d300a4409fff1f4142434445464748494a4b4c4d4e4f505152535455565758595a3031323334ff1f34333231305a595857565554535251504f4e4d4c4b4a4948474645444342411f6162636465666768696a6b6c6d6e6f707172737475767778797a30313233341f6162636465666768696a6b6c6d6e6f707172737475767778797a35363738391f303132333435363738394142434445464748494a4b4c4d4e4f505152535455189889";
// Only the string at index k holds (k + 1) 'X' characters; station 5, setup id 9.
const char* const ONLY_STRING[5U] = {
    "d3000a409005015809000000004d46b9",
    "d3000b4090050009025858000000fec53d",
    "d3000c409005000900035858580000efa633",
    "d3000d40900500090000045858585800ca1bf5",
    "d3000e4090050009000000055858585858d34dbf",
};

const char* const MAX_TEXT = "ABCDEFGHIJKLMNOPQRSTUVWXYZ01234";

struct frame_buffer
{
    uint8_t data[256U] = {};
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

castle::status encode(const receiver_antenna_descriptor& message, frame_buffer& frame)
{
    return encode_message(message, frame.data, sizeof(frame.data), frame.size);
}

void expect_frame(const receiver_antenna_descriptor& message, const char* hex)
{
    frame_buffer frame;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(to_hex(frame), std::string(hex));
}

receiver_antenna_descriptor golden_message()
{
    receiver_antenna_descriptor message{};
    message.station_id = 1234U;
    EXPECT_EQ(message.antenna.assign("ANT"), castle::status::ok);
    message.antenna_setup_id = 2U;
    EXPECT_EQ(message.antenna_serial.assign("SN123"), castle::status::ok);
    EXPECT_EQ(message.receiver_type.assign("UBLOX ZED-F9P"), castle::status::ok);
    EXPECT_EQ(message.firmware.assign("HPG 1.32"), castle::status::ok);
    EXPECT_EQ(message.receiver_serial.assign("0042"), castle::status::ok);
    return message;
}

descriptor_string* strings_of(receiver_antenna_descriptor& message, castle::size_type index)
{
    descriptor_string* const all[5U] = {&message.antenna,
                                        &message.antenna_serial,
                                        &message.receiver_type,
                                        &message.firmware,
                                        &message.receiver_serial};
    return all[index];
}

TEST(RtcmMessageReceiverAntennaDescriptor, DefaultsAndConstants)
{
    const receiver_antenna_descriptor message{};
    EXPECT_EQ(message.station_id, 0U);
    EXPECT_TRUE(message.antenna.empty());
    EXPECT_EQ(message.antenna_setup_id, 0U);
    EXPECT_TRUE(message.antenna_serial.empty());
    EXPECT_TRUE(message.receiver_type.empty());
    EXPECT_TRUE(message.firmware.empty());
    EXPECT_TRUE(message.receiver_serial.empty());
    EXPECT_EQ(RTCM_MAX_DESCRIPTOR_LEN, 31U);
}

TEST(RtcmMessageReceiverAntennaDescriptor, AcceptsOnly1033)
{
    static_assert(receiver_antenna_descriptor::MESSAGE_TYPE == RTCM_TYPE_RECEIVER_ANTENNA, "1033 model");
    EXPECT_TRUE(receiver_antenna_descriptor::accepts(1033U));
    EXPECT_TRUE(receiver_antenna_descriptor::accepts(RTCM_TYPE_RECEIVER_ANTENNA));
    EXPECT_FALSE(receiver_antenna_descriptor::accepts(1007U));
    EXPECT_FALSE(receiver_antenna_descriptor::accepts(1032U));
    EXPECT_FALSE(receiver_antenna_descriptor::accepts(1034U));
    EXPECT_FALSE(receiver_antenna_descriptor::accepts(0U));
    EXPECT_FALSE(receiver_antenna_descriptor::accepts(0xFFFFU));
}

TEST(RtcmMessageReceiverAntennaDescriptor, EncodesGoldenFrame)
{
    expect_frame(golden_message(), GOLDEN_1033);
}

TEST(RtcmMessageReceiverAntennaDescriptor, EncodesAllEmptyStrings)
{
    expect_frame(receiver_antenna_descriptor{}, EMPTY_1033);

    frame_buffer frame;
    ASSERT_EQ(encode(receiver_antenna_descriptor{}, frame), castle::status::ok);
    EXPECT_EQ(frame.size, frame_size(9U)); // 3 header bytes, 5 counts, 1 setup id
}

TEST(RtcmMessageReceiverAntennaDescriptor, EncodesMaximumLengthStringsAndFieldBounds)
{
    receiver_antenna_descriptor message{};
    message.station_id = 4095U;
    ASSERT_EQ(message.antenna.assign(MAX_TEXT), castle::status::ok);
    message.antenna_setup_id = 255U;
    ASSERT_EQ(message.antenna_serial.assign("43210ZYXWVUTSRQPONMLKJIHGFEDCBA"), castle::status::ok);
    ASSERT_EQ(message.receiver_type.assign("abcdefghijklmnopqrstuvwxyz01234"), castle::status::ok);
    ASSERT_EQ(message.firmware.assign("abcdefghijklmnopqrstuvwxyz56789"), castle::status::ok);
    ASSERT_EQ(message.receiver_serial.assign("0123456789ABCDEFGHIJKLMNOPQRSTU"), castle::status::ok);
    expect_frame(message, MAX_1033);

    frame_buffer frame;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.size, frame_size(164U)); // 3 header bytes + 5 counts + 1 setup id + 5 * 31 characters
}

TEST(RtcmMessageReceiverAntennaDescriptor, EachStringIsWrittenAtItsOwnPosition)
{
    for (castle::size_type index = 0U; index < 5U; ++index)
    {
        receiver_antenna_descriptor message{};
        message.station_id = 5U;
        message.antenna_setup_id = 9U;
        ASSERT_EQ(strings_of(message, index)->assign(std::string(index + 1U, 'X').c_str()), castle::status::ok);
        expect_frame(message, ONLY_STRING[index]);
    }
}

TEST(RtcmMessageReceiverAntennaDescriptor, StringLongerThanLimitCannotBeStored)
{
    for (castle::size_type index = 0U; index < 5U; ++index)
    {
        receiver_antenna_descriptor message{};
        EXPECT_EQ(strings_of(message, index)->assign(std::string(32U, 'Q').c_str()), castle::status::full);
        EXPECT_TRUE(strings_of(message, index)->empty());
    }
}

TEST(RtcmMessageReceiverAntennaDescriptor, StationIdBounds)
{
    frame_buffer frame;
    receiver_antenna_descriptor message = golden_message();

    message.station_id = 0U;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 1U], 0x90U); // 1033 = 0x409 -> low nibble 9, id high nibble 0
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 2U], 0x00U);

    message.station_id = 4095U;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 1U], 0x9FU);
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 2U], 0xFFU);

    message.station_id = 4096U;
    frame.size = 99U;
    EXPECT_EQ(encode(message, frame), castle::status::out_of_range);
    EXPECT_EQ(frame.size, 0U);
}

TEST(RtcmMessageReceiverAntennaDescriptor, AntennaSetupIdBounds)
{
    frame_buffer frame;
    receiver_antenna_descriptor message{};
    const castle::size_type setup_index = RTCM_FRAME_INDEX_PAYLOAD + 4U; // after type, id and empty antenna

    message.antenna_setup_id = 0U;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.data[setup_index], 0x00U);

    message.antenna_setup_id = 255U;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.data[setup_index], 0xFFU);
}

TEST(RtcmMessageReceiverAntennaDescriptor, ExactFitAndOneByteShortBuffers)
{
    const receiver_antenna_descriptor message = golden_message();
    const castle::size_type exact = frame_size(42U); // golden payload length from the frame header
    uint8_t buffer[128U] = {};
    castle::size_type written = 99U;

    ASSERT_EQ(encode_message(message, buffer, exact, written), castle::status::ok);
    EXPECT_EQ(written, exact);
    EXPECT_EQ(buffer[2], 42U);

    written = 99U;
    EXPECT_EQ(encode_message(message, buffer, exact - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    EXPECT_EQ(encode_message(message, buffer, 0U, written), castle::status::full);
    EXPECT_EQ(encode_message(message, nullptr, sizeof(buffer), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

} // namespace

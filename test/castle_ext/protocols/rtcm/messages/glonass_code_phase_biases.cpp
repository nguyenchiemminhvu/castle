#include <gtest/gtest.h>

#include "castle_ext/protocols/rtcm/messages/glonass_code_phase_biases.hpp"

#include <string>

namespace
{

using namespace castle::protocols::rtcm;
using namespace castle::protocols::rtcm::messages;

// Golden frame: station 1234, indicator set, mask 0b1101, L1 C/A -100, L1 P 200, L2 P -32768.
// Decoded and CRC-checked with pyrtcm.
const char* const GOLDEN_1230 = "d3000a4ce4d28dff9c00c880006fbb59";

// Station 1234, indicator set, L1 C/A -100, L1 P 200, L2 C/A 32767, L2 P -32768, one frame per
// signals_mask 0..15. Hand-assembled with an independent Python bit packer + CRC-24Q and
// confirmed by pyrtcm.
const char* const MASK_FRAMES[16U] = {
    "d300044ce4d2801435a6",
    "d300064ce4d2818000610b94",
    "d300064ce4d2827fff6161f2",
    "d300084ce4d2837fff80000f6c53",
    "d300064ce4d28400c86d37cb",
    "d300084ce4d28500c88000d72089",
    "d300084ce4d28600c87fff385302",
    "d3000a4ce4d28700c87fff80002fbfe3",
    "d300064ce4d288ff9cf1dfa5",
    "d300084ce4d289ff9c8000e963b8",
    "d300084ce4d28aff9c7fff061033",
    "d3000a4ce4d28bff9c7fff800070408e",
    "d300084ce4d28cff9c00c852392b",
    "d3000a4ce4d28dff9c00c880006fbb59",
    "d3000a4ce4d28eff9c00c87fff203529",
    "d3000c4ce4d28fff9c00c87fff8000b57131",
};

const char* const ZERO_1230 = "d300044ce00000deaa72";
const char* const MAX_BIASES = "d3000c4cefff8f7fff7fff7fff7fff85affb";   // station 4095, indicator set, mask 15, every bias +32767
const char* const MIN_BIASES = "d3000c4cefff0f80008000800080007b3631";   // station 4095, indicator clear, mask 15, every bias -32768
const char* const INDICATOR_CLEAR = "d300064ce007080001c58331"; // station 7, indicator clear, mask 8, L1 C/A +1

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

castle::status encode(const glonass_code_phase_biases& message, frame_buffer& frame)
{
    return encode_message(message, frame.data, sizeof(frame.data), frame.size);
}

void expect_frame(const glonass_code_phase_biases& message, const char* hex)
{
    frame_buffer frame;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(to_hex(frame), std::string(hex));
}

castle::size_type popcount4(uint8_t mask)
{
    castle::size_type count = 0U;
    for (uint8_t bit = 0U; bit < 4U; ++bit)
    {
        count += (mask >> bit) & 1U;
    }
    return count;
}

glonass_code_phase_biases all_biases(uint8_t mask)
{
    glonass_code_phase_biases message{};
    message.station_id = 1234U;
    message.code_phase_bias_indicator = true;
    message.signals_mask = mask;
    message.l1_ca_bias = -100;
    message.l1_p_bias = 200;
    message.l2_ca_bias = 32767;
    message.l2_p_bias = -32768;
    return message;
}

TEST(RtcmMessageGlonassCodePhaseBiases, DefaultsAndConstants)
{
    const glonass_code_phase_biases message{};
    EXPECT_EQ(message.station_id, 0U);
    EXPECT_FALSE(message.code_phase_bias_indicator);
    EXPECT_EQ(message.signals_mask, 0U);
    EXPECT_EQ(message.l1_ca_bias, 0);
    EXPECT_EQ(message.l1_p_bias, 0);
    EXPECT_EQ(message.l2_ca_bias, 0);
    EXPECT_EQ(message.l2_p_bias, 0);

    EXPECT_EQ(glonass_code_phase_biases::MASK_L1_CA, 0x08U);
    EXPECT_EQ(glonass_code_phase_biases::MASK_L1_P, 0x04U);
    EXPECT_EQ(glonass_code_phase_biases::MASK_L2_CA, 0x02U);
    EXPECT_EQ(glonass_code_phase_biases::MASK_L2_P, 0x01U);
    EXPECT_DOUBLE_EQ(glonass_code_phase_biases::RESOLUTION_M, 0.02);
}

TEST(RtcmMessageGlonassCodePhaseBiases, AcceptsOnly1230)
{
    static_assert(glonass_code_phase_biases::MESSAGE_TYPE == RTCM_TYPE_GLO_BIASES, "1230 model");
    EXPECT_TRUE(glonass_code_phase_biases::accepts(1230U));
    EXPECT_TRUE(glonass_code_phase_biases::accepts(RTCM_TYPE_GLO_BIASES));
    EXPECT_FALSE(glonass_code_phase_biases::accepts(1229U));
    EXPECT_FALSE(glonass_code_phase_biases::accepts(1231U));
    EXPECT_FALSE(glonass_code_phase_biases::accepts(0U));
    EXPECT_FALSE(glonass_code_phase_biases::accepts(0xFFFFU));
}

TEST(RtcmMessageGlonassCodePhaseBiases, EncodesGoldenFrame)
{
    glonass_code_phase_biases message{};
    message.station_id = 1234U;
    message.code_phase_bias_indicator = true;
    message.signals_mask = glonass_code_phase_biases::MASK_L1_CA | glonass_code_phase_biases::MASK_L1_P
                         | glonass_code_phase_biases::MASK_L2_P;
    message.l1_ca_bias = -100;
    message.l1_p_bias = 200;
    message.l2_p_bias = -32768;
    message.l2_ca_bias = 1234; // masked out, must not appear on the wire
    expect_frame(message, GOLDEN_1230);
}

TEST(RtcmMessageGlonassCodePhaseBiases, EveryMaskCombinationWritesOnlySelectedBiases)
{
    for (uint8_t mask = 0U; mask < 16U; ++mask)
    {
        frame_buffer frame;
        ASSERT_EQ(encode(all_biases(mask), frame), castle::status::ok) << "mask " << static_cast<int>(mask);
        EXPECT_EQ(to_hex(frame), std::string(MASK_FRAMES[mask])) << "mask " << static_cast<int>(mask);
        EXPECT_EQ(frame.data[2U], 4U + (2U * popcount4(mask))) << "mask " << static_cast<int>(mask);
        EXPECT_EQ(frame.size, frame_size(4U + (2U * popcount4(mask)))) << "mask " << static_cast<int>(mask);
    }
}

TEST(RtcmMessageGlonassCodePhaseBiases, UnselectedBiasValuesNeverChangeTheFrame)
{
    for (uint8_t mask = 0U; mask < 16U; ++mask)
    {
        glonass_code_phase_biases message = all_biases(mask);
        // Overwrite exactly the values whose mask bit is clear with different numbers.
        if ((mask & glonass_code_phase_biases::MASK_L1_CA) == 0U) { message.l1_ca_bias = 12345; }
        if ((mask & glonass_code_phase_biases::MASK_L1_P) == 0U) { message.l1_p_bias = -12345; }
        if ((mask & glonass_code_phase_biases::MASK_L2_CA) == 0U) { message.l2_ca_bias = 777; }
        if ((mask & glonass_code_phase_biases::MASK_L2_P) == 0U) { message.l2_p_bias = -777; }
        expect_frame(message, MASK_FRAMES[mask]);
    }
}

TEST(RtcmMessageGlonassCodePhaseBiases, Int16Extremes)
{
    glonass_code_phase_biases maximum{};
    maximum.station_id = 4095U;
    maximum.code_phase_bias_indicator = true;
    maximum.signals_mask = 15U;
    maximum.l1_ca_bias = INT16_MAX;
    maximum.l1_p_bias = INT16_MAX;
    maximum.l2_ca_bias = INT16_MAX;
    maximum.l2_p_bias = INT16_MAX;
    expect_frame(maximum, MAX_BIASES);

    glonass_code_phase_biases minimum{};
    minimum.station_id = 4095U;
    minimum.signals_mask = 15U;
    minimum.l1_ca_bias = INT16_MIN;
    minimum.l1_p_bias = INT16_MIN;
    minimum.l2_ca_bias = INT16_MIN;
    minimum.l2_p_bias = INT16_MIN;
    expect_frame(minimum, MIN_BIASES);
}

TEST(RtcmMessageGlonassCodePhaseBiases, EncodesZeroAndIndicatorClearMessages)
{
    expect_frame(glonass_code_phase_biases{}, ZERO_1230);

    glonass_code_phase_biases message{};
    message.station_id = 7U;
    message.signals_mask = glonass_code_phase_biases::MASK_L1_CA;
    message.l1_ca_bias = 1;
    expect_frame(message, INDICATOR_CLEAR);
}

TEST(RtcmMessageGlonassCodePhaseBiases, MaskAboveFourBitsIsOutOfRange)
{
    const uint8_t invalid[] = {16U, 17U, 0x1FU, 0x80U, 0xFFU};
    for (const uint8_t mask : invalid)
    {
        frame_buffer frame;
        frame.size = 99U;
        EXPECT_EQ(encode(all_biases(mask), frame), castle::status::out_of_range) << static_cast<int>(mask);
        EXPECT_EQ(frame.size, 0U);
    }
}

TEST(RtcmMessageGlonassCodePhaseBiases, StationIdBounds)
{
    frame_buffer frame;
    glonass_code_phase_biases message = all_biases(0U);

    message.station_id = 0U;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 1U], 0xE0U); // 1230 = 0x4CE: low nibble 0xE, id high nibble 0
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 2U], 0x00U);
    message.station_id = 4095U;
    ASSERT_EQ(encode(message, frame), castle::status::ok);
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 1U], 0xEFU);
    EXPECT_EQ(frame.data[RTCM_FRAME_INDEX_PAYLOAD + 2U], 0xFFU);

    message.station_id = 4096U;
    frame.size = 99U;
    EXPECT_EQ(encode(message, frame), castle::status::out_of_range);
    EXPECT_EQ(frame.size, 0U);
}

TEST(RtcmMessageGlonassCodePhaseBiases, ExactFitAndOneByteShortBuffers)
{
    const glonass_code_phase_biases message = all_biases(15U);
    const castle::size_type exact = frame_size(12U); // 4 + 4 * 2 payload bytes
    uint8_t buffer[64U] = {};
    castle::size_type written = 99U;

    ASSERT_EQ(encode_message(message, buffer, exact, written), castle::status::ok);
    EXPECT_EQ(written, exact);

    written = 99U;
    EXPECT_EQ(encode_message(message, buffer, exact - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    // A shorter mask fits the smaller buffer.
    EXPECT_EQ(encode_message(all_biases(0U), buffer, frame_size(4U), written), castle::status::ok);
    EXPECT_EQ(written, frame_size(4U));
    EXPECT_EQ(encode_message(all_biases(0U), buffer, frame_size(4U) - 1U, written), castle::status::full);

    EXPECT_EQ(encode_message(message, buffer, 0U, written), castle::status::full);
    EXPECT_EQ(encode_message(message, nullptr, sizeof(buffer), written), castle::status::invalid_argument);
    EXPECT_EQ(written, 0U);
}

} // namespace

#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_hw2.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(CASTLE_CONST uint8_t* payload, uint16_t length, uint8_t msg_class = UBX_CLASS_MON,
                     uint8_t msg_id = UBX_ID_MON_HW2)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

TEST(UbxMessageMonHw2, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_hw2::msg_class, 0x0AU);
    EXPECT_EQ(mon_hw2::msg_id, 0x0BU);
    EXPECT_EQ(mon_hw2::payload_length, 28U);
    EXPECT_EQ(static_cast<uint8_t>(mon_hw2_cfg_source::rom), 114U);
    EXPECT_EQ(static_cast<uint8_t>(mon_hw2_cfg_source::otp), 111U);
    EXPECT_EQ(static_cast<uint8_t>(mon_hw2_cfg_source::config_pins), 112U);
    EXPECT_EQ(static_cast<uint8_t>(mon_hw2_cfg_source::flash_image), 102U);
}

TEST(UbxMessageMonHw2, RoundTripThroughFrame)
{
    mon_hw2 message{};
    message.ofs_i = -100;
    message.mag_i = 200U;
    message.ofs_q = 55;
    message.mag_q = 201U;
    message.cfg_source = mon_hw2_cfg_source::flash_image;
    message.low_lev_cfg = 0x11223344U;
    message.post_status = 0xA5A55A5AU;

    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(28U));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    ASSERT_TRUE(mon_hw2::matches(raw));

    mon_hw2 decoded{};
    ASSERT_TRUE(mon_hw2::decode(raw, decoded));
    EXPECT_EQ(decoded.ofs_i, -100);
    EXPECT_EQ(decoded.mag_i, 200U);
    EXPECT_EQ(decoded.ofs_q, 55);
    EXPECT_EQ(decoded.mag_q, 201U);
    EXPECT_EQ(decoded.cfg_source, mon_hw2_cfg_source::flash_image);
    EXPECT_EQ(decoded.low_lev_cfg, 0x11223344U);
    EXPECT_EQ(decoded.post_status, 0xA5A55A5AU);

    for (castle::size_type offset : {5U, 6U, 7U, 12U, 19U, 24U, 27U})
    {
        EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + offset], 0U) << "reserved offset " << offset;
    }
}

TEST(UbxMessageMonHw2, HandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[28U] = {};
    for (uint8_t& byte : payload)
    {
        byte = 0xEEU; // reserved bytes carry garbage, must be ignored
    }
    payload[0U] = 0x80U; // ofsI = -128
    payload[1U] = 0x7FU;
    payload[2U] = 0x7FU; // ofsQ = 127
    payload[3U] = 0x01U;
    payload[4U] = 111U;  // OTP
    castle::write_le32(&payload[8U], 0xCAFEBABEU);
    castle::write_le32(&payload[20U], 0x01020304U);

    mon_hw2 decoded{};
    ASSERT_TRUE(mon_hw2::decode(view_of(payload, 28U), decoded));
    EXPECT_EQ(decoded.ofs_i, -128);
    EXPECT_EQ(decoded.mag_i, 0x7FU);
    EXPECT_EQ(decoded.ofs_q, 127);
    EXPECT_EQ(decoded.mag_q, 0x01U);
    EXPECT_EQ(decoded.cfg_source, mon_hw2_cfg_source::otp);
    EXPECT_EQ(decoded.low_lev_cfg, 0xCAFEBABEU);
    EXPECT_EQ(decoded.post_status, 0x01020304U);
}

TEST(UbxMessageMonHw2, RejectsWrongClassOrId)
{
    uint8_t payload[28U] = {};
    payload[1U] = 9U;
    mon_hw2 decoded{};
    EXPECT_FALSE(mon_hw2::matches(view_of(payload, 28U, UBX_CLASS_NAV, UBX_ID_MON_HW2)));
    EXPECT_FALSE(mon_hw2::decode(view_of(payload, 28U, UBX_CLASS_NAV, UBX_ID_MON_HW2), decoded));
    EXPECT_FALSE(mon_hw2::matches(view_of(payload, 28U, UBX_CLASS_MON, UBX_ID_MON_HW)));
    EXPECT_FALSE(mon_hw2::decode(view_of(payload, 28U, UBX_CLASS_MON, UBX_ID_MON_HW), decoded));
    EXPECT_EQ(decoded.mag_i, 0U);
}

TEST(UbxMessageMonHw2, RejectsShortAndLongPayloads)
{
    uint8_t payload[29U] = {};
    payload[1U] = 9U;
    mon_hw2 decoded{};
    EXPECT_FALSE(mon_hw2::decode(view_of(payload, 27U), decoded));
    EXPECT_EQ(decoded.mag_i, 0U);
    EXPECT_FALSE(mon_hw2::decode(view_of(payload, 29U), decoded));
    EXPECT_EQ(decoded.mag_i, 0U);
}

TEST(UbxMessageMonHw2, EncodeIntoTooSmallBufferReportsFull)
{
    mon_hw2 message{};
    uint8_t frame[frame_size(27U)] = {};
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace

#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_hw.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(CASTLE_CONST uint8_t* payload, uint16_t length, uint8_t msg_class = UBX_CLASS_MON,
                     uint8_t msg_id = UBX_ID_MON_HW)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

mon_hw distinctive()
{
    mon_hw message{};
    message.pin_sel = 0x01020304U;
    message.pin_bank = 0x05060708U;
    message.pin_dir = 0x090A0B0CU;
    message.pin_val = 0x0D0E0F10U;
    message.noise_per_ms = 0x1112U;
    message.agc_cnt = 0x1314U;
    message.a_status = mon_antenna_status::short_circuit;
    message.a_power = mon_antenna_power::dont_know;
    message.flags = 0x1BU;
    message.used_mask = 0x15161718U;
    for (castle::size_type index = 0U; index < mon_hw::VP_COUNT; ++index)
    {
        message.vp[index] = static_cast<uint8_t>(0x20U + index);
    }
    message.jam_ind = 0x99U;
    message.pin_irq = 0x31323334U;
    message.pull_h = 0x35363738U;
    message.pull_l = 0x393A3B3CU;
    return message;
}

TEST(UbxMessageMonHw, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_hw::msg_class, 0x0AU);
    EXPECT_EQ(mon_hw::msg_id, 0x09U);
    EXPECT_EQ(mon_hw::payload_length, 60U);
    EXPECT_EQ(mon_hw::VP_COUNT, 17U);
    EXPECT_EQ(static_cast<uint8_t>(mon_antenna_status::init), 0U);
    EXPECT_EQ(static_cast<uint8_t>(mon_antenna_status::dont_know), 1U);
    EXPECT_EQ(static_cast<uint8_t>(mon_antenna_status::ok), 2U);
    EXPECT_EQ(static_cast<uint8_t>(mon_antenna_status::short_circuit), 3U);
    EXPECT_EQ(static_cast<uint8_t>(mon_antenna_status::open), 4U);
    EXPECT_EQ(static_cast<uint8_t>(mon_antenna_power::off), 0U);
    EXPECT_EQ(static_cast<uint8_t>(mon_antenna_power::on), 1U);
    EXPECT_EQ(static_cast<uint8_t>(mon_antenna_power::dont_know), 2U);
}

TEST(UbxMessageMonHw, RoundTripThroughFrame)
{
    CASTLE_CONST mon_hw message = distinctive();

    uint8_t frame[96U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(60U));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 23U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 46U], 0U);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 47U], 0U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    ASSERT_TRUE(mon_hw::matches(raw));

    mon_hw decoded{};
    ASSERT_TRUE(mon_hw::decode(raw, decoded));
    EXPECT_EQ(decoded.pin_sel, message.pin_sel);
    EXPECT_EQ(decoded.pin_bank, message.pin_bank);
    EXPECT_EQ(decoded.pin_dir, message.pin_dir);
    EXPECT_EQ(decoded.pin_val, message.pin_val);
    EXPECT_EQ(decoded.noise_per_ms, message.noise_per_ms);
    EXPECT_EQ(decoded.agc_cnt, message.agc_cnt);
    EXPECT_EQ(decoded.a_status, mon_antenna_status::short_circuit);
    EXPECT_EQ(decoded.a_power, mon_antenna_power::dont_know);
    EXPECT_EQ(decoded.flags, 0x1BU);
    EXPECT_EQ(decoded.used_mask, message.used_mask);
    for (castle::size_type index = 0U; index < mon_hw::VP_COUNT; ++index)
    {
        EXPECT_EQ(decoded.vp[index], message.vp[index]) << "vp " << index;
    }
    EXPECT_EQ(decoded.jam_ind, 0x99U);
    EXPECT_EQ(decoded.pin_irq, message.pin_irq);
    EXPECT_EQ(decoded.pull_h, message.pull_h);
    EXPECT_EQ(decoded.pull_l, message.pull_l);
}

TEST(UbxMessageMonHw, HandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[60U] = {};
    castle::write_le32(&payload[0U], 0xA0000001U);
    castle::write_le32(&payload[4U], 0xA0000002U);
    castle::write_le32(&payload[8U], 0xA0000003U);
    castle::write_le32(&payload[12U], 0xA0000004U);
    castle::write_le16(&payload[16U], 0x0B05U);
    castle::write_le16(&payload[18U], 0x1FFFU);
    payload[20U] = 4U; // OPEN
    payload[21U] = 1U; // ON
    payload[22U] = 0x08U; // jammingState = 2 (warning)
    payload[23U] = 0xEEU; // reserved
    castle::write_le32(&payload[24U], 0xA0000005U);
    for (castle::size_type index = 0U; index < 17U; ++index)
    {
        payload[28U + index] = static_cast<uint8_t>(0x40U + index);
    }
    payload[45U] = 0x77U;
    payload[46U] = 0xEEU; // reserved
    payload[47U] = 0xEEU; // reserved
    castle::write_le32(&payload[48U], 0xA0000006U);
    castle::write_le32(&payload[52U], 0xA0000007U);
    castle::write_le32(&payload[56U], 0xA0000008U);

    mon_hw decoded{};
    ASSERT_TRUE(mon_hw::decode(view_of(payload, 60U), decoded));
    EXPECT_EQ(decoded.pin_sel, 0xA0000001U);
    EXPECT_EQ(decoded.pin_bank, 0xA0000002U);
    EXPECT_EQ(decoded.pin_dir, 0xA0000003U);
    EXPECT_EQ(decoded.pin_val, 0xA0000004U);
    EXPECT_EQ(decoded.noise_per_ms, 0x0B05U);
    EXPECT_EQ(decoded.agc_cnt, 0x1FFFU);
    EXPECT_EQ(decoded.a_status, mon_antenna_status::open);
    EXPECT_EQ(decoded.a_power, mon_antenna_power::on);
    EXPECT_EQ(decoded.flags, 0x08U);
    EXPECT_EQ(decoded.jamming_state(), mon_jamming_state::warning);
    EXPECT_EQ(decoded.used_mask, 0xA0000005U);
    EXPECT_EQ(decoded.vp[0U], 0x40U);
    EXPECT_EQ(decoded.vp[16U], 0x50U);
    EXPECT_EQ(decoded.jam_ind, 0x77U);
    EXPECT_EQ(decoded.pin_irq, 0xA0000006U);
    EXPECT_EQ(decoded.pull_h, 0xA0000007U);
    EXPECT_EQ(decoded.pull_l, 0xA0000008U);
}

TEST(UbxMessageMonHw, FlagAccessorsSetAndClear)
{
    mon_hw message{};
    EXPECT_FALSE(message.rtc_calib());
    EXPECT_FALSE(message.safe_boot());
    EXPECT_FALSE(message.xtal_absent());
    EXPECT_EQ(message.jamming_state(), mon_jamming_state::unknown);

    message.flags = mon_hw::FLAGS_RTC_CALIB;
    EXPECT_TRUE(message.rtc_calib());
    EXPECT_FALSE(message.safe_boot());

    message.flags = mon_hw::FLAGS_SAFE_BOOT;
    EXPECT_TRUE(message.safe_boot());
    EXPECT_FALSE(message.rtc_calib());

    message.flags = mon_hw::FLAGS_XTAL_ABSENT;
    EXPECT_TRUE(message.xtal_absent());
    EXPECT_EQ(message.jamming_state(), mon_jamming_state::unknown);

    message.flags = 0x04U;
    EXPECT_EQ(message.jamming_state(), mon_jamming_state::ok);
    message.flags = 0x08U;
    EXPECT_EQ(message.jamming_state(), mon_jamming_state::warning);
    message.flags = static_cast<uint8_t>(mon_hw::FLAGS_JAMMING_STATE | mon_hw::FLAGS_RTC_CALIB | mon_hw::FLAGS_XTAL_ABSENT);
    EXPECT_EQ(message.jamming_state(), mon_jamming_state::critical);
    EXPECT_TRUE(message.rtc_calib());
    EXPECT_FALSE(message.safe_boot());
    EXPECT_TRUE(message.xtal_absent());
}

TEST(UbxMessageMonHw, RejectsWrongClassOrId)
{
    uint8_t payload[60U] = {};
    payload[0U] = 1U;
    mon_hw decoded{};
    EXPECT_FALSE(mon_hw::matches(view_of(payload, 60U, UBX_CLASS_NAV, UBX_ID_MON_HW)));
    EXPECT_FALSE(mon_hw::decode(view_of(payload, 60U, UBX_CLASS_NAV, UBX_ID_MON_HW), decoded));
    EXPECT_FALSE(mon_hw::matches(view_of(payload, 60U, UBX_CLASS_MON, UBX_ID_MON_HW2)));
    EXPECT_FALSE(mon_hw::decode(view_of(payload, 60U, UBX_CLASS_MON, UBX_ID_MON_HW2), decoded));
    EXPECT_EQ(decoded.pin_sel, 0U);
}

TEST(UbxMessageMonHw, RejectsShortAndLongPayloads)
{
    uint8_t payload[61U] = {};
    payload[0U] = 1U;
    mon_hw decoded{};
    EXPECT_FALSE(mon_hw::decode(view_of(payload, 59U), decoded));
    EXPECT_EQ(decoded.pin_sel, 0U);
    EXPECT_FALSE(mon_hw::decode(view_of(payload, 61U), decoded));
    EXPECT_EQ(decoded.pin_sel, 0U);
}

TEST(UbxMessageMonHw, EncodeIntoTooSmallBufferReportsFull)
{
    CASTLE_CONST mon_hw message = distinctive();
    uint8_t frame[frame_size(59U)] = {};
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace

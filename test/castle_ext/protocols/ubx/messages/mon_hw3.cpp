#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/mon_hw3.hpp"

#include <string.h>

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(CASTLE_CONST uint8_t* payload, uint16_t length, uint8_t msg_class = UBX_CLASS_MON,
                     uint8_t msg_id = UBX_ID_MON_HW3)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

mon_hw3_pin make_pin(uint16_t id, uint16_t mask, uint8_t vp)
{
    mon_hw3_pin pin{};
    pin.pin_id = id;
    pin.pin_mask = mask;
    pin.vp = vp;
    return pin;
}

TEST(UbxMessageMonHw3, ConstantsMatchSpec)
{
    EXPECT_EQ(mon_hw3<>::msg_class, 0x0AU);
    EXPECT_EQ(mon_hw3<>::msg_id, 0x37U);
    EXPECT_EQ(mon_hw3<>::header_length, 22U);
    EXPECT_EQ(mon_hw3<>::block_length, 6U);
    EXPECT_EQ(mon_hw3<>::HW_VERSION_LENGTH, 10U);
}

TEST(UbxMessageMonHw3, RoundTripThroughFrame)
{
    mon_hw3<> message{};
    message.version = 0x00U;
    message.flags = 0x07U;
    ASSERT_EQ(message.hw_version.assign("00190000"), castle::status::ok);
    message.n_pins = 77U; // ignored by encode
    ASSERT_EQ(message.pins.push_back(make_pin(0x0102U, 0x03FFU, 0x11U)), castle::status::ok);
    ASSERT_EQ(message.pins.push_back(make_pin(0x0304U, 0x0155U, 0x22U)), castle::status::ok);

    uint8_t frame[64U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(22U + (2U * 6U)));
    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(payload[1U], 2U);
    EXPECT_EQ(memcmp(&payload[3U], "00190000\0\0", 10U), 0);
    for (castle::size_type offset = 13U; offset < 22U; ++offset)
    {
        EXPECT_EQ(payload[offset], 0U) << "reserved0 offset " << offset;
    }
    EXPECT_EQ(payload[22U + 5U], 0U);

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    ASSERT_TRUE(mon_hw3<>::matches(raw));

    mon_hw3<> decoded{};
    ASSERT_TRUE(mon_hw3<>::decode(raw, decoded));
    EXPECT_EQ(decoded.version, 0x00U);
    EXPECT_EQ(decoded.n_pins, 2U);
    EXPECT_EQ(decoded.flags, 0x07U);
    EXPECT_STREQ(decoded.hw_version.c_str(), "00190000");
    ASSERT_EQ(decoded.pins.size(), 2U);
    EXPECT_EQ(decoded.pins[0U].pin_id, 0x0102U);
    EXPECT_EQ(decoded.pins[0U].pin_mask, 0x03FFU);
    EXPECT_EQ(decoded.pins[0U].vp, 0x11U);
    EXPECT_EQ(decoded.pins[1U].pin_id, 0x0304U);
    EXPECT_EQ(decoded.pins[1U].pin_mask, 0x0155U);
    EXPECT_EQ(decoded.pins[1U].vp, 0x22U);
}

TEST(UbxMessageMonHw3, HandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[22U + 6U] = {};
    payload[0U] = 0x00U;
    payload[1U] = 1U;
    payload[2U] = 0x05U; // rtcCalib + xtalAbsent
    memcpy(&payload[3U], "ABCDEFGHIJ", 10U); // full width, no terminator
    for (castle::size_type offset = 13U; offset < 22U; ++offset)
    {
        payload[offset] = 0xEEU; // reserved0
    }
    castle::write_le16(&payload[22U], 0xBEEFU);
    castle::write_le16(&payload[24U], 0x02ABU);
    payload[26U] = 0x5AU;
    payload[27U] = 0xEEU; // reserved1

    mon_hw3<> decoded{};
    ASSERT_TRUE(mon_hw3<>::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.version, 0x00U);
    EXPECT_EQ(decoded.n_pins, 1U);
    EXPECT_EQ(decoded.flags, 0x05U);
    EXPECT_TRUE(decoded.rtc_calib());
    EXPECT_FALSE(decoded.safe_boot());
    EXPECT_TRUE(decoded.xtal_absent());
    EXPECT_EQ(decoded.hw_version.size(), 10U);
    EXPECT_STREQ(decoded.hw_version.c_str(), "ABCDEFGHIJ");
    ASSERT_EQ(decoded.pins.size(), 1U);
    EXPECT_EQ(decoded.pins[0U].pin_id, 0xBEEFU);
    EXPECT_EQ(decoded.pins[0U].pin_mask, 0x02ABU);
    EXPECT_EQ(decoded.pins[0U].vp, 0x5AU);
}

TEST(UbxMessageMonHw3, HwVersionStopsAtFirstNul)
{
    uint8_t payload[22U] = {};
    memcpy(&payload[3U], "XY\0Z", 4U);
    mon_hw3<> decoded{};
    ASSERT_TRUE(mon_hw3<>::decode(view_of(payload, 22U), decoded));
    EXPECT_EQ(decoded.n_pins, 0U);
    EXPECT_TRUE(decoded.pins.empty());
    EXPECT_STREQ(decoded.hw_version.c_str(), "XY");
}

TEST(UbxMessageMonHw3, HeaderFlagAccessorsSetAndClear)
{
    mon_hw3<> message{};
    EXPECT_FALSE(message.rtc_calib());
    EXPECT_FALSE(message.safe_boot());
    EXPECT_FALSE(message.xtal_absent());
    message.flags = mon_hw3<>::FLAGS_SAFE_BOOT;
    EXPECT_FALSE(message.rtc_calib());
    EXPECT_TRUE(message.safe_boot());
    EXPECT_FALSE(message.xtal_absent());
    message.flags = static_cast<uint8_t>(mon_hw3<>::FLAGS_RTC_CALIB | mon_hw3<>::FLAGS_XTAL_ABSENT);
    EXPECT_TRUE(message.rtc_calib());
    EXPECT_FALSE(message.safe_boot());
    EXPECT_TRUE(message.xtal_absent());
}

TEST(UbxMessageMonHw3, PinMaskAccessorsSetAndClear)
{
    mon_hw3_pin pin{};
    EXPECT_FALSE(pin.periph_pio());
    EXPECT_EQ(pin.pin_bank(), 0U);
    EXPECT_FALSE(pin.direction());
    EXPECT_FALSE(pin.value());
    EXPECT_FALSE(pin.vp_manager());
    EXPECT_FALSE(pin.pio_irq());
    EXPECT_FALSE(pin.pio_pull_high());
    EXPECT_FALSE(pin.pio_pull_low());

    pin.pin_mask = 0x03FFU;
    EXPECT_TRUE(pin.periph_pio());
    EXPECT_EQ(pin.pin_bank(), 7U);
    EXPECT_TRUE(pin.direction());
    EXPECT_TRUE(pin.value());
    EXPECT_TRUE(pin.vp_manager());
    EXPECT_TRUE(pin.pio_irq());
    EXPECT_TRUE(pin.pio_pull_high());
    EXPECT_TRUE(pin.pio_pull_low());

    pin.pin_mask = static_cast<uint16_t>(0x0004U | mon_hw3_pin::PIN_MASK_VALUE | mon_hw3_pin::PIN_MASK_PIO_PULL_LOW);
    EXPECT_FALSE(pin.periph_pio());
    EXPECT_EQ(pin.pin_bank(), 2U); // bank C
    EXPECT_FALSE(pin.direction());
    EXPECT_TRUE(pin.value());
    EXPECT_FALSE(pin.vp_manager());
    EXPECT_FALSE(pin.pio_irq());
    EXPECT_FALSE(pin.pio_pull_high());
    EXPECT_TRUE(pin.pio_pull_low());

    pin.pin_mask = static_cast<uint16_t>(mon_hw3_pin::PIN_MASK_PERIPH_PIO | mon_hw3_pin::PIN_MASK_DIRECTION |
                                         mon_hw3_pin::PIN_MASK_VP_MANAGER | mon_hw3_pin::PIN_MASK_PIO_IRQ |
                                         mon_hw3_pin::PIN_MASK_PIO_PULL_HIGH);
    EXPECT_TRUE(pin.periph_pio());
    EXPECT_EQ(pin.pin_bank(), 0U);
    EXPECT_TRUE(pin.direction());
    EXPECT_FALSE(pin.value());
    EXPECT_TRUE(pin.vp_manager());
    EXPECT_TRUE(pin.pio_irq());
    EXPECT_TRUE(pin.pio_pull_high());
    EXPECT_FALSE(pin.pio_pull_low());
}

TEST(UbxMessageMonHw3, CapacityOverflowKeepsWireCountAndStoresPrefix)
{
    uint8_t payload[22U + (3U * 6U)] = {};
    payload[1U] = 3U;
    for (castle::size_type index = 0U; index < 3U; ++index)
    {
        castle::write_le16(&payload[22U + (index * 6U)], static_cast<uint16_t>(0x100U + index));
    }
    mon_hw3<2U> decoded{};
    ASSERT_TRUE(mon_hw3<2U>::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.n_pins, 3U);
    ASSERT_EQ(decoded.pins.size(), 2U);
    EXPECT_EQ(decoded.pins[0U].pin_id, 0x100U);
    EXPECT_EQ(decoded.pins[1U].pin_id, 0x101U);
}

TEST(UbxMessageMonHw3, RejectsGroupLengthMismatch)
{
    uint8_t payload[22U + 13U] = {};
    payload[1U] = 2U;
    payload[2U] = 0x01U;
    mon_hw3<> decoded{};
    EXPECT_FALSE(mon_hw3<>::decode(view_of(payload, 22U + 6U), decoded));
    EXPECT_EQ(decoded.n_pins, 0U);
    EXPECT_EQ(decoded.flags, 0U);
    EXPECT_FALSE(mon_hw3<>::decode(view_of(payload, 22U + 13U), decoded));
    EXPECT_EQ(decoded.flags, 0U);
}

TEST(UbxMessageMonHw3, RejectsShortHeaderAndWrongClassOrId)
{
    uint8_t payload[22U] = {};
    mon_hw3<> decoded{};
    EXPECT_FALSE(mon_hw3<>::decode(view_of(payload, 21U), decoded));
    EXPECT_FALSE(mon_hw3<>::matches(view_of(payload, 22U, UBX_CLASS_NAV, UBX_ID_MON_HW3)));
    EXPECT_FALSE(mon_hw3<>::decode(view_of(payload, 22U, UBX_CLASS_NAV, UBX_ID_MON_HW3), decoded));
    EXPECT_FALSE(mon_hw3<>::matches(view_of(payload, 22U, UBX_CLASS_MON, UBX_ID_MON_RF)));
    EXPECT_FALSE(mon_hw3<>::decode(view_of(payload, 22U, UBX_CLASS_MON, UBX_ID_MON_RF), decoded));
}

TEST(UbxMessageMonHw3, EncodeIntoTooSmallBufferReportsFull)
{
    mon_hw3<> message{};
    ASSERT_EQ(message.pins.push_back(make_pin(1U, 2U, 3U)), castle::status::ok);
    uint8_t frame[frame_size(22U + 5U)] = {};
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace

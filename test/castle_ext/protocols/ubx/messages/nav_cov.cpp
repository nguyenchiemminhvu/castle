#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_cov.hpp"
#include "castle/utility/bit_cast.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_cov;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

void write_r4(uint8_t* target, float value)
{
    castle::write_le32(target, castle::bit_cast<uint32_t>(value));
}

float read_r4(const uint8_t* source)
{
    return castle::bit_cast<float>(castle::read_le32(source));
}

message distinctive()
{
    message value{};
    value.i_tow = 0x11223344U;
    value.version = 0x5AU;
    value.pos_cov_valid = 0x01U;
    value.vel_cov_valid = 0x02U;
    value.pos_cov_nn = 1.0F;
    value.pos_cov_ne = -2.0F;
    value.pos_cov_nd = 3.5F;
    value.pos_cov_ee = -4.25F;
    value.pos_cov_ed = 5.125F;
    value.pos_cov_dd = -6.0625F;
    value.vel_cov_nn = 7.5F;
    value.vel_cov_ne = -8.75F;
    value.vel_cov_nd = 9.375F;
    value.vel_cov_ee = -10.5F;
    value.vel_cov_ed = 11.25F;
    value.vel_cov_dd = -12.125F;
    return value;
}

void expect_default(const message& value)
{
    EXPECT_EQ(value.i_tow, 0U);
    EXPECT_EQ(value.version, 0U);
    EXPECT_EQ(value.pos_cov_valid, 0U);
    EXPECT_EQ(value.vel_cov_valid, 0U);
    EXPECT_EQ(value.pos_cov_nn, 0.0F);
    EXPECT_EQ(value.pos_cov_ne, 0.0F);
    EXPECT_EQ(value.pos_cov_nd, 0.0F);
    EXPECT_EQ(value.pos_cov_ee, 0.0F);
    EXPECT_EQ(value.pos_cov_ed, 0.0F);
    EXPECT_EQ(value.pos_cov_dd, 0.0F);
    EXPECT_EQ(value.vel_cov_nn, 0.0F);
    EXPECT_EQ(value.vel_cov_ne, 0.0F);
    EXPECT_EQ(value.vel_cov_nd, 0.0F);
    EXPECT_EQ(value.vel_cov_ee, 0.0F);
    EXPECT_EQ(value.vel_cov_ed, 0.0F);
    EXPECT_EQ(value.vel_cov_dd, 0.0F);
}

TEST(UbxMessageNavCov, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_NAV);
    EXPECT_EQ(message::msg_id, UBX_ID_NAV_COV);
    EXPECT_EQ(message::payload_length, 64U);
}

TEST(UbxMessageNavCov, RoundTripPreservesEveryField)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(message::matches(raw));

    message decoded{};
    ASSERT_TRUE(message::decode(raw, decoded));
    EXPECT_EQ(decoded.i_tow, original.i_tow);
    EXPECT_EQ(decoded.version, original.version);
    EXPECT_EQ(decoded.pos_cov_valid, original.pos_cov_valid);
    EXPECT_EQ(decoded.vel_cov_valid, original.vel_cov_valid);
    EXPECT_EQ(decoded.pos_cov_nn, original.pos_cov_nn);
    EXPECT_EQ(decoded.pos_cov_ne, original.pos_cov_ne);
    EXPECT_EQ(decoded.pos_cov_nd, original.pos_cov_nd);
    EXPECT_EQ(decoded.pos_cov_ee, original.pos_cov_ee);
    EXPECT_EQ(decoded.pos_cov_ed, original.pos_cov_ed);
    EXPECT_EQ(decoded.pos_cov_dd, original.pos_cov_dd);
    EXPECT_EQ(decoded.vel_cov_nn, original.vel_cov_nn);
    EXPECT_EQ(decoded.vel_cov_ne, original.vel_cov_ne);
    EXPECT_EQ(decoded.vel_cov_nd, original.vel_cov_nd);
    EXPECT_EQ(decoded.vel_cov_ee, original.vel_cov_ee);
    EXPECT_EQ(decoded.vel_cov_ed, original.vel_cov_ed);
    EXPECT_EQ(decoded.vel_cov_dd, original.vel_cov_dd);
}

TEST(UbxMessageNavCov, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[message::payload_length] = {};
    castle::write_le32(&payload[0U], 504000123U);
    payload[4U] = 0x00U;
    payload[5U] = 0x01U;
    payload[6U] = 0x00U;
    for (castle::size_type index = 7U; index < 16U; ++index)
    {
        payload[index] = 0xEEU; // reserved0 must be skipped, whatever its content.
    }
    for (castle::size_type slot = 0U; slot < 12U; ++slot)
    {
        write_r4(&payload[16U + (slot * 4U)], static_cast<float>(slot + 1U) * 0.5F);
    }

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.i_tow, 504000123U);
    EXPECT_EQ(decoded.version, 0x00U);
    EXPECT_EQ(decoded.pos_cov_valid, 0x01U);
    EXPECT_EQ(decoded.vel_cov_valid, 0x00U);
    EXPECT_EQ(decoded.pos_cov_nn, 0.5F);
    EXPECT_EQ(decoded.pos_cov_ne, 1.0F);
    EXPECT_EQ(decoded.pos_cov_nd, 1.5F);
    EXPECT_EQ(decoded.pos_cov_ee, 2.0F);
    EXPECT_EQ(decoded.pos_cov_ed, 2.5F);
    EXPECT_EQ(decoded.pos_cov_dd, 3.0F);
    EXPECT_EQ(decoded.vel_cov_nn, 3.5F);
    EXPECT_EQ(decoded.vel_cov_ne, 4.0F);
    EXPECT_EQ(decoded.vel_cov_nd, 4.5F);
    EXPECT_EQ(decoded.vel_cov_ee, 5.0F);
    EXPECT_EQ(decoded.vel_cov_ed, 5.5F);
    EXPECT_EQ(decoded.vel_cov_dd, 6.0F);
}

TEST(UbxMessageNavCov, EncodesFieldsAtSpecOffsetsWithZeroReserved)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(castle::read_le32(&payload[0U]), original.i_tow);
    EXPECT_EQ(payload[4U], original.version);
    EXPECT_EQ(payload[5U], original.pos_cov_valid);
    EXPECT_EQ(payload[6U], original.vel_cov_valid);
    for (castle::size_type index = 7U; index < 16U; ++index)
    {
        EXPECT_EQ(payload[index], 0U) << "reserved byte " << index;
    }
    EXPECT_EQ(read_r4(&payload[16U]), original.pos_cov_nn);
    EXPECT_EQ(read_r4(&payload[20U]), original.pos_cov_ne);
    EXPECT_EQ(read_r4(&payload[24U]), original.pos_cov_nd);
    EXPECT_EQ(read_r4(&payload[28U]), original.pos_cov_ee);
    EXPECT_EQ(read_r4(&payload[32U]), original.pos_cov_ed);
    EXPECT_EQ(read_r4(&payload[36U]), original.pos_cov_dd);
    EXPECT_EQ(read_r4(&payload[40U]), original.vel_cov_nn);
    EXPECT_EQ(read_r4(&payload[44U]), original.vel_cov_ne);
    EXPECT_EQ(read_r4(&payload[48U]), original.vel_cov_nd);
    EXPECT_EQ(read_r4(&payload[52U]), original.vel_cov_ee);
    EXPECT_EQ(read_r4(&payload[56U]), original.vel_cov_ed);
    EXPECT_EQ(read_r4(&payload[60U]), original.vel_cov_dd);
}

TEST(UbxMessageNavCov, RejectsWrongClassOrId)
{
    uint8_t payload[message::payload_length] = {};
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV2;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_NAV_EELL;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavCov, RejectsShortAndLongPayloads)
{
    uint8_t payload[message::payload_length + 1U] = {};
    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, message::payload_length + 1U), decoded));
    expect_default(decoded);
}

TEST(UbxMessageNavCov, EncodeIntoTooSmallBufferReportsFull)
{
    const message value = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + message::payload_length];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace

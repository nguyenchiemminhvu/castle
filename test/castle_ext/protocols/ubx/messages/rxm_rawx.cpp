#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/rxm_rawx.hpp"
#include "castle/utility/bytes.hpp"
#include "castle/utility/bit_cast.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = rxm_rawx<4U>;

constexpr castle::size_type HEADER = 16U;
constexpr castle::size_type BLOCK = 32U;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

uint64_t bits_of(double value)
{
    return castle::bit_cast<uint64_t>(value);
}

uint32_t bits_of(float value)
{
    return castle::bit_cast<uint32_t>(value);
}

rxm_rawx_meas make_meas(uint8_t seed)
{
    rxm_rawx_meas meas{};
    meas.pr_mes = 21000000.123456789 + static_cast<double>(seed);
    meas.cp_mes = -110345678.987654321 - static_cast<double>(seed);
    meas.do_mes = -1234.5F + static_cast<float>(seed);
    meas.gnss_id = static_cast<uint8_t>(seed + 1U);
    meas.sv_id = static_cast<uint8_t>(seed + 2U);
    meas.sig_id = static_cast<uint8_t>(seed + 3U);
    meas.freq_id = static_cast<uint8_t>(seed + 4U);
    meas.locktime = static_cast<uint16_t>(64500U - seed);
    meas.cno = static_cast<uint8_t>(seed + 5U);
    meas.pr_stdev = static_cast<uint8_t>(0x06U + seed);
    meas.cp_stdev = static_cast<uint8_t>(0x07U + seed);
    meas.do_stdev = static_cast<uint8_t>(0x08U + seed);
    meas.trk_stat = static_cast<uint8_t>(0x0FU - seed);
    return meas;
}

message distinctive()
{
    message value{};
    value.rcv_tow = 345600.123456789;
    value.week = 2345U;
    value.leap_s = -18;
    value.num_meas = 200U; // ignored on encode: count comes from meas.size()
    value.rec_stat = 0x03U;
    value.version = 0x01U;
    static_cast<void>(value.meas.push_back(make_meas(0U)));
    static_cast<void>(value.meas.push_back(make_meas(1U)));
    static_cast<void>(value.meas.push_back(make_meas(2U)));
    return value;
}

void expect_meas_eq(const rxm_rawx_meas& actual, const rxm_rawx_meas& expected)
{
    EXPECT_EQ(bits_of(actual.pr_mes), bits_of(expected.pr_mes));
    EXPECT_EQ(bits_of(actual.cp_mes), bits_of(expected.cp_mes));
    EXPECT_EQ(bits_of(actual.do_mes), bits_of(expected.do_mes));
    EXPECT_EQ(actual.gnss_id, expected.gnss_id);
    EXPECT_EQ(actual.sv_id, expected.sv_id);
    EXPECT_EQ(actual.sig_id, expected.sig_id);
    EXPECT_EQ(actual.freq_id, expected.freq_id);
    EXPECT_EQ(actual.locktime, expected.locktime);
    EXPECT_EQ(actual.cno, expected.cno);
    EXPECT_EQ(actual.pr_stdev, expected.pr_stdev);
    EXPECT_EQ(actual.cp_stdev, expected.cp_stdev);
    EXPECT_EQ(actual.do_stdev, expected.do_stdev);
    EXPECT_EQ(actual.trk_stat, expected.trk_stat);
}

template <castle::size_type N>
void expect_default(const rxm_rawx<N>& value)
{
    EXPECT_EQ(bits_of(value.rcv_tow), 0U);
    EXPECT_EQ(value.week, 0U);
    EXPECT_EQ(value.leap_s, 0);
    EXPECT_EQ(value.num_meas, 0U);
    EXPECT_EQ(value.rec_stat, 0U);
    EXPECT_EQ(value.version, 0U);
    EXPECT_TRUE(value.meas.empty());
}

TEST(UbxMessageRxmRawx, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_RXM);
    EXPECT_EQ(message::msg_id, UBX_ID_RXM_RAWX);
    EXPECT_EQ(message::msg_class, 0x02U);
    EXPECT_EQ(message::msg_id, 0x15U);
    EXPECT_EQ(message::header_length, HEADER);
    EXPECT_EQ(message::block_length, BLOCK);
    EXPECT_EQ(rxm_rawx_meas::wire_size, BLOCK);
    EXPECT_EQ(message::REC_STAT_LEAP_SEC, 0x01U);
    EXPECT_EQ(message::REC_STAT_CLK_RESET, 0x02U);
    EXPECT_EQ(rxm_rawx_meas::STDEV_INDEX, 0x0FU);
    EXPECT_EQ(rxm_rawx_meas::CP_STD_INVALID, 0x0FU);
    EXPECT_EQ(rxm_rawx_meas::TRK_STAT_PR_VALID, 0x01U);
    EXPECT_EQ(rxm_rawx_meas::TRK_STAT_CP_VALID, 0x02U);
    EXPECT_EQ(rxm_rawx_meas::TRK_STAT_HALF_CYC, 0x04U);
    EXPECT_EQ(rxm_rawx_meas::TRK_STAT_SUB_HALF_CYC, 0x08U);
    EXPECT_EQ(message{}.meas.capacity(), 4U);
    EXPECT_EQ(rxm_rawx<>{}.meas.capacity(), 64U);
}

TEST(UbxMessageRxmRawx, RoundTripPreservesEveryField)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + HEADER + (3U * BLOCK)];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));

    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(message::matches(raw));
    EXPECT_EQ(raw.payload_length(), HEADER + (3U * BLOCK));

    message decoded{};
    ASSERT_TRUE(message::decode(raw, decoded));
    EXPECT_EQ(bits_of(decoded.rcv_tow), bits_of(345600.123456789));
    EXPECT_EQ(decoded.rcv_tow, 345600.123456789);
    EXPECT_EQ(decoded.week, original.week);
    EXPECT_EQ(decoded.leap_s, -18);
    EXPECT_EQ(decoded.num_meas, 3U);
    EXPECT_EQ(decoded.rec_stat, original.rec_stat);
    EXPECT_EQ(decoded.version, original.version);
    ASSERT_EQ(decoded.meas.size(), 3U);
    expect_meas_eq(decoded.meas[0U], make_meas(0U));
    expect_meas_eq(decoded.meas[1U], make_meas(1U));
    expect_meas_eq(decoded.meas[2U], make_meas(2U));
    EXPECT_EQ(decoded.meas[0U].do_mes, -1234.5F);
}

TEST(UbxMessageRxmRawx, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[HEADER + (2U * BLOCK)] = {};
    castle::write_le64(&payload[0U], castle::bit_cast<uint64_t>(345600.123456789));
    castle::write_le16(&payload[8U], 2201U);
    payload[10U] = static_cast<uint8_t>(-18);
    payload[11U] = 2U;
    payload[12U] = 0x02U;
    payload[13U] = 0x01U;
    payload[14U] = 0xEEU; // reserved0 is ignored
    payload[15U] = 0xEEU;

    uint8_t* block = &payload[HEADER];
    castle::write_le64(&block[0U], castle::bit_cast<uint64_t>(23456789.0625));
    castle::write_le64(&block[8U], castle::bit_cast<uint64_t>(-123456789.5));
    castle::write_le32(&block[16U], castle::bit_cast<uint32_t>(-1234.5F));
    block[20U] = 6U;
    block[21U] = 17U;
    block[22U] = 2U;
    block[23U] = 13U;
    castle::write_le16(&block[24U], 64500U);
    block[26U] = 47U;
    block[27U] = 0xA3U; // prStd = 3, high nibble garbage
    block[28U] = 0x5FU; // cpStd = 0x0F (invalid), high nibble garbage
    block[29U] = 0xC9U; // doStd = 9, high nibble garbage
    block[30U] = 0x05U;
    block[31U] = 0xEEU; // reserved1 is ignored

    block = &payload[HEADER + BLOCK];
    castle::write_le64(&block[0U], castle::bit_cast<uint64_t>(-1.0e-300));
    castle::write_le64(&block[8U], castle::bit_cast<uint64_t>(9.875e10));
    castle::write_le32(&block[16U], castle::bit_cast<uint32_t>(4321.25F));
    block[20U] = 0U;
    block[21U] = 1U;
    block[22U] = 0U;
    block[23U] = 0U;
    castle::write_le16(&block[24U], 1U);
    block[26U] = 255U;
    block[27U] = 0x0FU;
    block[28U] = 0x00U;
    block[29U] = 0x00U;
    block[30U] = 0x0AU;
    block[31U] = 0U;

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.rcv_tow, 345600.123456789);
    EXPECT_EQ(decoded.week, 2201U);
    EXPECT_EQ(decoded.leap_s, -18);
    EXPECT_EQ(decoded.num_meas, 2U);
    EXPECT_EQ(decoded.rec_stat, 0x02U);
    EXPECT_EQ(decoded.version, 0x01U);
    EXPECT_FALSE(decoded.leap_sec());
    EXPECT_TRUE(decoded.clk_reset());
    ASSERT_EQ(decoded.meas.size(), 2U);

    const rxm_rawx_meas& first = decoded.meas[0U];
    EXPECT_EQ(first.pr_mes, 23456789.0625);
    EXPECT_EQ(first.cp_mes, -123456789.5);
    EXPECT_EQ(first.do_mes, -1234.5F);
    EXPECT_EQ(first.gnss_id, 6U);
    EXPECT_EQ(first.sv_id, 17U);
    EXPECT_EQ(first.sig_id, 2U);
    EXPECT_EQ(first.freq_id, 13U);
    EXPECT_EQ(first.locktime, 64500U);
    EXPECT_EQ(first.cno, 47U);
    EXPECT_EQ(first.pr_stdev, 0xA3U);
    EXPECT_EQ(first.cp_stdev, 0x5FU);
    EXPECT_EQ(first.do_stdev, 0xC9U);
    EXPECT_EQ(first.pr_std(), 3U);
    EXPECT_EQ(first.cp_std(), 0x0FU);
    EXPECT_FALSE(first.cp_std_valid());
    EXPECT_EQ(first.do_std(), 9U);
    EXPECT_EQ(first.trk_stat, 0x05U);
    EXPECT_TRUE(first.pr_valid());
    EXPECT_FALSE(first.cp_valid());
    EXPECT_TRUE(first.half_cyc());
    EXPECT_FALSE(first.sub_half_cyc());

    const rxm_rawx_meas& second = decoded.meas[1U];
    EXPECT_EQ(second.pr_mes, -1.0e-300);
    EXPECT_EQ(second.cp_mes, 9.875e10);
    EXPECT_EQ(second.do_mes, 4321.25F);
    EXPECT_EQ(second.gnss_id, 0U);
    EXPECT_EQ(second.sv_id, 1U);
    EXPECT_EQ(second.sig_id, 0U);
    EXPECT_EQ(second.freq_id, 0U);
    EXPECT_EQ(second.locktime, 1U);
    EXPECT_EQ(second.cno, 255U);
    EXPECT_EQ(second.pr_std(), 15U);
    EXPECT_EQ(second.cp_std(), 0U);
    EXPECT_TRUE(second.cp_std_valid());
    EXPECT_EQ(second.do_std(), 0U);
    EXPECT_EQ(second.trk_stat, 0x0AU);
    EXPECT_FALSE(second.pr_valid());
    EXPECT_TRUE(second.cp_valid());
    EXPECT_FALSE(second.half_cyc());
    EXPECT_TRUE(second.sub_half_cyc());
}

TEST(UbxMessageRxmRawx, EncodesFieldsAtSpecOffsets)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + HEADER + (3U * BLOCK)];
    for (uint8_t& byte : frame)
    {
        byte = 0xAAU;
    }
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);

    const uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], UBX_CLASS_RXM);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_RXM_RAWX);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), HEADER + (3U * BLOCK));
    EXPECT_EQ(castle::read_le64(&payload[0U]), bits_of(345600.123456789));
    EXPECT_EQ(castle::read_le16(&payload[8U]), 2345U);
    EXPECT_EQ(static_cast<int8_t>(payload[10U]), -18);
    EXPECT_EQ(payload[11U], 3U); // numMeas from meas.size(), not num_meas
    EXPECT_EQ(payload[12U], 0x03U);
    EXPECT_EQ(payload[13U], 0x01U);
    EXPECT_EQ(payload[14U], 0U);
    EXPECT_EQ(payload[15U], 0U);

    for (uint8_t index = 0U; index < 3U; ++index)
    {
        const rxm_rawx_meas expected = make_meas(index);
        const uint8_t* block = &payload[HEADER + (index * BLOCK)];
        EXPECT_EQ(castle::read_le64(&block[0U]), bits_of(expected.pr_mes));
        EXPECT_EQ(castle::read_le64(&block[8U]), bits_of(expected.cp_mes));
        EXPECT_EQ(castle::read_le32(&block[16U]), bits_of(expected.do_mes));
        EXPECT_EQ(block[20U], index + 1U);
        EXPECT_EQ(block[21U], index + 2U);
        EXPECT_EQ(block[22U], index + 3U);
        EXPECT_EQ(block[23U], index + 4U);
        EXPECT_EQ(castle::read_le16(&block[24U]), 64500U - index);
        EXPECT_EQ(block[26U], index + 5U);
        EXPECT_EQ(block[27U], 0x06U + index);
        EXPECT_EQ(block[28U], 0x07U + index);
        EXPECT_EQ(block[29U], 0x08U + index);
        EXPECT_EQ(block[30U], 0x0FU - index);
        EXPECT_EQ(block[31U], 0U);
    }
    EXPECT_EQ(castle::read_le64(&payload[HEADER]), castle::bit_cast<uint64_t>(21000000.123456789));
    EXPECT_EQ(castle::read_le32(&payload[HEADER + 16U]), castle::bit_cast<uint32_t>(-1234.5F));
}

TEST(UbxMessageRxmRawx, AcceptsHeaderOnlyPayload)
{
    uint8_t payload[HEADER] = {};
    castle::write_le64(&payload[0U], castle::bit_cast<uint64_t>(1.5));
    castle::write_le16(&payload[8U], 7U);
    payload[10U] = static_cast<uint8_t>(-1);
    payload[12U] = 0x01U;
    payload[13U] = 0x01U;

    message decoded = distinctive();
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.rcv_tow, 1.5);
    EXPECT_EQ(decoded.week, 7U);
    EXPECT_EQ(decoded.leap_s, -1);
    EXPECT_EQ(decoded.num_meas, 0U);
    EXPECT_EQ(decoded.rec_stat, 0x01U);
    EXPECT_EQ(decoded.version, 0x01U);
    EXPECT_TRUE(decoded.meas.empty());

    message empty{};
    uint8_t frame[UBX_FRAME_OVERHEAD + HEADER];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(empty, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), HEADER);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 11U], 0U);
}

TEST(UbxMessageRxmRawx, RejectsWrongClassOrId)
{
    uint8_t payload[HEADER] = {};
    castle::write_le16(&payload[8U], 7U);
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_RXM_MEASX;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_id = message::msg_id;
    EXPECT_TRUE(message::matches(raw));
}

TEST(UbxMessageRxmRawx, RejectsShortAndLongPayloads)
{
    uint8_t payload[HEADER + BLOCK + 1U] = {};
    castle::write_le16(&payload[8U], 7U);

    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, HEADER - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, 0U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, HEADER + 1U), decoded)); // count 0, extra byte
    expect_default(decoded);
}

TEST(UbxMessageRxmRawx, RejectsGroupLengthMismatchAndResetsOutput)
{
    uint8_t payload[HEADER + BLOCK + 1U] = {};
    castle::write_le16(&payload[8U], 7U);
    payload[11U] = 1U;

    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, HEADER + BLOCK - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, HEADER + BLOCK + 1U), decoded));
    expect_default(decoded);

    payload[11U] = 2U; // count says two blocks but one is present
    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, HEADER + BLOCK), decoded));
    expect_default(decoded);

    payload[11U] = 1U;
    decoded = distinctive();
    EXPECT_TRUE(message::decode(view_of(payload, HEADER + BLOCK), decoded));
    EXPECT_EQ(decoded.week, 7U);
    EXPECT_EQ(decoded.meas.size(), 1U);
}

TEST(UbxMessageRxmRawx, ClampsStorageButKeepsWireCount)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + HEADER + (3U * BLOCK)];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));

    rxm_rawx<1U> clamped{};
    ASSERT_TRUE(rxm_rawx<1U>::decode(raw, clamped));
    EXPECT_EQ(clamped.num_meas, 3U);
    ASSERT_EQ(clamped.meas.size(), clamped.meas.capacity());
    ASSERT_EQ(clamped.meas.size(), 1U);
    expect_meas_eq(clamped.meas[0U], make_meas(0U));
    EXPECT_EQ(clamped.rcv_tow, original.rcv_tow);
    EXPECT_EQ(clamped.week, original.week);
    EXPECT_EQ(clamped.leap_s, original.leap_s);
    EXPECT_EQ(clamped.rec_stat, original.rec_stat);
    EXPECT_EQ(clamped.version, original.version);

    rxm_rawx<2U> partial{};
    ASSERT_TRUE(rxm_rawx<2U>::decode(raw, partial));
    EXPECT_EQ(partial.num_meas, 3U);
    ASSERT_EQ(partial.meas.size(), 2U);
    expect_meas_eq(partial.meas[0U], make_meas(0U));
    expect_meas_eq(partial.meas[1U], make_meas(1U));
}

TEST(UbxMessageRxmRawx, RecStatAccessors)
{
    message value{};
    EXPECT_FALSE(value.leap_sec());
    EXPECT_FALSE(value.clk_reset());

    value.rec_stat = message::REC_STAT_LEAP_SEC;
    EXPECT_TRUE(value.leap_sec());
    EXPECT_FALSE(value.clk_reset());

    value.rec_stat = message::REC_STAT_CLK_RESET;
    EXPECT_FALSE(value.leap_sec());
    EXPECT_TRUE(value.clk_reset());

    value.rec_stat = 0xFCU; // undefined bits only
    EXPECT_FALSE(value.leap_sec());
    EXPECT_FALSE(value.clk_reset());

    value.rec_stat = 0xFFU;
    EXPECT_TRUE(value.leap_sec());
    EXPECT_TRUE(value.clk_reset());
}

TEST(UbxMessageRxmRawx, TrkStatAccessors)
{
    rxm_rawx_meas meas{};
    EXPECT_FALSE(meas.pr_valid());
    EXPECT_FALSE(meas.cp_valid());
    EXPECT_FALSE(meas.half_cyc());
    EXPECT_FALSE(meas.sub_half_cyc());

    meas.trk_stat = rxm_rawx_meas::TRK_STAT_PR_VALID;
    EXPECT_TRUE(meas.pr_valid());
    EXPECT_FALSE(meas.cp_valid());
    EXPECT_FALSE(meas.half_cyc());
    EXPECT_FALSE(meas.sub_half_cyc());

    meas.trk_stat = rxm_rawx_meas::TRK_STAT_CP_VALID;
    EXPECT_FALSE(meas.pr_valid());
    EXPECT_TRUE(meas.cp_valid());
    EXPECT_FALSE(meas.half_cyc());
    EXPECT_FALSE(meas.sub_half_cyc());

    meas.trk_stat = rxm_rawx_meas::TRK_STAT_HALF_CYC;
    EXPECT_FALSE(meas.pr_valid());
    EXPECT_FALSE(meas.cp_valid());
    EXPECT_TRUE(meas.half_cyc());
    EXPECT_FALSE(meas.sub_half_cyc());

    meas.trk_stat = rxm_rawx_meas::TRK_STAT_SUB_HALF_CYC;
    EXPECT_FALSE(meas.pr_valid());
    EXPECT_FALSE(meas.cp_valid());
    EXPECT_FALSE(meas.half_cyc());
    EXPECT_TRUE(meas.sub_half_cyc());

    meas.trk_stat = 0xF0U; // undefined bits only
    EXPECT_FALSE(meas.pr_valid());
    EXPECT_FALSE(meas.cp_valid());
    EXPECT_FALSE(meas.half_cyc());
    EXPECT_FALSE(meas.sub_half_cyc());

    meas.trk_stat = 0x0FU;
    EXPECT_TRUE(meas.pr_valid());
    EXPECT_TRUE(meas.cp_valid());
    EXPECT_TRUE(meas.half_cyc());
    EXPECT_TRUE(meas.sub_half_cyc());
}

TEST(UbxMessageRxmRawx, StdevIndexMasksLowNibbleOnly)
{
    rxm_rawx_meas meas{};
    meas.pr_stdev = 0xF0U;
    meas.cp_stdev = 0xA0U;
    meas.do_stdev = 0x50U;
    EXPECT_EQ(meas.pr_std(), 0U);
    EXPECT_EQ(meas.cp_std(), 0U);
    EXPECT_EQ(meas.do_std(), 0U);
    EXPECT_DOUBLE_EQ(meas.pr_std_m(), 0.01);
    EXPECT_DOUBLE_EQ(meas.cp_std_cycles(), 0.0);
    EXPECT_TRUE(meas.cp_std_valid());
    EXPECT_DOUBLE_EQ(meas.do_std_hz(), 0.002);

    meas.pr_stdev = 0x7FU;
    meas.cp_stdev = 0x3EU;
    meas.do_stdev = 0xBFU;
    EXPECT_EQ(meas.pr_std(), 15U);
    EXPECT_EQ(meas.cp_std(), 14U);
    EXPECT_EQ(meas.do_std(), 15U);
    EXPECT_TRUE(meas.cp_std_valid());
    EXPECT_DOUBLE_EQ(meas.cp_std_cycles(), 0.056);

    meas.cp_stdev = 0x9FU;
    EXPECT_EQ(meas.cp_std(), 0x0FU);
    EXPECT_FALSE(meas.cp_std_valid());
    meas.cp_stdev = 0x0FU;
    EXPECT_FALSE(meas.cp_std_valid());
    meas.cp_stdev = 0xF0U;
    EXPECT_TRUE(meas.cp_std_valid());
}

TEST(UbxMessageRxmRawx, ScaledHelpers)
{
    rxm_rawx_meas meas{};
    EXPECT_DOUBLE_EQ(meas.pr_std_m(), 0.01);
    EXPECT_DOUBLE_EQ(meas.cp_std_cycles(), 0.0);
    EXPECT_DOUBLE_EQ(meas.do_std_hz(), 0.002);

    meas.pr_stdev = 15U;
    meas.cp_stdev = 15U;
    meas.do_stdev = 15U;
    EXPECT_DOUBLE_EQ(meas.pr_std_m(), 327.68);
    EXPECT_DOUBLE_EQ(meas.cp_std_cycles(), 0.06);
    EXPECT_DOUBLE_EQ(meas.do_std_hz(), 65.536);

    meas.pr_stdev = 5U;
    meas.cp_stdev = 1U;
    meas.do_stdev = 3U;
    EXPECT_DOUBLE_EQ(meas.pr_std_m(), 0.32);
    EXPECT_DOUBLE_EQ(meas.cp_std_cycles(), 0.004);
    EXPECT_DOUBLE_EQ(meas.do_std_hz(), 0.016);

    meas.pr_stdev = 0xE5U; // high nibble ignored by the helpers too
    meas.cp_stdev = 0xD1U;
    meas.do_stdev = 0xC3U;
    EXPECT_DOUBLE_EQ(meas.pr_std_m(), 0.32);
    EXPECT_DOUBLE_EQ(meas.cp_std_cycles(), 0.004);
    EXPECT_DOUBLE_EQ(meas.do_std_hz(), 0.016);
}

TEST(UbxMessageRxmRawx, EncodeIntoTooSmallBufferReportsFull)
{
    const message value = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + HEADER + (3U * BLOCK)];
    castle::size_type written = 99U;
    EXPECT_EQ(encode_message(value, frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    written = 99U;
    EXPECT_EQ(encode_message(value, frame, UBX_FRAME_OVERHEAD + HEADER - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);

    written = 99U;
    EXPECT_EQ(encode_message(value, frame, UBX_FRAME_OVERHEAD - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace

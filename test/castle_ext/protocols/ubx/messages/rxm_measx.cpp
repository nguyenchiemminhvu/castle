#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/rxm_measx.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = rxm_measx<4U>;

constexpr castle::size_type HEADER = 44U;
constexpr castle::size_type BLOCK = 24U;

message_view view_of(const uint8_t* payload, castle::size_type length)
{
    return message_view{{message::msg_class, message::msg_id, static_cast<uint16_t>(length)},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

rxm_measx_sv make_sv(uint8_t seed)
{
    rxm_measx_sv sv{};
    sv.gnss_id = static_cast<uint8_t>(seed + 1U);
    sv.sv_id = static_cast<uint8_t>(seed + 2U);
    sv.cno = static_cast<uint8_t>(seed + 40U);
    sv.mpath_indic = static_cast<uint8_t>((seed % 3U) + 1U);
    sv.doppler_ms = -123456 - static_cast<int32_t>(seed);
    sv.doppler_hz = 654321 + static_cast<int32_t>(seed);
    sv.whole_chips = static_cast<uint16_t>(1022U - seed);
    sv.frac_chips = static_cast<uint16_t>(1000U + seed);
    sv.code_phase = 0x89ABCDEFU - seed;
    sv.int_code_phase = static_cast<uint8_t>(seed + 7U);
    sv.pseu_range_rms_err = static_cast<uint8_t>(63U - seed);
    return sv;
}

message distinctive()
{
    message value{};
    value.version = 0x01U;
    value.gps_tow = 0x11223344U;
    value.glo_tow = 0x55667788U;
    value.bds_tow = 0x99AABBCCU;
    value.qzss_tow = 0xDDEEFF01U;
    value.gps_tow_acc = 0x1234U;
    value.glo_tow_acc = 0x5678U;
    value.bds_tow_acc = 0x9ABCU;
    value.qzss_tow_acc = 0xDEF0U;
    value.num_svs = 200U; // ignored on encode: count comes from svs.size()
    value.flags = 0x02U;
    static_cast<void>(value.svs.push_back(make_sv(0U)));
    static_cast<void>(value.svs.push_back(make_sv(1U)));
    static_cast<void>(value.svs.push_back(make_sv(2U)));
    return value;
}

void expect_sv_eq(const rxm_measx_sv& actual, const rxm_measx_sv& expected)
{
    EXPECT_EQ(actual.gnss_id, expected.gnss_id);
    EXPECT_EQ(actual.sv_id, expected.sv_id);
    EXPECT_EQ(actual.cno, expected.cno);
    EXPECT_EQ(actual.mpath_indic, expected.mpath_indic);
    EXPECT_EQ(actual.doppler_ms, expected.doppler_ms);
    EXPECT_EQ(actual.doppler_hz, expected.doppler_hz);
    EXPECT_EQ(actual.whole_chips, expected.whole_chips);
    EXPECT_EQ(actual.frac_chips, expected.frac_chips);
    EXPECT_EQ(actual.code_phase, expected.code_phase);
    EXPECT_EQ(actual.int_code_phase, expected.int_code_phase);
    EXPECT_EQ(actual.pseu_range_rms_err, expected.pseu_range_rms_err);
}

template <castle::size_type N>
void expect_default(const rxm_measx<N>& value)
{
    EXPECT_EQ(value.version, 0U);
    EXPECT_EQ(value.gps_tow, 0U);
    EXPECT_EQ(value.glo_tow, 0U);
    EXPECT_EQ(value.bds_tow, 0U);
    EXPECT_EQ(value.qzss_tow, 0U);
    EXPECT_EQ(value.gps_tow_acc, 0U);
    EXPECT_EQ(value.glo_tow_acc, 0U);
    EXPECT_EQ(value.bds_tow_acc, 0U);
    EXPECT_EQ(value.qzss_tow_acc, 0U);
    EXPECT_EQ(value.num_svs, 0U);
    EXPECT_EQ(value.flags, 0U);
    EXPECT_TRUE(value.svs.empty());
}

TEST(UbxMessageRxmMeasx, ConstantsMatchSpec)
{
    EXPECT_EQ(message::msg_class, UBX_CLASS_RXM);
    EXPECT_EQ(message::msg_id, UBX_ID_RXM_MEASX);
    EXPECT_EQ(message::msg_class, 0x02U);
    EXPECT_EQ(message::msg_id, 0x14U);
    EXPECT_EQ(message::header_length, HEADER);
    EXPECT_EQ(message::block_length, BLOCK);
    EXPECT_EQ(rxm_measx_sv::wire_size, BLOCK);
    EXPECT_EQ(message::FLAGS_TOW_SET, 0x03U);
    EXPECT_EQ(message::TOW_ACC_OVER_4S, 0xFFFFU);
    EXPECT_EQ(static_cast<uint8_t>(rxm_measx_mpath::not_measured), 0U);
    EXPECT_EQ(static_cast<uint8_t>(rxm_measx_mpath::low), 1U);
    EXPECT_EQ(static_cast<uint8_t>(rxm_measx_mpath::medium), 2U);
    EXPECT_EQ(static_cast<uint8_t>(rxm_measx_mpath::high), 3U);
    EXPECT_EQ(message{}.svs.capacity(), 4U);
    EXPECT_EQ(rxm_measx<>{}.svs.capacity(), 64U);
}

TEST(UbxMessageRxmMeasx, RoundTripPreservesEveryField)
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
    EXPECT_EQ(decoded.version, original.version);
    EXPECT_EQ(decoded.gps_tow, original.gps_tow);
    EXPECT_EQ(decoded.glo_tow, original.glo_tow);
    EXPECT_EQ(decoded.bds_tow, original.bds_tow);
    EXPECT_EQ(decoded.qzss_tow, original.qzss_tow);
    EXPECT_EQ(decoded.gps_tow_acc, original.gps_tow_acc);
    EXPECT_EQ(decoded.glo_tow_acc, original.glo_tow_acc);
    EXPECT_EQ(decoded.bds_tow_acc, original.bds_tow_acc);
    EXPECT_EQ(decoded.qzss_tow_acc, original.qzss_tow_acc);
    EXPECT_EQ(decoded.num_svs, 3U);
    EXPECT_EQ(decoded.flags, original.flags);
    ASSERT_EQ(decoded.svs.size(), 3U);
    expect_sv_eq(decoded.svs[0U], make_sv(0U));
    expect_sv_eq(decoded.svs[1U], make_sv(1U));
    expect_sv_eq(decoded.svs[2U], make_sv(2U));
}

TEST(UbxMessageRxmMeasx, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[HEADER + (2U * BLOCK)] = {};
    payload[0U] = 0x01U;
    payload[1U] = 0xEEU; // reserved0 is ignored
    payload[2U] = 0xEEU;
    payload[3U] = 0xEEU;
    castle::write_le32(&payload[4U], 345600000U);
    castle::write_le32(&payload[8U], 10800123U);
    castle::write_le32(&payload[12U], 345586000U);
    for (castle::size_type index = 16U; index < 20U; ++index)
    {
        payload[index] = 0xEEU; // reserved1 is ignored
    }
    castle::write_le32(&payload[20U], 345600001U);
    castle::write_le16(&payload[24U], 16U);
    castle::write_le16(&payload[26U], 32U);
    castle::write_le16(&payload[28U], 48U);
    payload[30U] = 0xEEU; // reserved2 is ignored
    payload[31U] = 0xEEU;
    castle::write_le16(&payload[32U], 0xFFFFU);
    payload[34U] = 2U;
    payload[35U] = 0xFDU; // towSet = 1, upper bits garbage
    for (castle::size_type index = 36U; index < 44U; ++index)
    {
        payload[index] = 0xEEU; // reserved3 is ignored
    }

    uint8_t* block = &payload[HEADER];
    block[0U] = 0U;
    block[1U] = 12U;
    block[2U] = 45U;
    block[3U] = 3U;
    castle::write_le32s(&block[4U], -2500);
    castle::write_le32s(&block[8U], 13125);
    castle::write_le16(&block[12U], 1022U);
    castle::write_le16(&block[14U], 1023U);
    castle::write_le32(&block[16U], 2097152U);
    block[20U] = 19U;
    block[21U] = 63U;
    block[22U] = 0xEEU; // reserved4 is ignored
    block[23U] = 0xEEU;

    block = &payload[HEADER + BLOCK];
    block[0U] = 6U;
    block[1U] = 24U;
    block[2U] = 1U;
    block[3U] = 0U;
    castle::write_le32s(&block[4U], 1000);
    castle::write_le32s(&block[8U], -5);
    castle::write_le16(&block[12U], 1U);
    castle::write_le16(&block[14U], 2U);
    castle::write_le32(&block[16U], 1U);
    block[20U] = 0U;
    block[21U] = 1U;

    message decoded{};
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.version, 0x01U);
    EXPECT_EQ(decoded.gps_tow, 345600000U);
    EXPECT_EQ(decoded.glo_tow, 10800123U);
    EXPECT_EQ(decoded.bds_tow, 345586000U);
    EXPECT_EQ(decoded.qzss_tow, 345600001U);
    EXPECT_EQ(decoded.gps_tow_acc, 16U);
    EXPECT_EQ(decoded.glo_tow_acc, 32U);
    EXPECT_EQ(decoded.bds_tow_acc, 48U);
    EXPECT_EQ(decoded.qzss_tow_acc, message::TOW_ACC_OVER_4S);
    EXPECT_EQ(decoded.num_svs, 2U);
    EXPECT_EQ(decoded.flags, 0xFDU);
    EXPECT_EQ(decoded.tow_set(), 1U);
    ASSERT_EQ(decoded.svs.size(), 2U);

    const rxm_measx_sv& first = decoded.svs[0U];
    EXPECT_EQ(first.gnss_id, 0U);
    EXPECT_EQ(first.sv_id, 12U);
    EXPECT_EQ(first.cno, 45U);
    EXPECT_EQ(first.mpath_indic, 3U);
    EXPECT_EQ(first.mpath(), rxm_measx_mpath::high);
    EXPECT_EQ(first.doppler_ms, -2500);
    EXPECT_EQ(first.doppler_hz, 13125);
    EXPECT_EQ(first.whole_chips, 1022U);
    EXPECT_EQ(first.frac_chips, 1023U);
    EXPECT_EQ(first.code_phase, 2097152U);
    EXPECT_EQ(first.int_code_phase, 19U);
    EXPECT_EQ(first.pseu_range_rms_err, 63U);
    EXPECT_DOUBLE_EQ(first.doppler_speed_mps(), -100.0);
    EXPECT_DOUBLE_EQ(first.doppler_frequency_hz(), 2625.0);
    EXPECT_DOUBLE_EQ(first.code_phase_ms(), 1.0);

    const rxm_measx_sv& second = decoded.svs[1U];
    EXPECT_EQ(second.gnss_id, 6U);
    EXPECT_EQ(second.sv_id, 24U);
    EXPECT_EQ(second.cno, 1U);
    EXPECT_EQ(second.mpath_indic, 0U);
    EXPECT_EQ(second.mpath(), rxm_measx_mpath::not_measured);
    EXPECT_EQ(second.doppler_ms, 1000);
    EXPECT_EQ(second.doppler_hz, -5);
    EXPECT_EQ(second.whole_chips, 1U);
    EXPECT_EQ(second.frac_chips, 2U);
    EXPECT_EQ(second.code_phase, 1U);
    EXPECT_EQ(second.int_code_phase, 0U);
    EXPECT_EQ(second.pseu_range_rms_err, 1U);
}

TEST(UbxMessageRxmMeasx, EncodesFieldsAtSpecOffsets)
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
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], UBX_ID_RXM_MEASX);
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), HEADER + (3U * BLOCK));
    EXPECT_EQ(payload[0U], 0x01U);
    EXPECT_EQ(payload[1U], 0U);
    EXPECT_EQ(payload[2U], 0U);
    EXPECT_EQ(payload[3U], 0U);
    EXPECT_EQ(castle::read_le32(&payload[4U]), 0x11223344U);
    EXPECT_EQ(castle::read_le32(&payload[8U]), 0x55667788U);
    EXPECT_EQ(castle::read_le32(&payload[12U]), 0x99AABBCCU);
    EXPECT_EQ(payload[16U], 0U);
    EXPECT_EQ(payload[17U], 0U);
    EXPECT_EQ(payload[18U], 0U);
    EXPECT_EQ(payload[19U], 0U);
    EXPECT_EQ(castle::read_le32(&payload[20U]), 0xDDEEFF01U);
    EXPECT_EQ(castle::read_le16(&payload[24U]), 0x1234U);
    EXPECT_EQ(castle::read_le16(&payload[26U]), 0x5678U);
    EXPECT_EQ(castle::read_le16(&payload[28U]), 0x9ABCU);
    EXPECT_EQ(payload[30U], 0U);
    EXPECT_EQ(payload[31U], 0U);
    EXPECT_EQ(castle::read_le16(&payload[32U]), 0xDEF0U);
    EXPECT_EQ(payload[34U], 3U); // numSV from svs.size(), not num_svs
    EXPECT_EQ(payload[35U], 0x02U);
    for (castle::size_type index = 36U; index < 44U; ++index)
    {
        EXPECT_EQ(payload[index], 0U) << "reserved3 byte " << index;
    }

    for (uint8_t index = 0U; index < 3U; ++index)
    {
        const rxm_measx_sv expected = make_sv(index);
        const uint8_t* block = &payload[HEADER + (index * BLOCK)];
        EXPECT_EQ(block[0U], index + 1U);
        EXPECT_EQ(block[1U], index + 2U);
        EXPECT_EQ(block[2U], index + 40U);
        EXPECT_EQ(block[3U], expected.mpath_indic);
        EXPECT_EQ(castle::read_le32s(&block[4U]), -123456 - index);
        EXPECT_EQ(castle::read_le32s(&block[8U]), 654321 + index);
        EXPECT_EQ(castle::read_le16(&block[12U]), 1022U - index);
        EXPECT_EQ(castle::read_le16(&block[14U]), 1000U + index);
        EXPECT_EQ(castle::read_le32(&block[16U]), 0x89ABCDEFU - index);
        EXPECT_EQ(block[20U], index + 7U);
        EXPECT_EQ(block[21U], 63U - index);
        EXPECT_EQ(block[22U], 0U);
        EXPECT_EQ(block[23U], 0U);
    }
}

TEST(UbxMessageRxmMeasx, AcceptsHeaderOnlyPayload)
{
    uint8_t payload[HEADER] = {};
    payload[0U] = 0x01U;
    castle::write_le32(&payload[4U], 42U);
    castle::write_le16(&payload[32U], 9U);
    payload[35U] = 0x01U;

    message decoded = distinctive();
    ASSERT_TRUE(message::decode(view_of(payload, sizeof(payload)), decoded));
    EXPECT_EQ(decoded.version, 0x01U);
    EXPECT_EQ(decoded.gps_tow, 42U);
    EXPECT_EQ(decoded.glo_tow, 0U);
    EXPECT_EQ(decoded.qzss_tow_acc, 9U);
    EXPECT_EQ(decoded.num_svs, 0U);
    EXPECT_EQ(decoded.flags, 0x01U);
    EXPECT_TRUE(decoded.svs.empty());

    const message empty{};
    uint8_t frame[UBX_FRAME_OVERHEAD + HEADER];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(empty, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, sizeof(frame));
    EXPECT_EQ(castle::read_le16(&frame[UBX_FRAME_INDEX_LENGTH]), HEADER);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 34U], 0U);
}

TEST(UbxMessageRxmMeasx, RejectsWrongClassOrId)
{
    uint8_t payload[HEADER] = {};
    castle::write_le32(&payload[4U], 7U);
    message_view raw = view_of(payload, sizeof(payload));

    message decoded = distinctive();
    raw.header.msg_class = UBX_CLASS_NAV;
    EXPECT_FALSE(message::matches(raw));
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_class = message::msg_class;
    raw.header.msg_id = UBX_ID_RXM_RAWX;
    EXPECT_FALSE(message::matches(raw));
    decoded = distinctive();
    EXPECT_FALSE(message::decode(raw, decoded));
    expect_default(decoded);

    raw.header.msg_id = message::msg_id;
    EXPECT_TRUE(message::matches(raw));
}

TEST(UbxMessageRxmMeasx, RejectsShortAndLongPayloads)
{
    uint8_t payload[HEADER + BLOCK + 1U] = {};
    castle::write_le32(&payload[4U], 7U);

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

TEST(UbxMessageRxmMeasx, RejectsGroupLengthMismatchAndResetsOutput)
{
    uint8_t payload[HEADER + BLOCK + 1U] = {};
    castle::write_le32(&payload[4U], 7U);
    payload[34U] = 1U;

    message decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, HEADER + BLOCK - 1U), decoded));
    expect_default(decoded);

    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, HEADER + BLOCK + 1U), decoded));
    expect_default(decoded);

    payload[34U] = 2U; // count says two blocks but one is present
    decoded = distinctive();
    EXPECT_FALSE(message::decode(view_of(payload, HEADER + BLOCK), decoded));
    expect_default(decoded);

    payload[34U] = 1U;
    decoded = distinctive();
    EXPECT_TRUE(message::decode(view_of(payload, HEADER + BLOCK), decoded));
    EXPECT_EQ(decoded.gps_tow, 7U);
    EXPECT_EQ(decoded.svs.size(), 1U);
}

TEST(UbxMessageRxmMeasx, ClampsStorageButKeepsWireCount)
{
    const message original = distinctive();
    uint8_t frame[UBX_FRAME_OVERHEAD + HEADER + (3U * BLOCK)];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(original, frame, sizeof(frame), written), castle::status::ok);
    message_view raw{};
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));

    rxm_measx<1U> clamped{};
    ASSERT_TRUE(rxm_measx<1U>::decode(raw, clamped));
    EXPECT_EQ(clamped.num_svs, 3U);
    ASSERT_EQ(clamped.svs.size(), clamped.svs.capacity());
    ASSERT_EQ(clamped.svs.size(), 1U);
    expect_sv_eq(clamped.svs[0U], make_sv(0U));
    EXPECT_EQ(clamped.version, original.version);
    EXPECT_EQ(clamped.gps_tow, original.gps_tow);
    EXPECT_EQ(clamped.qzss_tow_acc, original.qzss_tow_acc);
    EXPECT_EQ(clamped.flags, original.flags);

    rxm_measx<2U> partial{};
    ASSERT_TRUE(rxm_measx<2U>::decode(raw, partial));
    EXPECT_EQ(partial.num_svs, 3U);
    ASSERT_EQ(partial.svs.size(), 2U);
    expect_sv_eq(partial.svs[0U], make_sv(0U));
    expect_sv_eq(partial.svs[1U], make_sv(1U));
}

TEST(UbxMessageRxmMeasx, TowSetMasksLowTwoBitsOnly)
{
    message value{};
    EXPECT_EQ(value.tow_set(), 0U);
    value.flags = 0x01U;
    EXPECT_EQ(value.tow_set(), 1U);
    value.flags = 0x02U;
    EXPECT_EQ(value.tow_set(), 2U);
    value.flags = 0x03U;
    EXPECT_EQ(value.tow_set(), 3U);
    value.flags = 0xFCU;
    EXPECT_EQ(value.tow_set(), 0U);
    value.flags = 0xFEU;
    EXPECT_EQ(value.tow_set(), 2U);
}

TEST(UbxMessageRxmMeasx, MpathCoversEveryEnumerator)
{
    rxm_measx_sv sv{};
    EXPECT_EQ(sv.mpath(), rxm_measx_mpath::not_measured);
    sv.mpath_indic = 1U;
    EXPECT_EQ(sv.mpath(), rxm_measx_mpath::low);
    sv.mpath_indic = 2U;
    EXPECT_EQ(sv.mpath(), rxm_measx_mpath::medium);
    sv.mpath_indic = 3U;
    EXPECT_EQ(sv.mpath(), rxm_measx_mpath::high);
    sv.mpath_indic = 0U;
    EXPECT_EQ(sv.mpath(), rxm_measx_mpath::not_measured);
}

TEST(UbxMessageRxmMeasx, ScaledHelpers)
{
    EXPECT_DOUBLE_EQ(message::tow_acc_ms(0U), 0.0);
    EXPECT_DOUBLE_EQ(message::tow_acc_ms(1U), 0.0625);
    EXPECT_DOUBLE_EQ(message::tow_acc_ms(16U), 1.0);
    EXPECT_DOUBLE_EQ(message::tow_acc_ms(40U), 2.5);
    EXPECT_DOUBLE_EQ(message::tow_acc_ms(message::TOW_ACC_OVER_4S), 4095.9375);

    rxm_measx_sv sv{};
    EXPECT_EQ(sv.doppler_speed_mps(), 0.0);
    EXPECT_EQ(sv.doppler_frequency_hz(), 0.0);
    EXPECT_EQ(sv.code_phase_ms(), 0.0);

    sv.doppler_ms = 25;
    sv.doppler_hz = 5;
    sv.code_phase = 1U;
    EXPECT_DOUBLE_EQ(sv.doppler_speed_mps(), 1.0);
    EXPECT_DOUBLE_EQ(sv.doppler_frequency_hz(), 1.0);
    EXPECT_DOUBLE_EQ(sv.code_phase_ms(), 1.0 / 2097152.0);

    sv.doppler_ms = -12345;
    sv.doppler_hz = -65432;
    sv.code_phase = 3145728U; // 1.5 * 2^21
    EXPECT_NEAR(sv.doppler_speed_mps(), -493.8, 1e-9);
    EXPECT_NEAR(sv.doppler_frequency_hz(), -13086.4, 1e-9);
    EXPECT_DOUBLE_EQ(sv.code_phase_ms(), 1.5);

    sv.code_phase = 0xFFFFFFFFU;
    EXPECT_DOUBLE_EQ(sv.code_phase_ms(), 4294967295.0 / 2097152.0);
}

TEST(UbxMessageRxmMeasx, EncodeIntoTooSmallBufferReportsFull)
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

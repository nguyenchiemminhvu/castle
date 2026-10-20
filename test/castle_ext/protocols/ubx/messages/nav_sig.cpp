#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_sig.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

nav_sig_info make_signal(uint8_t seed)
{
    nav_sig_info info{};
    info.gnss_id = static_cast<uint8_t>(seed + 1U);
    info.sv_id = static_cast<uint8_t>(seed + 2U);
    info.sig_id = static_cast<uint8_t>(seed + 3U);
    info.freq_id = static_cast<uint8_t>(seed + 4U);
    info.pr_res = static_cast<int16_t>(-500 - seed);
    info.cno = static_cast<uint8_t>(seed + 30U);
    info.qual_ind = nav_signal_quality::unusable;
    info.corr_source = nav_sig_corr_source::rtcm3_ssr;
    info.iono_model = nav_sig_iono_model::klobuchar_beidou;
    info.sig_flags = static_cast<uint16_t>(0x01FDU - seed);
    return info;
}

void expect_signal_eq(CASTLE_CONST nav_sig_info& actual, CASTLE_CONST nav_sig_info& expected)
{
    EXPECT_EQ(actual.gnss_id, expected.gnss_id);
    EXPECT_EQ(actual.sv_id, expected.sv_id);
    EXPECT_EQ(actual.sig_id, expected.sig_id);
    EXPECT_EQ(actual.freq_id, expected.freq_id);
    EXPECT_EQ(actual.pr_res, expected.pr_res);
    EXPECT_EQ(actual.cno, expected.cno);
    EXPECT_EQ(actual.qual_ind, expected.qual_ind);
    EXPECT_EQ(actual.corr_source, expected.corr_source);
    EXPECT_EQ(actual.iono_model, expected.iono_model);
    EXPECT_EQ(actual.sig_flags, expected.sig_flags);
}

TEST(UbxMessageNavSig, RoundTripsEveryField)
{
    nav_sig<4U> message{};
    message.i_tow = 0x76543210U;
    message.version = 0x5AU;
    message.num_sigs = 200U; // ignored on encode
    ASSERT_TRUE(castle::succeeded(message.signals.push_back(make_signal(0U))));
    ASSERT_TRUE(castle::succeeded(message.signals.push_back(make_signal(9U))));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD + 8U + (2U * 16U));

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(nav_sig<4U>::matches(raw));

    nav_sig<4U> decoded;
    ASSERT_TRUE(nav_sig<4U>::decode(raw, decoded));
    EXPECT_EQ(decoded.i_tow, 0x76543210U);
    EXPECT_EQ(decoded.version, 0x5AU);
    EXPECT_EQ(decoded.num_sigs, 2U);
    ASSERT_EQ(decoded.signals.size(), 2U);
    expect_signal_eq(decoded.signals[0U], make_signal(0U));
    expect_signal_eq(decoded.signals[1U], make_signal(9U));
}

TEST(UbxMessageNavSig, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[24U] = {};
    castle::write_le32(&payload[0U], 55U);
    payload[4U] = 0U;
    payload[5U] = 1U;
    payload[6U] = 0xEEU;
    payload[7U] = 0xEEU;
    payload[8U] = 6U;
    payload[9U] = 22U;
    payload[10U] = 7U;
    payload[11U] = 3U;
    castle::write_le16(&payload[12U], static_cast<uint16_t>(-15));
    payload[14U] = 40U;
    payload[15U] = 5U;
    payload[16U] = 2U;
    payload[17U] = 3U;
    castle::write_le16(&payload[18U], 0x1234U);
    payload[20U] = 0xEEU; // reserved1 is ignored
    payload[23U] = 0xEEU;

    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SIG, payload, 24U);
    nav_sig<2U> out;
    ASSERT_TRUE(nav_sig<2U>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 55U);
    EXPECT_EQ(out.version, 0U);
    EXPECT_EQ(out.num_sigs, 1U);
    ASSERT_EQ(out.signals.size(), 1U);
    EXPECT_EQ(out.signals[0U].gnss_id, 6U);
    EXPECT_EQ(out.signals[0U].sv_id, 22U);
    EXPECT_EQ(out.signals[0U].sig_id, 7U);
    EXPECT_EQ(out.signals[0U].freq_id, 3U);
    EXPECT_EQ(out.signals[0U].pr_res, -15);
    EXPECT_EQ(out.signals[0U].cno, 40U);
    EXPECT_EQ(out.signals[0U].qual_ind, nav_signal_quality::code_carrier_locked);
    EXPECT_EQ(out.signals[0U].corr_source, nav_sig_corr_source::beidou);
    EXPECT_EQ(out.signals[0U].iono_model, nav_sig_iono_model::klobuchar_beidou);
    EXPECT_EQ(out.signals[0U].sig_flags, 0x1234U);
}

TEST(UbxMessageNavSig, EncodesSpecOffsetsAndZeroReserved)
{
    nav_sig<1U> message{};
    message.i_tow = 0xCAFEBABEU;
    ASSERT_TRUE(castle::succeeded(message.signals.push_back(make_signal(0U))));

    uint8_t frame[40U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(castle::read_le32(&payload[0U]), 0xCAFEBABEU);
    EXPECT_EQ(payload[5U], 1U);
    EXPECT_EQ(payload[6U], 0U);
    EXPECT_EQ(payload[7U], 0U);
    EXPECT_EQ(payload[8U], 1U);
    EXPECT_EQ(payload[9U], 2U);
    EXPECT_EQ(payload[10U], 3U);
    EXPECT_EQ(payload[11U], 4U);
    EXPECT_EQ(static_cast<int16_t>(castle::read_le16(&payload[12U])), -500);
    EXPECT_EQ(payload[14U], 30U);
    EXPECT_EQ(payload[15U], 3U);
    EXPECT_EQ(payload[16U], 5U);
    EXPECT_EQ(payload[17U], 3U);
    EXPECT_EQ(castle::read_le16(&payload[18U]), 0x01FDU);
    EXPECT_EQ(payload[20U], 0U);
    EXPECT_EQ(payload[21U], 0U);
    EXPECT_EQ(payload[22U], 0U);
    EXPECT_EQ(payload[23U], 0U);
}

TEST(UbxMessageNavSig, AcceptsEmptyGroup)
{
    uint8_t payload[8U] = {};
    castle::write_le32(&payload[0U], 9U);
    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SIG, payload, 8U);
    nav_sig<> out;
    ASSERT_TRUE(nav_sig<>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 9U);
    EXPECT_EQ(out.num_sigs, 0U);
    EXPECT_TRUE(out.signals.empty());
}

TEST(UbxMessageNavSig, RejectsWrongClassOrId)
{
    uint8_t payload[8U] = {};
    castle::write_le32(&payload[0U], 9U);
    nav_sig<> out;
    out.i_tow = 1U;

    message_view wrong_class = make_view(UBX_CLASS_MON, UBX_ID_NAV_SIG, payload, 8U);
    EXPECT_FALSE(nav_sig<>::matches(wrong_class));
    EXPECT_FALSE(nav_sig<>::decode(wrong_class, out));
    EXPECT_EQ(out.i_tow, 0U);

    message_view wrong_id = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SAT, payload, 8U);
    EXPECT_FALSE(nav_sig<>::matches(wrong_id));
    EXPECT_FALSE(nav_sig<>::decode(wrong_id, out));
}

TEST(UbxMessageNavSig, RejectsShortHeader)
{
    uint8_t payload[7U] = {};
    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SIG, payload, 7U);
    nav_sig<> out;
    EXPECT_FALSE(nav_sig<>::decode(raw, out));
}

TEST(UbxMessageNavSig, RejectsGroupLengthMismatchAndResetsOutput)
{
    uint8_t payload[41U] = {};
    castle::write_le32(&payload[0U], 77U);
    payload[5U] = 2U;
    nav_sig<> out;

    message_view too_short = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SIG, payload, 39U);
    EXPECT_FALSE(nav_sig<>::decode(too_short, out));
    EXPECT_EQ(out.i_tow, 0U);
    EXPECT_EQ(out.num_sigs, 0U);
    EXPECT_TRUE(out.signals.empty());

    message_view too_long = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SIG, payload, 41U);
    EXPECT_FALSE(nav_sig<>::decode(too_long, out));

    payload[5U] = 1U;
    message_view count_mismatch = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SIG, payload, 40U);
    EXPECT_FALSE(nav_sig<>::decode(count_mismatch, out));
    EXPECT_EQ(out.num_sigs, 0U);
}

TEST(UbxMessageNavSig, ClampsStorageButKeepsWireCount)
{
    nav_sig<3U> message{};
    ASSERT_TRUE(castle::succeeded(message.signals.push_back(make_signal(0U))));
    ASSERT_TRUE(castle::succeeded(message.signals.push_back(make_signal(5U))));
    ASSERT_TRUE(castle::succeeded(message.signals.push_back(make_signal(10U))));

    uint8_t frame[80U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));

    nav_sig<2U> out;
    ASSERT_TRUE(nav_sig<2U>::decode(raw, out));
    EXPECT_EQ(out.num_sigs, 3U);
    ASSERT_EQ(out.signals.size(), 2U);
    expect_signal_eq(out.signals[0U], make_signal(0U));
    expect_signal_eq(out.signals[1U], make_signal(5U));
}

TEST(UbxMessageNavSig, EncodeIntoTooSmallBufferReportsFull)
{
    nav_sig<1U> message{};
    ASSERT_TRUE(castle::succeeded(message.signals.push_back(make_signal(0U))));
    uint8_t frame[UBX_FRAME_OVERHEAD + 8U + 15U];
    castle::size_type written = 3U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageNavSig, SigFlagAccessors)
{
    nav_sig_info info{};
    EXPECT_EQ(info.health(), nav_sv_health::unknown);
    EXPECT_FALSE(info.pr_smoothed());
    EXPECT_FALSE(info.pr_used());
    EXPECT_FALSE(info.cr_used());
    EXPECT_FALSE(info.do_used());
    EXPECT_FALSE(info.pr_corr_used());
    EXPECT_FALSE(info.cr_corr_used());
    EXPECT_FALSE(info.do_corr_used());

    info.sig_flags = 0x0001U;
    EXPECT_EQ(info.health(), nav_sv_health::healthy);
    info.sig_flags = 0x0002U;
    EXPECT_EQ(info.health(), nav_sv_health::unhealthy);
    info.sig_flags = nav_sig_info::SIG_FLAGS_PR_SMOOTHED;
    EXPECT_TRUE(info.pr_smoothed());
    EXPECT_EQ(info.health(), nav_sv_health::unknown);
    info.sig_flags = nav_sig_info::SIG_FLAGS_PR_USED;
    EXPECT_TRUE(info.pr_used());
    EXPECT_FALSE(info.pr_smoothed());
    info.sig_flags = nav_sig_info::SIG_FLAGS_CR_USED;
    EXPECT_TRUE(info.cr_used());
    info.sig_flags = nav_sig_info::SIG_FLAGS_DO_USED;
    EXPECT_TRUE(info.do_used());
    info.sig_flags = nav_sig_info::SIG_FLAGS_PR_CORR_USED;
    EXPECT_TRUE(info.pr_corr_used());
    info.sig_flags = nav_sig_info::SIG_FLAGS_CR_CORR_USED;
    EXPECT_TRUE(info.cr_corr_used());
    info.sig_flags = nav_sig_info::SIG_FLAGS_DO_CORR_USED;
    EXPECT_TRUE(info.do_corr_used());
    EXPECT_FALSE(info.cr_corr_used());

    info.sig_flags = 0xFE00U; // unassigned high bits
    EXPECT_FALSE(info.do_corr_used());
    EXPECT_EQ(info.health(), nav_sv_health::unknown);
}

TEST(UbxMessageNavSig, ScalesPseudorangeResidual)
{
    nav_sig_info info{};
    info.pr_res = 250;
    EXPECT_DOUBLE_EQ(info.pr_res_m(), 25.0);
    info.pr_res = -1;
    EXPECT_DOUBLE_EQ(info.pr_res_m(), -0.1);
}

} // namespace

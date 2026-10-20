#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_sat.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

nav_sat_sv make_sv(uint8_t seed)
{
    nav_sat_sv sv{};
    sv.gnss_id = static_cast<uint8_t>(seed + 1U);
    sv.sv_id = static_cast<uint8_t>(seed + 2U);
    sv.cno = static_cast<uint8_t>(seed + 3U);
    sv.elev = static_cast<int8_t>(-static_cast<int>(seed) - 4);
    sv.azim = static_cast<int16_t>(300 + seed);
    sv.pr_res = static_cast<int16_t>(-1000 - seed);
    sv.flags = 0x007F7FFFU - seed;
    return sv;
}

void expect_sv_eq(CASTLE_CONST nav_sat_sv& actual, CASTLE_CONST nav_sat_sv& expected)
{
    EXPECT_EQ(actual.gnss_id, expected.gnss_id);
    EXPECT_EQ(actual.sv_id, expected.sv_id);
    EXPECT_EQ(actual.cno, expected.cno);
    EXPECT_EQ(actual.elev, expected.elev);
    EXPECT_EQ(actual.azim, expected.azim);
    EXPECT_EQ(actual.pr_res, expected.pr_res);
    EXPECT_EQ(actual.flags, expected.flags);
}

TEST(UbxMessageNavSat, RoundTripsEveryField)
{
    nav_sat<4U> message{};
    message.i_tow = 0x89ABCDEFU;
    message.version = 0x01U;
    message.num_svs = 99U; // ignored on encode: count comes from svs.size()
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(10U))));
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(20U))));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD + 8U + (3U * 12U));

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(nav_sat<4U>::matches(raw));

    nav_sat<4U> decoded;
    ASSERT_TRUE(nav_sat<4U>::decode(raw, decoded));
    EXPECT_EQ(decoded.i_tow, 0x89ABCDEFU);
    EXPECT_EQ(decoded.version, 0x01U);
    EXPECT_EQ(decoded.num_svs, 3U);
    ASSERT_EQ(decoded.svs.size(), 3U);
    expect_sv_eq(decoded.svs[0U], make_sv(0U));
    expect_sv_eq(decoded.svs[1U], make_sv(10U));
    expect_sv_eq(decoded.svs[2U], make_sv(20U));
}

TEST(UbxMessageNavSat, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[32U] = {};
    castle::write_le32(&payload[0U], 0x01020304U);
    payload[4U] = 1U;
    payload[5U] = 2U;
    payload[6U] = 0xEEU; // reserved0 is ignored
    payload[7U] = 0xEEU;
    payload[8U] = 6U;
    payload[9U] = 22U;
    payload[10U] = 35U;
    payload[11U] = static_cast<uint8_t>(-12);
    castle::write_le16(&payload[12U], 1234U);
    castle::write_le16(&payload[14U], static_cast<uint16_t>(-45));
    castle::write_le32(&payload[16U], 0xA5A5A5A5U);
    payload[20U] = 3U;
    payload[21U] = 7U;
    payload[22U] = 41U;
    payload[23U] = 89U;
    castle::write_le16(&payload[24U], 359U);
    castle::write_le16(&payload[26U], 77U);
    castle::write_le32(&payload[28U], 0x00000001U);

    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SAT, payload, 32U);
    nav_sat<2U> out;
    ASSERT_TRUE(nav_sat<2U>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 0x01020304U);
    EXPECT_EQ(out.version, 1U);
    EXPECT_EQ(out.num_svs, 2U);
    ASSERT_EQ(out.svs.size(), 2U);
    EXPECT_EQ(out.svs[0U].gnss_id, 6U);
    EXPECT_EQ(out.svs[0U].sv_id, 22U);
    EXPECT_EQ(out.svs[0U].cno, 35U);
    EXPECT_EQ(out.svs[0U].elev, -12);
    EXPECT_EQ(out.svs[0U].azim, 1234);
    EXPECT_EQ(out.svs[0U].pr_res, -45);
    EXPECT_EQ(out.svs[0U].flags, 0xA5A5A5A5U);
    EXPECT_EQ(out.svs[1U].gnss_id, 3U);
    EXPECT_EQ(out.svs[1U].sv_id, 7U);
    EXPECT_EQ(out.svs[1U].cno, 41U);
    EXPECT_EQ(out.svs[1U].elev, 89);
    EXPECT_EQ(out.svs[1U].azim, 359);
    EXPECT_EQ(out.svs[1U].pr_res, 77);
    EXPECT_EQ(out.svs[1U].flags, 0x00000001U);
}

TEST(UbxMessageNavSat, EncodesSpecOffsetsAndZeroReserved)
{
    nav_sat<1U> message{};
    message.i_tow = 0x11223344U;
    message.version = 1U;
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));

    uint8_t frame[32U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(castle::read_le32(&payload[0U]), 0x11223344U);
    EXPECT_EQ(payload[4U], 1U);
    EXPECT_EQ(payload[5U], 1U);
    EXPECT_EQ(payload[6U], 0U);
    EXPECT_EQ(payload[7U], 0U);
    EXPECT_EQ(payload[8U], 1U);
    EXPECT_EQ(payload[9U], 2U);
    EXPECT_EQ(payload[10U], 3U);
    EXPECT_EQ(static_cast<int8_t>(payload[11U]), -4);
    EXPECT_EQ(castle::read_le16(&payload[12U]), 300U);
    EXPECT_EQ(static_cast<int16_t>(castle::read_le16(&payload[14U])), -1000);
    EXPECT_EQ(castle::read_le32(&payload[16U]), 0x007F7FFFU);
}

TEST(UbxMessageNavSat, AcceptsEmptyGroup)
{
    uint8_t payload[8U] = {};
    castle::write_le32(&payload[0U], 5U);
    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SAT, payload, 8U);
    nav_sat<> out;
    ASSERT_TRUE(nav_sat<>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 5U);
    EXPECT_EQ(out.num_svs, 0U);
    EXPECT_TRUE(out.svs.empty());
}

TEST(UbxMessageNavSat, RejectsWrongClassOrId)
{
    uint8_t payload[8U] = {};
    castle::write_le32(&payload[0U], 5U);
    nav_sat<> out;
    out.i_tow = 1U;

    message_view wrong_class = make_view(UBX_CLASS_RXM, UBX_ID_NAV_SAT, payload, 8U);
    EXPECT_FALSE(nav_sat<>::matches(wrong_class));
    EXPECT_FALSE(nav_sat<>::decode(wrong_class, out));
    EXPECT_EQ(out.i_tow, 0U);

    message_view wrong_id = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SIG, payload, 8U);
    EXPECT_FALSE(nav_sat<>::matches(wrong_id));
    EXPECT_FALSE(nav_sat<>::decode(wrong_id, out));
}

TEST(UbxMessageNavSat, RejectsShortHeader)
{
    uint8_t payload[7U] = {};
    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SAT, payload, 7U);
    nav_sat<> out;
    EXPECT_FALSE(nav_sat<>::decode(raw, out));
}

TEST(UbxMessageNavSat, RejectsGroupLengthMismatchAndResetsOutput)
{
    uint8_t payload[33U] = {};
    castle::write_le32(&payload[0U], 77U);
    payload[5U] = 2U;
    nav_sat<> out;

    message_view too_short = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SAT, payload, 31U);
    EXPECT_FALSE(nav_sat<>::decode(too_short, out));
    EXPECT_EQ(out.i_tow, 0U);
    EXPECT_EQ(out.num_svs, 0U);
    EXPECT_TRUE(out.svs.empty());

    message_view too_long = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SAT, payload, 33U);
    EXPECT_FALSE(nav_sat<>::decode(too_long, out));
    EXPECT_EQ(out.num_svs, 0U);

    payload[5U] = 1U; // count says one block but two are present
    message_view count_mismatch = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SAT, payload, 32U);
    EXPECT_FALSE(nav_sat<>::decode(count_mismatch, out));
}

TEST(UbxMessageNavSat, ClampsStorageButKeepsWireCount)
{
    nav_sat<3U> message{};
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(10U))));
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(20U))));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));

    nav_sat<1U> out;
    ASSERT_TRUE(nav_sat<1U>::decode(raw, out));
    EXPECT_EQ(out.num_svs, 3U);
    ASSERT_EQ(out.svs.size(), 1U);
    expect_sv_eq(out.svs[0U], make_sv(0U));
}

TEST(UbxMessageNavSat, EncodeIntoTooSmallBufferReportsFull)
{
    nav_sat<2U> message{};
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));
    uint8_t frame[UBX_FRAME_OVERHEAD + 8U + 11U];
    castle::size_type written = 7U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);

    uint8_t header_only[UBX_FRAME_OVERHEAD + 7U];
    EXPECT_EQ(encode_message(message, header_only, sizeof(header_only), written), castle::status::full);
}

TEST(UbxMessageNavSat, FlagAccessorsClearWhenZero)
{
    nav_sat_sv sv{};
    EXPECT_EQ(sv.quality_ind(), nav_signal_quality::no_signal);
    EXPECT_FALSE(sv.sv_used());
    EXPECT_EQ(sv.health(), nav_sv_health::unknown);
    EXPECT_FALSE(sv.diff_corr());
    EXPECT_FALSE(sv.smoothed());
    EXPECT_EQ(sv.orbit_source(), nav_sat_orbit_source::none);
    EXPECT_FALSE(sv.eph_avail());
    EXPECT_FALSE(sv.alm_avail());
    EXPECT_FALSE(sv.ano_avail());
    EXPECT_FALSE(sv.aop_avail());
    EXPECT_FALSE(sv.sbas_corr_used());
    EXPECT_FALSE(sv.rtcm_corr_used());
    EXPECT_FALSE(sv.slas_corr_used());
    EXPECT_FALSE(sv.spartn_corr_used());
    EXPECT_FALSE(sv.pr_corr_used());
    EXPECT_FALSE(sv.cr_corr_used());
    EXPECT_FALSE(sv.do_corr_used());
}

TEST(UbxMessageNavSat, FlagAccessorsDecodeEachBit)
{
    nav_sat_sv sv{};
    sv.flags = 0x00000004U;
    EXPECT_EQ(sv.quality_ind(), nav_signal_quality::code_locked);
    sv.flags = 0x00000007U | nav_sat_sv::FLAGS_SV_USED;
    EXPECT_EQ(static_cast<uint8_t>(sv.quality_ind()), 7U);
    EXPECT_TRUE(sv.sv_used());
    EXPECT_EQ(sv.health(), nav_sv_health::unknown);

    sv.flags = 0x00000020U;
    EXPECT_EQ(sv.health(), nav_sv_health::unhealthy);
    EXPECT_EQ(sv.quality_ind(), nav_signal_quality::no_signal);
    sv.flags = 0x00000010U;
    EXPECT_EQ(sv.health(), nav_sv_health::healthy);

    sv.flags = nav_sat_sv::FLAGS_DIFF_CORR;
    EXPECT_TRUE(sv.diff_corr());
    EXPECT_FALSE(sv.smoothed());
    sv.flags = nav_sat_sv::FLAGS_SMOOTHED;
    EXPECT_TRUE(sv.smoothed());
    EXPECT_FALSE(sv.diff_corr());

    sv.flags = 0x00000300U;
    EXPECT_EQ(sv.orbit_source(), nav_sat_orbit_source::assistnow_offline);
    sv.flags = 0x00000400U;
    EXPECT_EQ(sv.orbit_source(), nav_sat_orbit_source::assistnow_autonomous);
    sv.flags = 0x00000700U;
    EXPECT_EQ(static_cast<uint8_t>(sv.orbit_source()), 7U);
    EXPECT_FALSE(sv.eph_avail());

    sv.flags = nav_sat_sv::FLAGS_EPH_AVAIL;
    EXPECT_TRUE(sv.eph_avail());
    EXPECT_EQ(sv.orbit_source(), nav_sat_orbit_source::none);
    sv.flags = nav_sat_sv::FLAGS_ALM_AVAIL;
    EXPECT_TRUE(sv.alm_avail());
    sv.flags = nav_sat_sv::FLAGS_ANO_AVAIL;
    EXPECT_TRUE(sv.ano_avail());
    sv.flags = nav_sat_sv::FLAGS_AOP_AVAIL;
    EXPECT_TRUE(sv.aop_avail());
    EXPECT_FALSE(sv.ano_avail());

    sv.flags = nav_sat_sv::FLAGS_SBAS_CORR_USED;
    EXPECT_TRUE(sv.sbas_corr_used());
    sv.flags = nav_sat_sv::FLAGS_RTCM_CORR_USED;
    EXPECT_TRUE(sv.rtcm_corr_used());
    EXPECT_FALSE(sv.sbas_corr_used());
    sv.flags = nav_sat_sv::FLAGS_SLAS_CORR_USED;
    EXPECT_TRUE(sv.slas_corr_used());
    sv.flags = nav_sat_sv::FLAGS_SPARTN_CORR_USED;
    EXPECT_TRUE(sv.spartn_corr_used());
    sv.flags = nav_sat_sv::FLAGS_PR_CORR_USED;
    EXPECT_TRUE(sv.pr_corr_used());
    sv.flags = nav_sat_sv::FLAGS_CR_CORR_USED;
    EXPECT_TRUE(sv.cr_corr_used());
    sv.flags = nav_sat_sv::FLAGS_DO_CORR_USED;
    EXPECT_TRUE(sv.do_corr_used());
    EXPECT_FALSE(sv.cr_corr_used());

    // Bit 15 is not assigned: no accessor reports it.
    sv.flags = 0x00008000U;
    EXPECT_FALSE(sv.aop_avail());
    EXPECT_FALSE(sv.sbas_corr_used());
}

TEST(UbxMessageNavSat, ScalesPseudorangeResidual)
{
    nav_sat_sv sv{};
    sv.pr_res = -123;
    EXPECT_DOUBLE_EQ(sv.pr_res_m(), -12.3);
    sv.pr_res = 0;
    EXPECT_DOUBLE_EQ(sv.pr_res_m(), 0.0);
}

} // namespace

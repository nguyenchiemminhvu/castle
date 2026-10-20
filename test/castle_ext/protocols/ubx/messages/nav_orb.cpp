#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_orb.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

nav_orb_sv make_sv(uint8_t seed)
{
    nav_orb_sv sv{};
    sv.gnss_id = static_cast<uint8_t>(seed + 1U);
    sv.sv_id = static_cast<uint8_t>(seed + 2U);
    sv.sv_flag = static_cast<uint8_t>(0x0DU + seed);
    sv.eph = static_cast<uint8_t>(0xA3U + seed);
    sv.alm = static_cast<uint8_t>(0x5CU + seed);
    sv.other_orb = static_cast<uint8_t>(0xE7U + seed);
    return sv;
}

void expect_sv_eq(CASTLE_CONST nav_orb_sv& actual, CASTLE_CONST nav_orb_sv& expected)
{
    EXPECT_EQ(actual.gnss_id, expected.gnss_id);
    EXPECT_EQ(actual.sv_id, expected.sv_id);
    EXPECT_EQ(actual.sv_flag, expected.sv_flag);
    EXPECT_EQ(actual.eph, expected.eph);
    EXPECT_EQ(actual.alm, expected.alm);
    EXPECT_EQ(actual.other_orb, expected.other_orb);
}

TEST(UbxMessageNavOrb, RoundTripsEveryField)
{
    nav_orb<4U> message{};
    message.i_tow = 0x0BADF00DU;
    message.version = 0x01U;
    message.num_sv = 77U; // ignored on encode: count comes from svs.size()
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(1U))));
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(2U))));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD + 8U + (3U * 6U));

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(nav_orb<4U>::matches(raw));

    nav_orb<4U> decoded;
    ASSERT_TRUE(nav_orb<4U>::decode(raw, decoded));
    EXPECT_EQ(decoded.i_tow, 0x0BADF00DU);
    EXPECT_EQ(decoded.version, 0x01U);
    EXPECT_EQ(decoded.num_sv, 3U);
    ASSERT_EQ(decoded.svs.size(), 3U);
    expect_sv_eq(decoded.svs[0U], make_sv(0U));
    expect_sv_eq(decoded.svs[1U], make_sv(1U));
    expect_sv_eq(decoded.svs[2U], make_sv(2U));
}

TEST(UbxMessageNavOrb, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[20U] = {};
    castle::write_le32(&payload[0U], 0x10203040U);
    payload[4U] = 1U;
    payload[5U] = 2U;
    payload[6U] = 0xEEU; // reserved0 is ignored
    payload[7U] = 0xEEU;
    payload[8U] = 0U;
    payload[9U] = 31U;
    payload[10U] = 0x09U;
    payload[11U] = 0x3EU;
    payload[12U] = 0x5FU;
    payload[13U] = 0x41U;
    payload[14U] = 2U;
    payload[15U] = 36U;
    payload[16U] = 0x06U;
    payload[17U] = 0x00U;
    payload[18U] = 0xFFU;
    payload[19U] = 0x80U;

    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_ORB, payload, 20U);
    nav_orb<2U> out;
    ASSERT_TRUE(nav_orb<2U>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 0x10203040U);
    EXPECT_EQ(out.version, 1U);
    EXPECT_EQ(out.num_sv, 2U);
    ASSERT_EQ(out.svs.size(), 2U);
    EXPECT_EQ(out.svs[0U].gnss_id, 0U);
    EXPECT_EQ(out.svs[0U].sv_id, 31U);
    EXPECT_EQ(out.svs[0U].sv_flag, 0x09U);
    EXPECT_EQ(out.svs[0U].eph, 0x3EU);
    EXPECT_EQ(out.svs[0U].alm, 0x5FU);
    EXPECT_EQ(out.svs[0U].other_orb, 0x41U);
    EXPECT_EQ(out.svs[1U].gnss_id, 2U);
    EXPECT_EQ(out.svs[1U].sv_id, 36U);
    EXPECT_EQ(out.svs[1U].sv_flag, 0x06U);
    EXPECT_EQ(out.svs[1U].eph, 0x00U);
    EXPECT_EQ(out.svs[1U].alm, 0xFFU);
    EXPECT_EQ(out.svs[1U].other_orb, 0x80U);

    // 0x09 = visibility 2, health 1; 0x3E = source 1, usability 30;
    // 0x5F = source 2, usability 31; 0x41 = type 2, usability 1.
    EXPECT_EQ(out.svs[0U].health(), nav_sv_health::healthy);
    EXPECT_EQ(out.svs[0U].visibility(), nav_orb_visibility::above_horizon);
    EXPECT_EQ(out.svs[0U].eph_usability(), nav_orb_sv::USABILITY_MAX);
    EXPECT_EQ(out.svs[0U].eph_source(), nav_orb_source::gnss_transmission);
    EXPECT_EQ(out.svs[0U].alm_usability(), nav_orb_sv::USABILITY_UNKNOWN);
    EXPECT_EQ(out.svs[0U].alm_source(), nav_orb_source::external_aiding);
    EXPECT_EQ(out.svs[0U].ano_aop_usability(), 1U);
    EXPECT_EQ(out.svs[0U].other_orb_type(), nav_orb_other_type::assistnow_autonomous);

    // 0x06 = visibility 1, health 2; 0xFF = source 7 ("other"), usability 31; 0x80 = type 4 ("other").
    EXPECT_EQ(out.svs[1U].health(), nav_sv_health::unhealthy);
    EXPECT_EQ(out.svs[1U].visibility(), nav_orb_visibility::below_horizon);
    EXPECT_EQ(out.svs[1U].eph_usability(), nav_orb_sv::USABILITY_EXPIRED);
    EXPECT_EQ(out.svs[1U].eph_source(), nav_orb_source::not_available);
    EXPECT_EQ(static_cast<uint8_t>(out.svs[1U].alm_source()), 7U);
    EXPECT_EQ(out.svs[1U].alm_usability(), 31U);
    EXPECT_EQ(out.svs[1U].ano_aop_usability(), 0U);
    EXPECT_EQ(static_cast<uint8_t>(out.svs[1U].other_orb_type()), 4U);
}

TEST(UbxMessageNavOrb, EncodesSpecOffsetsAndZeroReserved)
{
    nav_orb<1U> message{};
    message.i_tow = 0xA1B2C3D4U;
    message.version = 1U;
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));

    uint8_t frame[32U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(castle::read_le32(&payload[0U]), 0xA1B2C3D4U);
    EXPECT_EQ(payload[4U], 1U);
    EXPECT_EQ(payload[5U], 1U);
    EXPECT_EQ(payload[6U], 0U);
    EXPECT_EQ(payload[7U], 0U);
    EXPECT_EQ(payload[8U], 1U);
    EXPECT_EQ(payload[9U], 2U);
    EXPECT_EQ(payload[10U], 0x0DU);
    EXPECT_EQ(payload[11U], 0xA3U);
    EXPECT_EQ(payload[12U], 0x5CU);
    EXPECT_EQ(payload[13U], 0xE7U);
}

TEST(UbxMessageNavOrb, AcceptsEmptyGroup)
{
    uint8_t payload[8U] = {};
    castle::write_le32(&payload[0U], 3U);
    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_ORB, payload, 8U);
    nav_orb<> out;
    ASSERT_TRUE(nav_orb<>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 3U);
    EXPECT_EQ(out.num_sv, 0U);
    EXPECT_TRUE(out.svs.empty());
}

TEST(UbxMessageNavOrb, RejectsWrongClassOrId)
{
    uint8_t payload[8U] = {};
    castle::write_le32(&payload[0U], 3U);
    nav_orb<> out;
    out.i_tow = 1U;

    message_view wrong_class = make_view(UBX_CLASS_RXM, UBX_ID_NAV_ORB, payload, 8U);
    EXPECT_FALSE(nav_orb<>::matches(wrong_class));
    EXPECT_FALSE(nav_orb<>::decode(wrong_class, out));
    EXPECT_EQ(out.i_tow, 0U);

    message_view wrong_id = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SAT, payload, 8U);
    EXPECT_FALSE(nav_orb<>::matches(wrong_id));
    EXPECT_FALSE(nav_orb<>::decode(wrong_id, out));
}

TEST(UbxMessageNavOrb, RejectsShortHeader)
{
    uint8_t payload[7U] = {};
    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_ORB, payload, 7U);
    nav_orb<> out;
    EXPECT_FALSE(nav_orb<>::decode(raw, out));
}

TEST(UbxMessageNavOrb, RejectsGroupLengthMismatchAndResetsOutput)
{
    uint8_t payload[21U] = {};
    castle::write_le32(&payload[0U], 77U);
    payload[5U] = 2U;
    nav_orb<> out;

    message_view too_short = make_view(UBX_CLASS_NAV, UBX_ID_NAV_ORB, payload, 19U);
    EXPECT_FALSE(nav_orb<>::decode(too_short, out));
    EXPECT_EQ(out.i_tow, 0U);
    EXPECT_EQ(out.num_sv, 0U);
    EXPECT_TRUE(out.svs.empty());

    message_view too_long = make_view(UBX_CLASS_NAV, UBX_ID_NAV_ORB, payload, 21U);
    EXPECT_FALSE(nav_orb<>::decode(too_long, out));

    payload[5U] = 3U;
    message_view count_mismatch = make_view(UBX_CLASS_NAV, UBX_ID_NAV_ORB, payload, 20U);
    EXPECT_FALSE(nav_orb<>::decode(count_mismatch, out));
    EXPECT_EQ(out.num_sv, 0U);
}

TEST(UbxMessageNavOrb, ClampsStorageButKeepsWireCount)
{
    nav_orb<3U> message{};
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(1U))));
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(2U))));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));

    nav_orb<1U> out;
    ASSERT_TRUE(nav_orb<1U>::decode(raw, out));
    EXPECT_EQ(out.num_sv, 3U);
    ASSERT_EQ(out.svs.size(), 1U);
    expect_sv_eq(out.svs[0U], make_sv(0U));
}

TEST(UbxMessageNavOrb, EncodeIntoTooSmallBufferReportsFull)
{
    nav_orb<1U> message{};
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));
    uint8_t frame[UBX_FRAME_OVERHEAD + 8U + 5U];
    castle::size_type written = 5U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageNavOrb, SubFieldAccessorsIsolateTheirBits)
{
    nav_orb_sv sv{};
    EXPECT_EQ(sv.health(), nav_sv_health::unknown);
    EXPECT_EQ(sv.visibility(), nav_orb_visibility::unknown);
    EXPECT_EQ(sv.eph_usability(), 0U);
    EXPECT_EQ(sv.eph_source(), nav_orb_source::not_available);
    EXPECT_EQ(sv.alm_usability(), 0U);
    EXPECT_EQ(sv.alm_source(), nav_orb_source::not_available);
    EXPECT_EQ(sv.ano_aop_usability(), 0U);
    EXPECT_EQ(sv.other_orb_type(), nav_orb_other_type::none);

    sv.sv_flag = nav_orb_sv::SV_FLAG_HEALTH;
    EXPECT_EQ(static_cast<uint8_t>(sv.health()), 3U);
    EXPECT_EQ(sv.visibility(), nav_orb_visibility::unknown);
    sv.sv_flag = nav_orb_sv::SV_FLAG_VISIBILITY;
    EXPECT_EQ(sv.visibility(), nav_orb_visibility::above_elevation_mask);
    EXPECT_EQ(sv.health(), nav_sv_health::unknown);
    sv.sv_flag = 0xF0U; // unassigned high bits
    EXPECT_EQ(sv.visibility(), nav_orb_visibility::unknown);
    EXPECT_EQ(sv.health(), nav_sv_health::unknown);

    sv.eph = nav_orb_sv::EPH_USABILITY;
    EXPECT_EQ(sv.eph_usability(), nav_orb_sv::USABILITY_UNKNOWN);
    EXPECT_EQ(sv.eph_source(), nav_orb_source::not_available);
    sv.eph = nav_orb_sv::EPH_SOURCE;
    EXPECT_EQ(sv.eph_usability(), 0U);
    EXPECT_EQ(static_cast<uint8_t>(sv.eph_source()), 7U);

    sv.alm = static_cast<uint8_t>(2U << 5U);
    EXPECT_EQ(sv.alm_source(), nav_orb_source::external_aiding);
    EXPECT_EQ(sv.alm_usability(), 0U);
    sv.alm = 0x0FU;
    EXPECT_EQ(sv.alm_usability(), 15U);
    EXPECT_EQ(sv.alm_source(), nav_orb_source::not_available);

    sv.other_orb = static_cast<uint8_t>(1U << 5U);
    EXPECT_EQ(sv.other_orb_type(), nav_orb_other_type::assistnow_offline);
    EXPECT_EQ(sv.ano_aop_usability(), 0U);
    sv.other_orb = nav_orb_sv::OTHER_ORB_ANO_AOP_USABILITY;
    EXPECT_EQ(sv.ano_aop_usability(), nav_orb_sv::USABILITY_UNKNOWN);
    EXPECT_EQ(sv.other_orb_type(), nav_orb_other_type::none);
}

} // namespace

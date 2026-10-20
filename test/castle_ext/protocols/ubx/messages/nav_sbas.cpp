#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_sbas.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view make_view(uint8_t msg_class, uint8_t msg_id, CASTLE_CONST uint8_t* payload, uint16_t length)
{
    return message_view{{msg_class, msg_id, length}, array_view<CASTLE_CONST uint8_t>(payload, length), checksum{}};
}

nav_sbas_sv make_sv(uint8_t seed)
{
    nav_sbas_sv sv{};
    sv.svid = static_cast<uint8_t>(120U + seed);
    sv.flags = static_cast<uint8_t>(0xA0U + seed);
    sv.udre = static_cast<uint8_t>(0x0BU + seed);
    sv.sv_sys = nav_sbas_system::msas;
    sv.sv_service = static_cast<uint8_t>(0x15U + seed);
    sv.prc = static_cast<int16_t>(-3000 - seed);
    sv.ic = static_cast<int16_t>(1500 + seed);
    return sv;
}

void expect_sv_eq(CASTLE_CONST nav_sbas_sv& actual, CASTLE_CONST nav_sbas_sv& expected)
{
    EXPECT_EQ(actual.svid, expected.svid);
    EXPECT_EQ(actual.flags, expected.flags);
    EXPECT_EQ(actual.udre, expected.udre);
    EXPECT_EQ(actual.sv_sys, expected.sv_sys);
    EXPECT_EQ(actual.sv_service, expected.sv_service);
    EXPECT_EQ(actual.prc, expected.prc);
    EXPECT_EQ(actual.ic, expected.ic);
}

nav_sbas<4U> make_message()
{
    nav_sbas<4U> message{};
    message.i_tow = 0x13579BDFU;
    message.geo = 137U;
    message.mode = nav_sbas_mode::enabled_test_mode;
    message.sys = nav_sbas_system::gagan;
    message.service = 0x1BU;
    message.cnt = 250U; // ignored on encode: count comes from svs.size()
    message.status_flags = 0x02U;
    return message;
}

TEST(UbxMessageNavSbas, RoundTripsEveryField)
{
    nav_sbas<4U> message = make_message();
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(3U))));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    EXPECT_EQ(written, UBX_FRAME_OVERHEAD + 12U + (2U * 12U));

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    EXPECT_TRUE(nav_sbas<4U>::matches(raw));

    nav_sbas<4U> decoded;
    ASSERT_TRUE(nav_sbas<4U>::decode(raw, decoded));
    EXPECT_EQ(decoded.i_tow, 0x13579BDFU);
    EXPECT_EQ(decoded.geo, 137U);
    EXPECT_EQ(decoded.mode, nav_sbas_mode::enabled_test_mode);
    EXPECT_EQ(decoded.sys, nav_sbas_system::gagan);
    EXPECT_EQ(decoded.service, 0x1BU);
    EXPECT_EQ(decoded.cnt, 2U);
    EXPECT_EQ(decoded.status_flags, 0x02U);
    ASSERT_EQ(decoded.svs.size(), 2U);
    expect_sv_eq(decoded.svs[0U], make_sv(0U));
    expect_sv_eq(decoded.svs[1U], make_sv(3U));
}

TEST(UbxMessageNavSbas, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[24U] = {};
    castle::write_le32(&payload[0U], 0x00ABCDEFU);
    payload[4U] = 131U;
    payload[5U] = 1U;
    payload[6U] = static_cast<uint8_t>(-1);
    payload[7U] = 0x13U;
    payload[8U] = 1U;
    payload[9U] = 0x01U;
    payload[10U] = 0xEEU; // reserved0 is ignored
    payload[11U] = 0xEEU;
    payload[12U] = 133U;
    payload[13U] = 0x5AU;
    payload[14U] = 15U;
    payload[15U] = 16U;
    payload[16U] = 0x0CU;
    payload[17U] = 0xEEU; // reserved1
    castle::write_le16(&payload[18U], static_cast<uint16_t>(-1234));
    payload[20U] = 0xEEU; // reserved2
    payload[21U] = 0xEEU;
    castle::write_le16(&payload[22U], 4321U);

    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SBAS, payload, 24U);
    nav_sbas<2U> out;
    ASSERT_TRUE(nav_sbas<2U>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 0x00ABCDEFU);
    EXPECT_EQ(out.geo, 131U);
    EXPECT_EQ(out.mode, nav_sbas_mode::enabled_integrity);
    EXPECT_EQ(out.sys, nav_sbas_system::unknown);
    EXPECT_EQ(out.service, 0x13U);
    EXPECT_EQ(out.cnt, 1U);
    EXPECT_EQ(out.status_flags, 0x01U);
    EXPECT_EQ(out.integrity_used(), nav_sbas_integrity_used::not_available);
    ASSERT_EQ(out.svs.size(), 1U);
    EXPECT_EQ(out.svs[0U].svid, 133U);
    EXPECT_EQ(out.svs[0U].flags, 0x5AU);
    EXPECT_EQ(out.svs[0U].udre, 15U);
    EXPECT_EQ(out.svs[0U].sv_sys, nav_sbas_system::gps);
    EXPECT_EQ(out.svs[0U].sv_service, 0x0CU);
    EXPECT_EQ(out.svs[0U].prc, -1234);
    EXPECT_EQ(out.svs[0U].ic, 4321);
}

TEST(UbxMessageNavSbas, EncodesSpecOffsetsAndZeroReserved)
{
    nav_sbas<4U> message = make_message();
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));

    uint8_t frame[40U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    CASTLE_CONST uint8_t* payload = &frame[UBX_FRAME_INDEX_PAYLOAD];
    EXPECT_EQ(castle::read_le32(&payload[0U]), 0x13579BDFU);
    EXPECT_EQ(payload[4U], 137U);
    EXPECT_EQ(payload[5U], 3U);
    EXPECT_EQ(payload[6U], 3U);
    EXPECT_EQ(payload[7U], 0x1BU);
    EXPECT_EQ(payload[8U], 1U);
    EXPECT_EQ(payload[9U], 0x02U);
    EXPECT_EQ(payload[10U], 0U);
    EXPECT_EQ(payload[11U], 0U);
    EXPECT_EQ(payload[12U], 120U);
    EXPECT_EQ(payload[13U], 0xA0U);
    EXPECT_EQ(payload[14U], 0x0BU);
    EXPECT_EQ(payload[15U], 2U);
    EXPECT_EQ(payload[16U], 0x15U);
    EXPECT_EQ(payload[17U], 0U);
    EXPECT_EQ(static_cast<int16_t>(castle::read_le16(&payload[18U])), -3000);
    EXPECT_EQ(payload[20U], 0U);
    EXPECT_EQ(payload[21U], 0U);
    EXPECT_EQ(castle::read_le16(&payload[22U]), 1500U);
}

TEST(UbxMessageNavSbas, AcceptsEmptyGroup)
{
    uint8_t payload[12U] = {};
    castle::write_le32(&payload[0U], 8U);
    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SBAS, payload, 12U);
    nav_sbas<> out;
    ASSERT_TRUE(nav_sbas<>::decode(raw, out));
    EXPECT_EQ(out.i_tow, 8U);
    EXPECT_EQ(out.sys, nav_sbas_system::waas);
    EXPECT_EQ(out.cnt, 0U);
    EXPECT_TRUE(out.svs.empty());
}

TEST(UbxMessageNavSbas, RejectsWrongClassOrId)
{
    uint8_t payload[12U] = {};
    castle::write_le32(&payload[0U], 8U);
    nav_sbas<> out;
    out.i_tow = 1U;

    message_view wrong_class = make_view(UBX_CLASS_RXM, UBX_ID_NAV_SBAS, payload, 12U);
    EXPECT_FALSE(nav_sbas<>::matches(wrong_class));
    EXPECT_FALSE(nav_sbas<>::decode(wrong_class, out));
    EXPECT_EQ(out.i_tow, 0U);

    message_view wrong_id = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SLAS, payload, 12U);
    EXPECT_FALSE(nav_sbas<>::matches(wrong_id));
    EXPECT_FALSE(nav_sbas<>::decode(wrong_id, out));
}

TEST(UbxMessageNavSbas, RejectsShortHeader)
{
    uint8_t payload[11U] = {};
    message_view raw = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SBAS, payload, 11U);
    nav_sbas<> out;
    EXPECT_FALSE(nav_sbas<>::decode(raw, out));
}

TEST(UbxMessageNavSbas, RejectsGroupLengthMismatchAndResetsOutput)
{
    uint8_t payload[37U] = {};
    castle::write_le32(&payload[0U], 77U);
    payload[4U] = 120U;
    payload[8U] = 2U;
    nav_sbas<> out;

    message_view too_short = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SBAS, payload, 35U);
    EXPECT_FALSE(nav_sbas<>::decode(too_short, out));
    EXPECT_EQ(out.i_tow, 0U);
    EXPECT_EQ(out.geo, 0U);
    EXPECT_EQ(out.cnt, 0U);
    EXPECT_TRUE(out.svs.empty());

    message_view too_long = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SBAS, payload, 37U);
    EXPECT_FALSE(nav_sbas<>::decode(too_long, out));

    payload[8U] = 1U;
    message_view count_mismatch = make_view(UBX_CLASS_NAV, UBX_ID_NAV_SBAS, payload, 36U);
    EXPECT_FALSE(nav_sbas<>::decode(count_mismatch, out));
    EXPECT_EQ(out.cnt, 0U);
}

TEST(UbxMessageNavSbas, ClampsStorageButKeepsWireCount)
{
    nav_sbas<4U> message = make_message();
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(1U))));
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(2U))));

    uint8_t frame[64U];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::ok);
    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));

    nav_sbas<1U> out;
    ASSERT_TRUE(nav_sbas<1U>::decode(raw, out));
    EXPECT_EQ(out.cnt, 3U);
    ASSERT_EQ(out.svs.size(), 1U);
    expect_sv_eq(out.svs[0U], make_sv(0U));
}

TEST(UbxMessageNavSbas, EncodeIntoTooSmallBufferReportsFull)
{
    nav_sbas<4U> message = make_message();
    ASSERT_TRUE(castle::succeeded(message.svs.push_back(make_sv(0U))));
    uint8_t frame[UBX_FRAME_OVERHEAD + 12U + 11U];
    castle::size_type written = 9U;
    EXPECT_EQ(encode_message(message, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageNavSbas, ServiceAndStatusAccessors)
{
    nav_sbas<> message{};
    EXPECT_FALSE(message.ranging());
    EXPECT_FALSE(message.corrections());
    EXPECT_FALSE(message.integrity());
    EXPECT_FALSE(message.test_mode());
    EXPECT_FALSE(message.bad());
    EXPECT_EQ(message.integrity_used(), nav_sbas_integrity_used::unknown);

    message.service = nav_sbas<>::SERVICE_RANGING;
    EXPECT_TRUE(message.ranging());
    EXPECT_FALSE(message.corrections());
    message.service = nav_sbas<>::SERVICE_CORRECTIONS;
    EXPECT_TRUE(message.corrections());
    message.service = nav_sbas<>::SERVICE_INTEGRITY;
    EXPECT_TRUE(message.integrity());
    message.service = nav_sbas<>::SERVICE_TESTMODE;
    EXPECT_TRUE(message.test_mode());
    message.service = nav_sbas<>::SERVICE_BAD;
    EXPECT_TRUE(message.bad());
    EXPECT_FALSE(message.test_mode());

    message.status_flags = 0x02U;
    EXPECT_EQ(message.integrity_used(), nav_sbas_integrity_used::gps_only);
    message.status_flags = 0xFDU; // only bits 1..0 belong to integrityUsed
    EXPECT_EQ(message.integrity_used(), nav_sbas_integrity_used::not_available);
}

TEST(UbxMessageNavSbas, SvServiceAccessors)
{
    nav_sbas_sv sv{};
    EXPECT_FALSE(sv.ranging());
    EXPECT_FALSE(sv.corrections());
    EXPECT_FALSE(sv.integrity());
    EXPECT_FALSE(sv.test_mode());
    EXPECT_FALSE(sv.bad());

    sv.sv_service = nav_sbas_sv::SV_SERVICE_RANGING;
    EXPECT_TRUE(sv.ranging());
    sv.sv_service = nav_sbas_sv::SV_SERVICE_CORRECTIONS;
    EXPECT_TRUE(sv.corrections());
    EXPECT_FALSE(sv.ranging());
    sv.sv_service = nav_sbas_sv::SV_SERVICE_INTEGRITY;
    EXPECT_TRUE(sv.integrity());
    sv.sv_service = nav_sbas_sv::SV_SERVICE_TESTMODE;
    EXPECT_TRUE(sv.test_mode());
    sv.sv_service = nav_sbas_sv::SV_SERVICE_BAD;
    EXPECT_TRUE(sv.bad());
    sv.sv_service = 0xE0U; // unassigned bits
    EXPECT_FALSE(sv.bad());
    EXPECT_FALSE(sv.ranging());
}

TEST(UbxMessageNavSbas, ScalesCorrections)
{
    nav_sbas_sv sv{};
    sv.prc = -250;
    sv.ic = 1234;
    EXPECT_DOUBLE_EQ(sv.prc_m(), -2.5);
    EXPECT_DOUBLE_EQ(sv.ic_m(), 12.34);
}

TEST(UbxMessageNavSbas, EnumValuesMatchSpec)
{
    EXPECT_EQ(static_cast<uint8_t>(nav_sbas_mode::disabled), 0U);
    EXPECT_EQ(static_cast<uint8_t>(nav_sbas_mode::enabled_integrity), 1U);
    EXPECT_EQ(static_cast<uint8_t>(nav_sbas_mode::enabled_test_mode), 3U);
    EXPECT_EQ(static_cast<int8_t>(nav_sbas_system::unknown), -1);
    EXPECT_EQ(static_cast<int8_t>(nav_sbas_system::waas), 0);
    EXPECT_EQ(static_cast<int8_t>(nav_sbas_system::egnos), 1);
    EXPECT_EQ(static_cast<int8_t>(nav_sbas_system::msas), 2);
    EXPECT_EQ(static_cast<int8_t>(nav_sbas_system::gagan), 3);
    EXPECT_EQ(static_cast<int8_t>(nav_sbas_system::gps), 16);
}

} // namespace

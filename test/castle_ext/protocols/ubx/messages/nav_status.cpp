#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_status.hpp"
#include "castle/utility/bytes.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

using message = nav_status;
CASTLE_CONSTEXPR uint8_t expected_class = UBX_CLASS_NAV;
CASTLE_CONSTEXPR uint8_t expected_id = UBX_ID_NAV_STATUS;
CASTLE_CONSTEXPR uint8_t other_class = UBX_CLASS_NAV2;
CASTLE_CONSTEXPR castle::size_type length = 16U;

message_view view_of(CASTLE_CONST uint8_t* payload, castle::size_type size,
                     uint8_t msg_class = expected_class, uint8_t msg_id = expected_id)
{
    return message_view{{msg_class, msg_id, static_cast<uint16_t>(size)},
                        array_view<CASTLE_CONST uint8_t>(payload, size), checksum{}};
}

message make_message()
{
    message value;
    value.i_tow = 345600123U;
    value.gps_fix = nav_status_gps_fix::gnss_dr;
    value.flags = 15U;
    value.fix_stat = 195U;
    value.flags2 = 154U;
    value.ttff = 29000U;
    value.msss = 123456789U;
    return value;
}

// Builds the payload of make_message() byte by byte at the spec offsets.
void build_payload(uint8_t* p, uint8_t reserved_fill)
{
    for (castle::size_type i = 0U; i < length; ++i)
    {
        p[i] = 0U;
    }
    castle::write_le32(&p[0U], 345600123U);
    p[4U] = 4U;
    p[5U] = 15U;
    p[6U] = 195U;
    p[7U] = 154U;
    castle::write_le32(&p[8U], 29000U);
    castle::write_le32(&p[12U], 123456789U);
    static_cast<void>(reserved_fill);
}

void expect_sample_fields(CASTLE_CONST message& value)
{
    EXPECT_EQ(value.i_tow, 345600123U);
    EXPECT_EQ(value.gps_fix, nav_status_gps_fix::gnss_dr);
    EXPECT_EQ(value.flags, 15U);
    EXPECT_EQ(value.fix_stat, 195U);
    EXPECT_EQ(value.flags2, 154U);
    EXPECT_EQ(value.ttff, 29000U);
    EXPECT_EQ(value.msss, 123456789U);
}

TEST(UbxMessageNavStatus, IdentityConstants)
{
    EXPECT_EQ(message::msg_class, expected_class);
    EXPECT_EQ(message::msg_id, expected_id);
    EXPECT_EQ(message::payload_length, length);
}

TEST(UbxMessageNavStatus, DecodesHandBuiltPayloadAtSpecOffsets)
{
    uint8_t payload[length];
    build_payload(payload, 0xA5U);
    message out;
    EXPECT_TRUE(message::matches(view_of(payload, length)));
    ASSERT_TRUE(message::decode(view_of(payload, length), out));
    expect_sample_fields(out);
}

TEST(UbxMessageNavStatus, EncodeMatchesSpecBytesAndRoundTrips)
{
    uint8_t expected[length];
    build_payload(expected, 0U);

    uint8_t frame[frame_size(length)];
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(make_message(), frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(length));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_CLASS], expected_class);
    EXPECT_EQ(frame[UBX_FRAME_INDEX_ID], expected_id);
    for (castle::size_type i = 0U; i < length; ++i)
    {
        EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + i], expected[i]) << "offset " << i;
    }

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    message out;
    ASSERT_TRUE(message::decode(raw, out));
    expect_sample_fields(out);
}

TEST(UbxMessageNavStatus, RejectsWrongClassOrId)
{
    uint8_t payload[length];
    build_payload(payload, 0U);
    message out = make_message();

    EXPECT_FALSE(message::matches(view_of(payload, length, other_class, expected_id)));
    EXPECT_FALSE(message::decode(view_of(payload, length, other_class, expected_id), out));
    EXPECT_EQ(out.i_tow, 0U);

    out = make_message();
    EXPECT_FALSE(message::matches(view_of(payload, length, expected_class, UBX_ID_NAV_PVT)));
    EXPECT_FALSE(message::decode(view_of(payload, length, expected_class, UBX_ID_NAV_PVT), out));
    EXPECT_EQ(out.i_tow, 0U);
}

TEST(UbxMessageNavStatus, RejectsShortAndLongPayloads)
{
    uint8_t payload[length + 1U];
    build_payload(payload, 0U);
    payload[length] = 0U;
    message out = make_message();

    EXPECT_FALSE(message::decode(view_of(payload, length - 1U), out));
    EXPECT_EQ(out.i_tow, 0U);

    out = make_message();
    EXPECT_FALSE(message::decode(view_of(payload, length + 1U), out));
    EXPECT_EQ(out.i_tow, 0U);

    EXPECT_FALSE(message::decode(view_of(payload, 0U), out));
}

TEST(UbxMessageNavStatus, EncodeIntoTooSmallBufferIsFull)
{
    uint8_t frame[frame_size(length)];
    castle::size_type written = 123U;
    EXPECT_EQ(encode_message(make_message(), frame, sizeof(frame) - 1U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
    EXPECT_EQ(encode_message(make_message(), frame, 4U, written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

TEST(UbxMessageNavStatus, FlagsBits)
{
    message value;
    EXPECT_FALSE(value.fix_ok());
    EXPECT_FALSE(value.diff_soln());
    EXPECT_FALSE(value.wkn_set());
    EXPECT_FALSE(value.tow_set());

    value.flags = message::FLAGS_GPS_FIX_OK;
    EXPECT_TRUE(value.fix_ok());
    EXPECT_FALSE(value.diff_soln());
    value.flags = message::FLAGS_DIFF_SOLN;
    EXPECT_TRUE(value.diff_soln());
    EXPECT_FALSE(value.wkn_set());
    value.flags = message::FLAGS_WKN_SET;
    EXPECT_TRUE(value.wkn_set());
    EXPECT_FALSE(value.tow_set());
    value.flags = message::FLAGS_TOW_SET;
    EXPECT_TRUE(value.tow_set());
    EXPECT_FALSE(value.fix_ok());
}

TEST(UbxMessageNavStatus, FixStatBitsAndMapMatching)
{
    message value;
    EXPECT_FALSE(value.diff_corr());
    EXPECT_FALSE(value.carr_soln_valid());
    EXPECT_EQ(value.map_matching(), nav_status_map_matching::none);

    value.fix_stat = message::FIX_STAT_DIFF_CORR;
    EXPECT_TRUE(value.diff_corr());
    EXPECT_FALSE(value.carr_soln_valid());
    value.fix_stat = message::FIX_STAT_CARR_SOLN_VALID;
    EXPECT_TRUE(value.carr_soln_valid());
    EXPECT_FALSE(value.diff_corr());

    value.fix_stat = 0x40U;
    EXPECT_EQ(value.map_matching(), nav_status_map_matching::valid_not_used);
    value.fix_stat = static_cast<uint8_t>(0x80U | 0x3FU);
    EXPECT_EQ(value.map_matching(), nav_status_map_matching::valid_used);
    value.fix_stat = message::FIX_STAT_MAP_MATCHING_MASK;
    EXPECT_EQ(value.map_matching(), nav_status_map_matching::valid_used_dr);
    EXPECT_FALSE(value.diff_corr());
    value.fix_stat = 0x3FU;
    EXPECT_EQ(value.map_matching(), nav_status_map_matching::none);
}

TEST(UbxMessageNavStatus, Flags2Fields)
{
    message value;
    EXPECT_EQ(value.psm_state(), nav_status_psm_state::acquisition);
    EXPECT_EQ(value.spoof_det_state(), nav_status_spoof_det_state::unknown);
    EXPECT_EQ(value.carr_soln(), nav_status_carr_soln::none);

    value.flags2 = 0x01U;
    EXPECT_EQ(value.psm_state(), nav_status_psm_state::tracking);
    value.flags2 = 0x02U;
    EXPECT_EQ(value.psm_state(), nav_status_psm_state::power_optimized_tracking);
    value.flags2 = static_cast<uint8_t>(0xFCU | 0x03U);
    EXPECT_EQ(value.psm_state(), nav_status_psm_state::inactive);

    value.flags2 = 0x08U;
    EXPECT_EQ(value.spoof_det_state(), nav_status_spoof_det_state::no_spoofing);
    EXPECT_EQ(value.psm_state(), nav_status_psm_state::acquisition);
    value.flags2 = static_cast<uint8_t>(0x10U | 0xE7U);
    EXPECT_EQ(value.spoof_det_state(), nav_status_spoof_det_state::spoofing);
    value.flags2 = message::FLAGS2_SPOOF_DET_STATE_MASK;
    EXPECT_EQ(value.spoof_det_state(), nav_status_spoof_det_state::multiple);
    EXPECT_EQ(value.carr_soln(), nav_status_carr_soln::none);

    value.flags2 = 0x40U;
    EXPECT_EQ(value.carr_soln(), nav_status_carr_soln::floating);
    value.flags2 = static_cast<uint8_t>(0x80U | 0x3FU);
    EXPECT_EQ(value.carr_soln(), nav_status_carr_soln::fixed);
    EXPECT_EQ(value.spoof_det_state(), nav_status_spoof_det_state::multiple);
}

TEST(UbxMessageNavStatus, SampleAccessorsAgreeWithRawBytes)
{
    // flags 0x0F, fixStat 0xC3, flags2 0x9A (carrSoln 2, spoofDetState 3, psmState 2).
    CASTLE_CONST message value = make_message();
    EXPECT_TRUE(value.fix_ok());
    EXPECT_TRUE(value.diff_soln());
    EXPECT_TRUE(value.wkn_set());
    EXPECT_TRUE(value.tow_set());
    EXPECT_TRUE(value.diff_corr());
    EXPECT_TRUE(value.carr_soln_valid());
    EXPECT_EQ(value.map_matching(), nav_status_map_matching::valid_used_dr);
    EXPECT_EQ(value.psm_state(), nav_status_psm_state::power_optimized_tracking);
    EXPECT_EQ(value.spoof_det_state(), nav_status_spoof_det_state::multiple);
    EXPECT_EQ(value.carr_soln(), nav_status_carr_soln::fixed);
}

} // namespace

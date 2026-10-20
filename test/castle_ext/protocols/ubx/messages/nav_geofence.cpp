#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/nav_geofence.hpp"

namespace
{

using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

message_view view_of(CASTLE_CONST uint8_t* payload, uint16_t length, uint8_t msg_id = UBX_ID_NAV_GEOFENCE)
{
    return message_view{{UBX_CLASS_NAV, msg_id, length},
                        array_view<CASTLE_CONST uint8_t>(payload, length),
                        checksum{}};
}

TEST(UbxMessageNavGeofence, ConstantsMatchSpec)
{
    EXPECT_EQ(nav_geofence<>::msg_class, 0x01U);
    EXPECT_EQ(nav_geofence<>::msg_id, 0x39U);
    EXPECT_EQ(nav_geofence<>::header_length, 8U);
    EXPECT_EQ(nav_geofence<>::block_length, 2U);
    EXPECT_EQ(nav_geofence_fence::wire_size, 2U);
    EXPECT_EQ(static_cast<uint8_t>(nav_geofence_state::unknown), 0U);
    EXPECT_EQ(static_cast<uint8_t>(nav_geofence_state::inside), 1U);
    EXPECT_EQ(static_cast<uint8_t>(nav_geofence_state::outside), 2U);
}

TEST(UbxMessageNavGeofence, DecodesFieldsAtSpecOffsets)
{
    uint8_t payload[12U] = {};
    castle::write_le32(&payload[0U], 0x01020304U); // iTOW
    payload[4U] = 0x00U;                           // version
    payload[5U] = nav_geofence<>::STATUS_ACTIVE;   // status
    payload[6U] = 2U;                              // numFences
    payload[7U] = 2U;                              // combState = outside
    payload[8U] = 1U;                              // fence 0 state = inside
    payload[9U] = 7U;                              // fence 0 id
    payload[10U] = 2U;                             // fence 1 state = outside
    payload[11U] = 9U;                             // fence 1 id

    nav_geofence<> out{};
    ASSERT_TRUE(nav_geofence<>::decode(view_of(payload, 12U), out));
    EXPECT_EQ(out.i_tow, 0x01020304U);
    EXPECT_EQ(out.version, 0U);
    EXPECT_EQ(out.status, nav_geofence<>::STATUS_ACTIVE);
    EXPECT_EQ(out.num_fences, 2U);
    EXPECT_EQ(out.comb_state, nav_geofence_state::outside);
    ASSERT_EQ(out.fences.size(), 2U);
    EXPECT_EQ(out.fences[0U].state, nav_geofence_state::inside);
    EXPECT_EQ(out.fences[0U].id, 7U);
    EXPECT_EQ(out.fences[1U].state, nav_geofence_state::outside);
    EXPECT_EQ(out.fences[1U].id, 9U);
}

TEST(UbxMessageNavGeofence, RoundTripWritesCountFromStoredFences)
{
    nav_geofence<4U> in{};
    in.i_tow = 345600000U;
    in.version = 0U;
    in.status = nav_geofence<4U>::STATUS_ACTIVE;
    in.num_fences = 99U; // ignored on encode: the wire count comes from fences.size()
    in.comb_state = nav_geofence_state::inside;
    for (uint8_t index = 0U; index < 3U; ++index)
    {
        nav_geofence_fence fence{};
        fence.state = (index == 1U) ? nav_geofence_state::outside : nav_geofence_state::inside;
        fence.id = static_cast<uint8_t>(index + 10U);
        ASSERT_EQ(in.fences.push_back(fence), castle::status::ok);
    }

    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::ok);
    ASSERT_EQ(written, frame_size(8U + (3U * 2U)));
    EXPECT_EQ(frame[UBX_FRAME_INDEX_PAYLOAD + 6U], 3U); // numFences
    EXPECT_EQ(in.num_fences, 99U);                      // encode does not mutate the message

    message_view raw;
    ASSERT_TRUE(decode_frame(array_view<CASTLE_CONST uint8_t>(frame, written), raw));
    ASSERT_TRUE(nav_geofence<4U>::matches(raw));

    nav_geofence<4U> out{};
    ASSERT_TRUE(nav_geofence<4U>::decode(raw, out));
    EXPECT_EQ(out.i_tow, in.i_tow);
    EXPECT_EQ(out.status, in.status);
    EXPECT_EQ(out.num_fences, 3U);
    EXPECT_EQ(out.comb_state, nav_geofence_state::inside);
    ASSERT_EQ(out.fences.size(), 3U);
    EXPECT_EQ(out.fences[1U].state, nav_geofence_state::outside);
    EXPECT_EQ(out.fences[2U].id, 12U);
}

TEST(UbxMessageNavGeofence, ZeroFencesIsHeaderOnly)
{
    uint8_t payload[8U] = {};
    castle::write_le32(&payload[0U], 77U);

    nav_geofence<> out{};
    ASSERT_TRUE(nav_geofence<>::decode(view_of(payload, 8U), out));
    EXPECT_EQ(out.i_tow, 77U);
    EXPECT_EQ(out.num_fences, 0U);
    EXPECT_TRUE(out.fences.empty());
}

TEST(UbxMessageNavGeofence, FencesBeyondCapacityAreCountedButNotStored)
{
    uint8_t payload[14U] = {};
    payload[6U] = 3U;
    payload[8U] = 1U;
    payload[9U] = 1U;
    payload[10U] = 1U;
    payload[11U] = 2U;
    payload[12U] = 2U;
    payload[13U] = 3U;

    nav_geofence<2U> out{};
    ASSERT_TRUE(nav_geofence<2U>::decode(view_of(payload, 14U), out));
    EXPECT_EQ(out.num_fences, 3U);
    ASSERT_EQ(out.fences.size(), 2U);
    EXPECT_EQ(out.fences[1U].id, 2U);
}

TEST(UbxMessageNavGeofence, RejectsWrongIdentityAndLengthAndResetsOutput)
{
    uint8_t payload[12U] = {};
    castle::write_le32(&payload[0U], 5U);
    payload[6U] = 2U;

    nav_geofence<> out{};
    out.i_tow = 123U;
    EXPECT_FALSE(nav_geofence<>::matches(view_of(payload, 12U, UBX_ID_NAV_SAT)));
    EXPECT_FALSE(nav_geofence<>::decode(view_of(payload, 12U, UBX_ID_NAV_SAT), out));
    EXPECT_EQ(out.i_tow, 0U);

    message_view wrong_class = view_of(payload, 12U);
    wrong_class.header.msg_class = UBX_CLASS_NAV2;
    EXPECT_FALSE(nav_geofence<>::decode(wrong_class, out));

    // Shorter than the fixed header.
    EXPECT_FALSE(nav_geofence<>::decode(view_of(payload, 7U), out));

    // Declared 2 fences, but one block is missing or one is extra.
    out.i_tow = 123U;
    EXPECT_FALSE(nav_geofence<>::decode(view_of(payload, 10U), out));
    EXPECT_EQ(out.i_tow, 0U);
    EXPECT_EQ(out.num_fences, 0U);

    uint8_t longer[14U] = {};
    longer[6U] = 2U;
    EXPECT_FALSE(nav_geofence<>::decode(view_of(longer, 14U), out));
}

TEST(UbxMessageNavGeofence, EncodeIntoTooSmallBufferReportsFull)
{
    nav_geofence<> in{};
    ASSERT_EQ(in.fences.push_back(nav_geofence_fence{}), castle::status::ok);

    uint8_t frame[frame_size(10U) - 1U] = {};
    castle::size_type written = 0xDEADU;
    EXPECT_EQ(encode_message(in, frame, sizeof(frame), written), castle::status::full);
    EXPECT_EQ(written, 0U);
}

} // namespace

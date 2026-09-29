#include <gtest/gtest.h>

#include "castle_ext/protocols/ubx/messages/inf.hpp"

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;
using castle::container::string_view;

TEST(UbxMessageInfMirror, MatchesSubtypeRangeAndStopsTextAtNul)
{
    uint8_t payload[8U] = {'E','R','R','!','\0','X','X','X'};
    message_view raw{{UBX_CLASS_INF, UBX_ID_INF_WARNING, 8U}, array_view<CASTLE_CONST uint8_t>(payload, 8U), checksum{}};
    inf<8U> out;
    ASSERT_TRUE(inf<8U>::matches(raw));
    ASSERT_TRUE(inf<8U>::decode(raw, out));
    EXPECT_EQ(out.message_id, UBX_ID_INF_WARNING);
    EXPECT_EQ(out.subtype, inf_subtype::warning);
    EXPECT_EQ(out.text(), string_view("ERR!"));
}

TEST(UbxMessageInfMirror, RejectsOutOfRangeClassOrSubtype)
{
    uint8_t payload[1U] = {'A'};
    message_view raw{{UBX_CLASS_INF, UBX_ID_INF_ERROR, 1U}, array_view<CASTLE_CONST uint8_t>(payload, 1U), checksum{}};
    inf<> out;
    EXPECT_TRUE(inf<>::decode(raw, out));
    raw.header.msg_id = 0x05U;
    EXPECT_FALSE(inf<>::matches(raw));
    EXPECT_FALSE(inf<>::decode(raw, out));
    raw.header.msg_id = UBX_ID_INF_ERROR;
    raw.header.msg_class = UBX_CLASS_NAV;
    EXPECT_FALSE(inf<>::matches(raw));
}

} // namespace

namespace
{
using namespace castle::protocols::ubx;
using namespace castle::protocols::ubx::messages;
using castle::container::array_view;

TEST(UbxMessageInfMirror, ClampsTextToConfiguredCapacity)
{
    uint8_t payload[6U] = {'A', 'B', 'C', 'D', 'E', 'F'};
    message_view raw{{UBX_CLASS_INF, UBX_ID_INF_DEBUG, 6U},
                     array_view<CASTLE_CONST uint8_t>(payload, 6U), checksum{}};
    inf<4U> out;
    ASSERT_TRUE(inf<4U>::decode(raw, out));
    EXPECT_EQ(out.text_length, 4U);
    EXPECT_EQ(out.text_storage[3U], 'D');
}

} // namespace

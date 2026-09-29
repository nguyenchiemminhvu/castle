#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/gll.hpp"

namespace
{

using namespace castle::protocols::nmea;
using namespace castle::protocols::nmea::messages;
using castle::container::string_view;

TEST(NmeaMessageGllMirror, DecodesOptionalStatusAndMode)
{
    string_view storage[8U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPGLL,4916.45,N,12311.12,W,225444,A,a"), storage, 8U, raw));

    gll out;
    ASSERT_TRUE(gll::decode(raw, out));
    EXPECT_TRUE(gll::matches(raw));
    EXPECT_TRUE(out.status_active);
    EXPECT_EQ(out.mode, 'a');
    EXPECT_NEAR(out.latitude, 49.2741667, 1e-7);
    EXPECT_NEAR(out.longitude, -123.1853333, 1e-7);
}

TEST(NmeaMessageGllMirror, RejectsMissingCoordinateFields)
{
    string_view storage[8U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPGLL,4916.45,N,12311.12"), storage, 8U, raw));

    gll out;
    EXPECT_FALSE(gll::decode(raw, out));
}

} // namespace

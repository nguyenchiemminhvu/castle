#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/vtg.hpp"

namespace
{

using namespace castle_ext::protocols::nmea;
using namespace castle_ext::protocols::nmea::messages;
using castle::container::string_view;

TEST(NmeaMessageVtgMirror, DecodesMagneticCourseAndMode)
{
    string_view storage[12U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPVTG,84.4,T,3.1,M,22.4,N,41.5,K,A"), storage, 12U, raw));

    vtg out;
    ASSERT_TRUE(vtg::decode(raw, out));
    EXPECT_TRUE(vtg::matches(raw));
    EXPECT_DOUBLE_EQ(out.course_true, 84.4);
    EXPECT_DOUBLE_EQ(out.course_mag, 3.1);
    EXPECT_TRUE(out.course_mag_valid);
    EXPECT_DOUBLE_EQ(out.speed_kmh, 41.5);
    EXPECT_EQ(out.mode, 'A');
}

TEST(NmeaMessageVtgMirror, EmptyMagneticCourseIsAccepted)
{
    string_view storage[10U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPVTG,84.4,T,,M,22.4,N,41.5,K"), storage, 10U, raw));

    vtg out;
    ASSERT_TRUE(vtg::decode(raw, out));
    EXPECT_FALSE(out.course_mag_valid);
    EXPECT_DOUBLE_EQ(out.course_mag, 0.0);
}

} // namespace

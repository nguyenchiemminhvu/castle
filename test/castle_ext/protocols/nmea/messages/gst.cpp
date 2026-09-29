#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/gst.hpp"

namespace
{

using namespace castle::protocols::nmea;
using namespace castle::protocols::nmea::messages;
using castle::container::string_view;

TEST(NmeaMessageGstMirror, DecodesAllErrorStatistics)
{
    string_view storage[10U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(
        string_view("GPGST,092725.00,1.1,2.2,3.3,4.4,5.5,6.6,7.7"),
        storage, 10U, raw));

    gst out;
    ASSERT_TRUE(gst::decode(raw, out));
    EXPECT_EQ(out.talker, string_view("GP"));
    EXPECT_DOUBLE_EQ(out.alt_err, 7.7);
    EXPECT_TRUE(out.valid);
}

TEST(NmeaMessageGstMirror, RejectsShortSentence)
{
    string_view storage[7U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(string_view("GPGST,1,2,3,4,5,6"), storage, 7U, raw));

    gst out;
    EXPECT_FALSE(gst::decode(raw, out));
}

} // namespace

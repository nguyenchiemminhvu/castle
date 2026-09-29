#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/dtm.hpp"

namespace
{
using namespace castle_ext::protocols::nmea;
using namespace castle_ext::protocols::nmea::messages;
using castle::container::string_view;

TEST(NmeaMessageDtmMirror, DecodesSignedOffsetsAndReferenceDatum)
{
    string_view storage[10U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(string_view("$GPDTM,W84,SUB,4717.0,S,00833.0,W,12.5,REF\r\n"), storage, 10U, raw));
    dtm out;
    ASSERT_TRUE(dtm::decode(raw, out));
    EXPECT_TRUE(dtm::matches(raw));
    EXPECT_EQ(out.talker, string_view("GP"));
    EXPECT_EQ(out.datum_code, string_view("W84"));
    EXPECT_EQ(out.ref_datum, string_view("REF"));
    EXPECT_NEAR(out.lat_offset, -4717.0, 1e-12);
    EXPECT_NEAR(out.lon_offset, -833.0, 1e-12);
    EXPECT_DOUBLE_EQ(out.alt_offset, 12.5);
    EXPECT_TRUE(out.valid);
}

TEST(NmeaMessageDtmMirror, RejectsTooFewFields)
{
    string_view storage[4U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(string_view("GPDTM,A,B,1,N,2,E,3"), storage, 4U, raw));
    // The storage is intentionally undersized, so the raw view has fewer than seven fields.
    dtm out;
    EXPECT_FALSE(dtm::decode(raw, out));
}

} // namespace

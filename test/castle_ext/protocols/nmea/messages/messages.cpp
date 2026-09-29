#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/messages.hpp"

namespace
{

using namespace castle::protocols::nmea::messages;

TEST(NmeaMessagesMirror, AggregateHeaderExposesAllBuiltInSentenceTypes)
{
    EXPECT_GT(sizeof(dtm), 0U);
    EXPECT_GT(sizeof(gbs), 0U);
    EXPECT_GT(sizeof(gga), 0U);
    EXPECT_GT(sizeof(gll), 0U);
    EXPECT_GT(sizeof(gns), 0U);
    EXPECT_GT(sizeof(gsa), 0U);
    EXPECT_GT(sizeof(gst), 0U);
    EXPECT_GT(sizeof(gsv), 0U);
    EXPECT_GT(sizeof(rmc), 0U);
    EXPECT_GT(sizeof(vtg), 0U);
    EXPECT_GT(sizeof(zda), 0U);
}

} // namespace

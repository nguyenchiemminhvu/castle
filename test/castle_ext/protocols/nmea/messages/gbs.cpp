#include <gtest/gtest.h>

#include "castle_ext/protocols/nmea/messages/gbs.hpp"

namespace
{
using namespace castle::protocols::nmea;
using namespace castle::protocols::nmea::messages;
using castle::container::string_view;

TEST(NmeaMessageGbsMirror, DecodesMandatoryAndOptionalValues)
{
    string_view storage[12U];
    message_view raw;
    ASSERT_TRUE(decode_sentence(string_view("GPGBS,092725.00,1.1,2.2,3.3,22,0.5,-0.2,0.1"), storage, 12U, raw));
    gbs out;
    ASSERT_TRUE(gbs::decode(raw, out));
    EXPECT_TRUE(gbs::matches(raw));
    EXPECT_DOUBLE_EQ(out.utc_time, 92725.0);
    EXPECT_EQ(out.failed_sv_id, 22);
    EXPECT_DOUBLE_EQ(out.probability, 0.5);
    EXPECT_DOUBLE_EQ(out.bias, -0.2);
    EXPECT_DOUBLE_EQ(out.std_dev, 0.1);
    EXPECT_TRUE(out.valid);
}

TEST(NmeaMessageGbsMirror, RejectsMissingMandatoryFieldsAndPreservesDefaultsOnOptionalEmpties)
{
    string_view storage[8U];
    message_view raw;
    gbs out;
    ASSERT_TRUE(decode_sentence(string_view("GPGBS,1,2,3"), storage, 8U, raw));
    EXPECT_FALSE(gbs::decode(raw, out));

    ASSERT_TRUE(decode_sentence(string_view("GPGBS,1,2,3,4,,,,"), storage, 8U, raw));
    ASSERT_TRUE(gbs::decode(raw, out));
    EXPECT_EQ(out.failed_sv_id, -1);
    EXPECT_DOUBLE_EQ(out.probability, 0.0);
    EXPECT_DOUBLE_EQ(out.bias, 0.0);
    EXPECT_DOUBLE_EQ(out.std_dev, 0.0);
}

} // namespace

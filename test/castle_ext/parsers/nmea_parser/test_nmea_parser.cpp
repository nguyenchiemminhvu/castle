#include <gtest/gtest.h>

#include "castle_ext/parsers/nmea_parser/nmea_parser.hpp"

namespace
{
using namespace castle::parsers::nmea_parser;
using castle::container::string_view;

TEST(NmeaParserMirror, ParsesSentenceFromStringViewAndClearsCallback)
{
    nmea_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&](const nmea_parser<>::message_type& message)
                                 {
                                     ++hits;
                                     EXPECT_EQ(message.type, string_view("TXT"));
                                     EXPECT_EQ(message.field(0U), string_view("1"));
                                 });
    EXPECT_TRUE(parser.has_message_callback());
    parser.feed(string_view("$GPTXT,1\r\n"));
    EXPECT_EQ(hits, 1);

    parser.clear_message_callback();
    EXPECT_FALSE(parser.has_message_callback());
    parser.reset();
    EXPECT_EQ(parser.state(), parse_state::wait_dollar);
    EXPECT_EQ(parser.messages_decoded(), 0U);
}

TEST(NmeaParserMirror, ErrorMessagesAreStableAndNullZeroSizeIsAccepted)
{
    EXPECT_STREQ(error_message(parser_error_code::none), "no error");
    EXPECT_STREQ(error_message(parser_error_code::checksum_mismatch), "NMEA checksum mismatch");
    EXPECT_STREQ(error_message(parser_error_code::sentence_too_long), "NMEA sentence exceeds parser capacity");
    EXPECT_STREQ(error_message(parser_error_code::unexpected_start_in_sentence), "unexpected '$' before previous sentence terminated");
    EXPECT_STREQ(error_message(parser_error_code::invalid_argument), "invalid parser input");

    nmea_parser<> parser;
    int errors = 0;
    parser.set_error_callback([&](const parse_error&) { ++errors; });
    parser.feed(static_cast<const char*>(nullptr), 0U);
    parser.feed(static_cast<const uint8_t*>(nullptr), 0U);
    EXPECT_EQ(errors, 0);
}

} // namespace

#include <gtest/gtest.h>

#include "castle_ext/parsers/ubx_parser/ubx_parser.hpp"

namespace
{
using namespace castle::parsers::ubx_parser;
using namespace castle::protocols::ubx;

TEST(UbxParserMirror, ParsesFrameAndClearsCallback)
{
    ubx_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&](const ubx_parser<>::message_type& message)
                                 {
                                     ++hits;
                                     EXPECT_EQ(message.msg_class(), UBX_CLASS_ACK);
                                     EXPECT_EQ(message.msg_id(), UBX_ID_ACK_ACK);
                                     EXPECT_EQ(message.payload_length(), 2U);
                                 });

    uint8_t payload[2U] = {0x06U, 0x8BU};
    uint8_t frame[32U] = {};
    castle::size_type written = 0U;
    ASSERT_EQ(encode_frame(UBX_CLASS_ACK, UBX_ID_ACK_ACK,
                           castle::container::array_view<CASTLE_CONST uint8_t>(payload, 2U),
                           frame, sizeof(frame), written), castle::status::ok);
    parser.feed(frame, written);
    EXPECT_EQ(hits, 1);
    EXPECT_EQ(parser.frames_decoded(), 1U);

    parser.clear_message_callback();
    EXPECT_FALSE(parser.has_message_callback());
}

TEST(UbxParserMirror, NullPointerWithDataReportsInvalidArgument)
{
    ubx_parser<> parser;
    int errors = 0;
    parser.set_error_callback([&](const parse_error& error)
                               {
                                   ++errors;
                                   EXPECT_EQ(error.code, parser_error_code::invalid_argument);
                               });
    parser.feed(static_cast<const uint8_t*>(nullptr), 1U);
    EXPECT_EQ(errors, 1);
    EXPECT_EQ(parser.frames_discarded(), 1U);
}

TEST(UbxParserMirror, ErrorMessagesAndCapacityAreStable)
{
    EXPECT_STREQ(error_message(parser_error_code::none), "no error");
    EXPECT_STREQ(error_message(parser_error_code::invalid_sync), "invalid UBX synchronization sequence");
    EXPECT_STREQ(error_message(parser_error_code::payload_too_large), "UBX payload exceeds parser capacity");
    EXPECT_STREQ(error_message(parser_error_code::checksum_mismatch), "UBX checksum mismatch");
    EXPECT_STREQ(error_message(parser_error_code::invalid_argument), "invalid parser input");
    EXPECT_EQ(ubx_parser<16U>::payload_capacity(), 16U);
}

} // namespace

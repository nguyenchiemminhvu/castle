#include <gtest/gtest.h>

#include "castle_ext/parsers/nmea_parser/nmea_parser.hpp"

#include <string.h>

namespace
{

using namespace castle::protocols::nmea;
using castle::container::string_view;
using castle::parsers::nmea_parser::nmea_parser;
using castle::parsers::nmea_parser::parse_error;
using castle::parsers::nmea_parser::parse_state;
using castle::parsers::nmea_parser::parser_error_code;

castle::size_type build_sentence(
    char* output, castle::size_type capacity,
    CASTLE_CONST char* talker, CASTLE_CONST char* type,
    std::initializer_list<CASTLE_CONST char*> field_values)
{
    string_view field_storage[16U];
    castle::size_type count = 0U;
    for (CASTLE_CONST char* value : field_values)
    {
        field_storage[count] = string_view(value);
        ++count;
    }

    castle::container::array_view<CASTLE_CONST string_view> fields(field_storage, count);
    castle::size_type written = 0U;
    castle::status result = encode_sentence(
        string_view(talker), string_view(type), fields, output, capacity, written);
    EXPECT_EQ(result, castle::status::ok);
    return written;
}

// ---------------------------------------------------------------------------
// error_message
// ---------------------------------------------------------------------------

TEST(NmeaParser, ErrorMessageCoversAllCodes)
{
    EXPECT_STREQ(castle::parsers::nmea_parser::error_message(parser_error_code::none), "no error");
    EXPECT_STREQ(castle::parsers::nmea_parser::error_message(parser_error_code::checksum_mismatch),
                 "NMEA checksum mismatch");
    EXPECT_STREQ(castle::parsers::nmea_parser::error_message(parser_error_code::sentence_too_long),
                 "NMEA sentence exceeds parser capacity");
    EXPECT_STREQ(castle::parsers::nmea_parser::error_message(parser_error_code::unexpected_start_in_sentence),
                 "unexpected '$' before previous sentence terminated");
    EXPECT_STREQ(castle::parsers::nmea_parser::error_message(parser_error_code::invalid_argument),
                 "invalid parser input");
    EXPECT_STREQ(castle::parsers::nmea_parser::error_message(static_cast<parser_error_code>(0xFFU)),
                 "unknown NMEA parser error");
}

// ---------------------------------------------------------------------------
// Default state and callback registration
// ---------------------------------------------------------------------------

TEST(NmeaParser, DefaultStateIsIdle)
{
    nmea_parser<> parser;
    EXPECT_EQ(parser.state(), parse_state::wait_dollar);
    EXPECT_EQ(parser.messages_decoded(), 0U);
    EXPECT_EQ(parser.messages_discarded(), 0U);
    EXPECT_EQ(parser.buffered_length(), 0U);
    EXPECT_FALSE(parser.has_message_callback());
    EXPECT_FALSE(parser.has_error_callback());
    EXPECT_EQ(nmea_parser<>::max_sentence_length(), NMEA_SAFE_MAX_SENTENCE_LEN);
    EXPECT_EQ(nmea_parser<>::max_fields(), NMEA_DEFAULT_MAX_FIELDS);
}

TEST(NmeaParser, SetMessageCallbackViaMoveOverload)
{
    nmea_parser<> parser;
    int hits = 0;
    nmea_parser<>::message_callback_type callback(
        [&hits](CASTLE_CONST message_view&) { ++hits; });
    parser.set_message_callback(castle::move(callback));
    EXPECT_TRUE(parser.has_message_callback());

    char sentence[64U];
    castle::size_type written = build_sentence(sentence, sizeof(sentence), "GP", "GSA", {"1", "08"});
    parser.feed(sentence, written);
    EXPECT_EQ(hits, 1U);

    parser.clear_message_callback();
    EXPECT_FALSE(parser.has_message_callback());
}

TEST(NmeaParser, SetMessageCallbackViaTemplateOverload)
{
    nmea_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });
    EXPECT_TRUE(parser.has_message_callback());

    char sentence[64U];
    castle::size_type written = build_sentence(sentence, sizeof(sentence), "GP", "GSA", {"1", "08"});
    parser.feed(sentence, written);
    EXPECT_EQ(hits, 1U);
}

TEST(NmeaParser, SetErrorCallbackViaMoveOverload)
{
    nmea_parser<> parser;
    int hits = 0;
    nmea_parser<>::error_callback_type callback(
        [&hits](CASTLE_CONST parse_error& error)
        {
            ++hits;
            EXPECT_EQ(error.code, parser_error_code::checksum_mismatch);
        });
    parser.set_error_callback(castle::move(callback));
    EXPECT_TRUE(parser.has_error_callback());

    char sentence[64U];
    castle::size_type written = build_sentence(sentence, sizeof(sentence), "GP", "GGA", {"1", "2"});
    sentence[written - 4U] ^= 0x01U; // flip a checksum hex digit
    parser.feed(sentence, written);
    EXPECT_EQ(hits, 1U);

    parser.clear_error_callback();
    EXPECT_FALSE(parser.has_error_callback());
}

TEST(NmeaParser, SetErrorCallbackViaTemplateOverload)
{
    nmea_parser<> parser;
    int hits = 0;
    parser.set_error_callback([&hits](CASTLE_CONST parse_error&) { ++hits; });
    EXPECT_TRUE(parser.has_error_callback());

    char sentence[64U];
    castle::size_type written = build_sentence(sentence, sizeof(sentence), "GP", "GGA", {"1", "2"});
    sentence[written - 4U] ^= 0x01U;
    parser.feed(sentence, written);
    EXPECT_EQ(hits, 1U);
}

// ---------------------------------------------------------------------------
// feed() overloads
// ---------------------------------------------------------------------------

TEST(NmeaParser, FeedSingleCharDrivesStateMachine)
{
    nmea_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });

    char sentence[64U];
    castle::size_type written = build_sentence(sentence, sizeof(sentence), "GP", "RMC", {"1"});
    for (castle::size_type i = 0U; i < written; ++i)
    {
        parser.feed(sentence[i]);
    }
    EXPECT_EQ(hits, 1U);
    EXPECT_EQ(parser.messages_decoded(), 1U);
}

TEST(NmeaParser, FeedCharPointerRejectsNullWithNonZeroSize)
{
    nmea_parser<> parser;
    int errors = 0;
    parser.set_error_callback([&errors](CASTLE_CONST parse_error& error)
                               {
                                   ++errors;
                                   EXPECT_EQ(error.code, parser_error_code::invalid_argument);
                               });
    parser.feed(static_cast<CASTLE_CONST char*>(nullptr), 4U);
    EXPECT_EQ(errors, 1U);
}

TEST(NmeaParser, FeedCharPointerAcceptsNullWithZeroSize)
{
    nmea_parser<> parser;
    int errors = 0;
    parser.set_error_callback([&errors](CASTLE_CONST parse_error&) { ++errors; });
    parser.feed(static_cast<CASTLE_CONST char*>(nullptr), 0U);
    EXPECT_EQ(errors, 0U);
}

TEST(NmeaParser, FeedStringView)
{
    nmea_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });

    char sentence[64U];
    castle::size_type written = build_sentence(sentence, sizeof(sentence), "GP", "GLL", {"4916.45", "N"});
    parser.feed(string_view(sentence, written));
    EXPECT_EQ(hits, 1U);
}

TEST(NmeaParser, FeedUint8PointerRejectsNullWithNonZeroSize)
{
    nmea_parser<> parser;
    int errors = 0;
    parser.set_error_callback([&errors](CASTLE_CONST parse_error& error)
                               {
                                   ++errors;
                                   EXPECT_EQ(error.code, parser_error_code::invalid_argument);
                               });
    parser.feed(static_cast<CASTLE_CONST uint8_t*>(nullptr), 4U);
    EXPECT_EQ(errors, 1U);
}

TEST(NmeaParser, FeedUint8PointerAcceptsNullWithZeroSize)
{
    nmea_parser<> parser;
    int errors = 0;
    parser.set_error_callback([&errors](CASTLE_CONST parse_error&) { ++errors; });
    parser.feed(static_cast<CASTLE_CONST uint8_t*>(nullptr), 0U);
    EXPECT_EQ(errors, 0U);
}

TEST(NmeaParser, FeedUint8ArrayView)
{
    nmea_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });

    char sentence[64U];
    castle::size_type written = build_sentence(sentence, sizeof(sentence), "GP", "VTG", {"1", "2"});

    uint8_t bytes[64U];
    for (castle::size_type i = 0U; i < written; ++i)
    {
        bytes[i] = static_cast<uint8_t>(sentence[i]);
    }
    parser.feed(castle::container::array_view<CASTLE_CONST uint8_t>(bytes, written));
    EXPECT_EQ(hits, 1U);
}

// ---------------------------------------------------------------------------
// accumulate() branches
// ---------------------------------------------------------------------------

TEST(NmeaParser, UnexpectedDollarMidSentenceReportsErrorAndResumes)
{
    nmea_parser<> parser;
    int hits = 0;
    int errors = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });
    parser.set_error_callback([&errors](CASTLE_CONST parse_error& error)
                               {
                                   ++errors;
                                   EXPECT_EQ(error.code, parser_error_code::unexpected_start_in_sentence);
                                   EXPECT_TRUE(error.talker.empty());
                                   EXPECT_TRUE(error.type.empty());
                               });

    parser.feed("$GPGGA,trunc", strlen("$GPGGA,trunc"));
    char sentence[64U];
    castle::size_type written = build_sentence(sentence, sizeof(sentence), "GP", "GGA", {"1"});
    parser.feed(sentence, written);

    EXPECT_EQ(errors, 1U);
    EXPECT_EQ(hits, 1U);
}

TEST(NmeaParser, SentenceWithoutChecksumTerminatedByLfOnly)
{
    nmea_parser<> parser;
    int hits = 0;
    message_view captured;
    parser.set_message_callback([&hits, &captured](CASTLE_CONST message_view& sentence)
                                 {
                                     ++hits;
                                     captured = sentence;
                                 });

    CASTLE_CONST char* text = "$GPTXT,1,1,01,OK\n";
    parser.feed(text, strlen(text));
    ASSERT_EQ(hits, 1U);
    EXPECT_FALSE(captured.checksum_present);
}

TEST(NmeaParser, SentenceTooLongIsDiscarded)
{
    nmea_parser<8U> parser;
    int hits = 0;
    int errors = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });
    parser.set_error_callback([&errors](CASTLE_CONST parse_error& error)
                               {
                                   ++errors;
                                   EXPECT_EQ(error.code, parser_error_code::sentence_too_long);
                               });

    char sentence[64U];
    castle::size_type written =
        build_sentence(sentence, sizeof(sentence), "GP", "GGA", {"1", "2", "3", "4", "5", "6"});
    parser.feed(sentence, written);

    EXPECT_EQ(hits, 0U);
    EXPECT_EQ(errors, 1U);
    EXPECT_EQ(parser.messages_discarded(), 1U);
}

// ---------------------------------------------------------------------------
// checksum branches
// ---------------------------------------------------------------------------

TEST(NmeaParser, InvalidChecksumHexDigitsAreDiscarded)
{
    nmea_parser<> parser;
    int errors = 0;
    parser.set_error_callback([&errors](CASTLE_CONST parse_error& error)
                               {
                                   ++errors;
                                   EXPECT_EQ(error.code, parser_error_code::checksum_mismatch);
                                   EXPECT_TRUE(error.talker.empty());
                                   EXPECT_TRUE(error.type.empty());
                               });

    CASTLE_CONST char* text = "$GPGSA,1*ZZ\r\n";
    parser.feed(text, strlen(text));
    EXPECT_EQ(errors, 1U);
    EXPECT_EQ(parser.messages_discarded(), 1U);
}

TEST(NmeaParser, ChecksumMismatchReportsTalkerAndType)
{
    nmea_parser<> parser;
    int errors = 0;
    parser.set_error_callback([&errors](CASTLE_CONST parse_error& error)
                               {
                                   ++errors;
                                   EXPECT_EQ(error.code, parser_error_code::checksum_mismatch);
                                   EXPECT_EQ(error.talker, string_view("GP"));
                                   EXPECT_EQ(error.type, string_view("GGA"));
                               });

    char sentence[64U];
    castle::size_type written = build_sentence(sentence, sizeof(sentence), "GP", "GGA", {"1", "2"});
    sentence[written - 4U] ^= 0x01U; // flip a checksum hex digit so the value mismatches
    parser.feed(sentence, written);
    EXPECT_EQ(errors, 1U);
}

TEST(NmeaParser, WaitLfAbsorbsCrAndIgnoresStrayBytes)
{
    nmea_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });

    parser.feed('$');
    parser.feed('G'); parser.feed('P'); parser.feed('G'); parser.feed('S'); parser.feed('A');
    parser.feed(','); parser.feed('1');
    parser.feed('*'); parser.feed('5'); parser.feed('6'); // arbitrary checksum, mismatch expected
    // At this point state is wait_lf regardless of checksum validity (set after wait_checksum_lo).
    parser.feed('X'); // stray byte while waiting for LF, must be ignored without completing/erroring
    EXPECT_EQ(parser.state(), parse_state::wait_lf);
    parser.feed('\r');
    parser.feed('\n');
    // Either a checksum error or a delivered message is acceptable here; the
    // key behavior under test is that the stray 'X' byte did not crash or
    // desynchronize the parser from reaching a terminal outcome exactly once.
    EXPECT_EQ(parser.messages_decoded() + parser.messages_discarded(), 1U);
}

TEST(NmeaParser, UnexpectedDollarWhileWaitingForLf)
{
    nmea_parser<> parser;
    int hits = 0;
    int errors = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });
    parser.set_error_callback([&errors](CASTLE_CONST parse_error& error)
                               {
                                   ++errors;
                                   EXPECT_EQ(error.code, parser_error_code::unexpected_start_in_sentence);
                               });

    char first[64U];
    castle::size_type first_written = build_sentence(first, sizeof(first), "GP", "GGA", {"1"});
    // Feed everything except the final CRLF so the parser is left in wait_lf.
    parser.feed(first, first_written - 2U);
    EXPECT_EQ(parser.state(), parse_state::wait_lf);

    char second[64U];
    castle::size_type second_written = build_sentence(second, sizeof(second), "GP", "RMC", {"2"});
    parser.feed(second, second_written);

    EXPECT_EQ(errors, 1U);
    EXPECT_EQ(hits, 1U);
}

// ---------------------------------------------------------------------------
// Field-table capacity plumbing
// ---------------------------------------------------------------------------

TEST(NmeaParser, FieldCountIsTruncatedToMaxFields)
{
    nmea_parser<64U, 2U> parser;
    message_view captured;
    int hits = 0;
    parser.set_message_callback([&hits, &captured](CASTLE_CONST message_view& sentence)
                                 {
                                     ++hits;
                                     captured = sentence;
                                 });

    char sentence[64U];
    castle::size_type written =
        build_sentence(sentence, sizeof(sentence), "GP", "GGA", {"1", "2", "3", "4"});
    parser.feed(sentence, written);

    ASSERT_EQ(hits, 1U);
    EXPECT_EQ(captured.field_count(), 2U);
}

// ---------------------------------------------------------------------------
// reset()
// ---------------------------------------------------------------------------

TEST(NmeaParser, ResetClearsStateButKeepsCallbacks)
{
    nmea_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });

    char sentence[64U];
    castle::size_type written = build_sentence(sentence, sizeof(sentence), "GP", "GLL", {"4916.45", "N"});
    parser.feed(sentence, written);
    EXPECT_EQ(parser.messages_decoded(), 1U);

    parser.reset();
    EXPECT_EQ(parser.messages_decoded(), 0U);
    EXPECT_EQ(parser.messages_discarded(), 0U);
    EXPECT_EQ(parser.state(), parse_state::wait_dollar);
    EXPECT_TRUE(parser.has_message_callback());

    parser.feed(sentence, written);
    EXPECT_EQ(hits, 2U);
    EXPECT_EQ(parser.messages_decoded(), 1U);
}

TEST(NmeaParser, BufferedLengthReflectsPartialAccumulation)
{
    nmea_parser<> parser;
    CASTLE_CONST char* partial = "$GPGGA,1,2";
    parser.feed(partial, strlen(partial));
    // "GPGGA,1,2" (9 chars) is buffered; the leading '$' is not stored.
    EXPECT_EQ(parser.buffered_length(), strlen(partial) - 1U);
}

// ---------------------------------------------------------------------------
// Callbacks not installed still allow parsing without crashing
// ---------------------------------------------------------------------------

TEST(NmeaParser, FeedWithoutAnyCallbacksStillUpdatesCounters)
{
    nmea_parser<8U> parser;

    char good_sentence[16U];
    castle::size_type good_written = build_sentence(good_sentence, sizeof(good_sentence), "", "A", {});
    parser.feed(good_sentence, good_written);
    EXPECT_EQ(parser.messages_decoded(), 1U);

    char long_sentence[64U];
    castle::size_type long_written =
        build_sentence(long_sentence, sizeof(long_sentence), "GP", "GGA", {"1", "2", "3", "4", "5", "6"});
    parser.feed(long_sentence, long_written);
    EXPECT_EQ(parser.messages_discarded(), 1U);
}

} // namespace

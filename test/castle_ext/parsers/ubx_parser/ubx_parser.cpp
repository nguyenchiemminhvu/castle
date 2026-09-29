#include <gtest/gtest.h>

#include "castle_ext/parsers/ubx_parser/ubx_parser.hpp"

namespace
{

using namespace castle::protocols::ubx;
using castle::parsers::ubx_parser::parse_error;
using castle::parsers::ubx_parser::parse_state;
using castle::parsers::ubx_parser::parser_error_code;
using castle::parsers::ubx_parser::ubx_parser;

castle::size_type make_frame(
    uint8_t* output, castle::size_type capacity,
    uint8_t msg_class, uint8_t msg_id,
    CASTLE_CONST uint8_t* payload_data, castle::size_type payload_size)
{
    castle::container::array_view<CASTLE_CONST uint8_t> payload(payload_data, payload_size);
    castle::size_type written = 0U;
    castle::status result = encode_frame(msg_class, msg_id, payload, output, capacity, written);
    EXPECT_EQ(result, castle::status::ok);
    return written;
}

// ---------------------------------------------------------------------------
// error_message
// ---------------------------------------------------------------------------

TEST(UbxParser, ErrorMessageCoversAllCodes)
{
    EXPECT_STREQ(castle::parsers::ubx_parser::error_message(parser_error_code::none), "no error");
    EXPECT_STREQ(castle::parsers::ubx_parser::error_message(parser_error_code::invalid_sync),
                 "invalid UBX synchronization sequence");
    EXPECT_STREQ(castle::parsers::ubx_parser::error_message(parser_error_code::payload_too_large),
                 "UBX payload exceeds parser capacity");
    EXPECT_STREQ(castle::parsers::ubx_parser::error_message(parser_error_code::checksum_mismatch),
                 "UBX checksum mismatch");
    EXPECT_STREQ(castle::parsers::ubx_parser::error_message(parser_error_code::invalid_argument),
                 "invalid parser input");
    EXPECT_STREQ(castle::parsers::ubx_parser::error_message(static_cast<parser_error_code>(0xFFU)),
                 "unknown UBX parser error");
}

// ---------------------------------------------------------------------------
// Default state and callback registration
// ---------------------------------------------------------------------------

TEST(UbxParser, DefaultStateIsIdle)
{
    ubx_parser<> parser;
    EXPECT_EQ(parser.state(), parse_state::wait_sync_1);
    EXPECT_EQ(parser.frames_decoded(), 0U);
    EXPECT_EQ(parser.frames_discarded(), 0U);
    EXPECT_EQ(parser.payload_size(), 0U);
    EXPECT_FALSE(parser.has_message_callback());
    EXPECT_FALSE(parser.has_error_callback());
    EXPECT_EQ(ubx_parser<>::payload_capacity(), UBX_SAFE_MAX_PAYLOAD_LEN);
}

TEST(UbxParser, SetMessageCallbackViaMoveOverload)
{
    ubx_parser<> parser;
    int hits = 0;
    ubx_parser<>::message_callback_type callback(
        [&hits](CASTLE_CONST message_view&) { ++hits; });
    parser.set_message_callback(castle::move(callback));
    EXPECT_TRUE(parser.has_message_callback());

    uint8_t payload_data[1U] = {0x01U};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_MON, UBX_ID_MON_VER, payload_data, 1U);
    parser.feed(frame, written);
    EXPECT_EQ(hits, 1U);

    parser.clear_message_callback();
    EXPECT_FALSE(parser.has_message_callback());
}

TEST(UbxParser, SetMessageCallbackViaTemplateOverload)
{
    ubx_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });
    EXPECT_TRUE(parser.has_message_callback());

    uint8_t payload_data[1U] = {0x01U};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_MON, UBX_ID_MON_VER, payload_data, 1U);
    parser.feed(frame, written);
    EXPECT_EQ(hits, 1U);
}

TEST(UbxParser, SetErrorCallbackViaMoveOverload)
{
    ubx_parser<4U> parser;
    int hits = 0;
    ubx_parser<4U>::error_callback_type callback(
        [&hits](CASTLE_CONST parse_error& error)
        {
            ++hits;
            EXPECT_EQ(error.code, parser_error_code::payload_too_large);
        });
    parser.set_error_callback(castle::move(callback));
    EXPECT_TRUE(parser.has_error_callback());

    uint8_t payload_data[8U] = {0U};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_NAV, UBX_ID_NAV_PVT, payload_data, 8U);
    parser.feed(frame, written);
    EXPECT_EQ(hits, 1U);
    EXPECT_EQ(parser.frames_discarded(), 1U);

    parser.clear_error_callback();
    EXPECT_FALSE(parser.has_error_callback());
}

TEST(UbxParser, SetErrorCallbackViaTemplateOverload)
{
    ubx_parser<4U> parser;
    int hits = 0;
    parser.set_error_callback([&hits](CASTLE_CONST parse_error&) { ++hits; });
    EXPECT_TRUE(parser.has_error_callback());

    uint8_t payload_data[8U] = {0U};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_NAV, UBX_ID_NAV_PVT, payload_data, 8U);
    parser.feed(frame, written);
    EXPECT_EQ(hits, 1U);
}

// ---------------------------------------------------------------------------
// feed() overloads
// ---------------------------------------------------------------------------

TEST(UbxParser, FeedSingleByteDrivesStateMachine)
{
    ubx_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });

    uint8_t payload_data[2U] = {0xAAU, 0xBBU};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_CFG, UBX_ID_CFG_VALSET, payload_data, 2U);

    for (castle::size_type i = 0U; i < written; ++i)
    {
        parser.feed(frame[i]);
    }
    EXPECT_EQ(hits, 1U);
    EXPECT_EQ(parser.frames_decoded(), 1U);
}

TEST(UbxParser, FeedPointerRejectsNullWithNonZeroSize)
{
    ubx_parser<> parser;
    int errors = 0;
    parser.set_error_callback([&errors](CASTLE_CONST parse_error& error)
                               {
                                   ++errors;
                                   EXPECT_EQ(error.code, parser_error_code::invalid_argument);
                               });
    parser.feed(static_cast<CASTLE_CONST uint8_t*>(nullptr), 4U);
    EXPECT_EQ(errors, 1U);
}

TEST(UbxParser, FeedPointerAcceptsNullWithZeroSize)
{
    ubx_parser<> parser;
    int errors = 0;
    parser.set_error_callback([&errors](CASTLE_CONST parse_error&) { ++errors; });
    parser.feed(static_cast<CASTLE_CONST uint8_t*>(nullptr), 0U);
    EXPECT_EQ(errors, 0U);
}

TEST(UbxParser, FeedArrayView)
{
    ubx_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });

    uint8_t payload_data[1U] = {0x7EU};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_TIM, UBX_ID_TIM_TP, payload_data, 1U);

    parser.feed(castle::container::array_view<CASTLE_CONST uint8_t>(frame, written));
    EXPECT_EQ(hits, 1U);
}

// ---------------------------------------------------------------------------
// Sync state machine branches
// ---------------------------------------------------------------------------

TEST(UbxParser, RepeatedSyncByteBeforeSecondSyncIsTolerated)
{
    ubx_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });

    uint8_t payload_data[1U] = {0x01U};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_MON, UBX_ID_MON_VER, payload_data, 1U);

    parser.feed(UBX_SYNC_CHAR_1);
    parser.feed(UBX_SYNC_CHAR_1); // repeated sync-1 while waiting for sync-2
    parser.feed(UBX_SYNC_CHAR_2);
    // Re-feed the remaining class..checksum bytes (sync bytes already consumed).
    parser.feed(&frame[2U], written - 2U);

    EXPECT_EQ(hits, 1U);
}

TEST(UbxParser, InvalidSecondSyncByteResetsToWaitSync1)
{
    ubx_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });

    parser.feed(UBX_SYNC_CHAR_1);
    parser.feed(0x00U); // neither SYNC_2 nor SYNC_1 -> back to wait_sync_1
    EXPECT_EQ(parser.state(), parse_state::wait_sync_1);

    uint8_t payload_data[1U] = {0x01U};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_MON, UBX_ID_MON_VER, payload_data, 1U);
    parser.feed(frame, written);
    EXPECT_EQ(hits, 1U);
}

// ---------------------------------------------------------------------------
// Payload length and checksum branches
// ---------------------------------------------------------------------------

TEST(UbxParser, ZeroLengthPayloadFrameIsDelivered)
{
    ubx_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view& message)
                                 {
                                     ++hits;
                                     EXPECT_EQ(message.payload_length(), 0U);
                                 });

    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_ACK, UBX_ID_ACK_ACK, nullptr, 0U);
    parser.feed(frame, written);
    EXPECT_EQ(hits, 1U);
}

TEST(UbxParser, OversizedPayloadIsDiscardedWithoutCallback)
{
    ubx_parser<4U> parser;
    int hits = 0;
    int errors = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });
    parser.set_error_callback([&errors](CASTLE_CONST parse_error& error)
                               {
                                   ++errors;
                                   EXPECT_EQ(error.code, parser_error_code::payload_too_large);
                               });

    uint8_t payload_data[8U] = {0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_NAV, UBX_ID_NAV_PVT, payload_data, 8U);
    parser.feed(frame, written);

    EXPECT_EQ(hits, 0U);
    EXPECT_EQ(errors, 1U);
    EXPECT_EQ(parser.frames_discarded(), 1U);
}

TEST(UbxParser, ChecksumMismatchIsDiscarded)
{
    ubx_parser<> parser;
    int hits = 0;
    int errors = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });
    parser.set_error_callback([&errors](CASTLE_CONST parse_error& error)
                               {
                                   ++errors;
                                   EXPECT_EQ(error.code, parser_error_code::checksum_mismatch);
                               });

    uint8_t payload_data[1U] = {0x55U};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_MON, UBX_ID_MON_VER, payload_data, 1U);
    frame[written - 1U] ^= 0xFFU;

    parser.feed(frame, written);
    EXPECT_EQ(hits, 0U);
    EXPECT_EQ(errors, 1U);
    EXPECT_EQ(parser.frames_discarded(), 1U);
}

TEST(UbxParser, MultiBytePayloadIsAccumulatedCorrectly)
{
    ubx_parser<> parser;
    message_view captured;
    int hits = 0;
    parser.set_message_callback([&hits, &captured](CASTLE_CONST message_view& message)
                                 {
                                     ++hits;
                                     captured = message;
                                 });

    uint8_t payload_data[4U] = {0x01U, 0x02U, 0x03U, 0x04U};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload_data, 4U);
    parser.feed(frame, written);

    ASSERT_EQ(hits, 1U);
    EXPECT_EQ(captured.payload_length(), 4U);
    EXPECT_EQ(captured.payload[3U], 0x04U);
}

// ---------------------------------------------------------------------------
// Callbacks not installed still allow parsing without crashing
// ---------------------------------------------------------------------------

TEST(UbxParser, FeedWithoutAnyCallbacksStillUpdatesCounters)
{
    ubx_parser<4U> parser_without_callbacks;

    uint8_t good_payload[1U] = {0x01U};
    uint8_t good_frame[32U];
    castle::size_type good_written =
        make_frame(good_frame, sizeof(good_frame), UBX_CLASS_MON, UBX_ID_MON_VER, good_payload, 1U);
    parser_without_callbacks.feed(good_frame, good_written);
    EXPECT_EQ(parser_without_callbacks.frames_decoded(), 1U);

    uint8_t big_payload[8U] = {0U};
    uint8_t big_frame[32U];
    castle::size_type big_written =
        make_frame(big_frame, sizeof(big_frame), UBX_CLASS_NAV, UBX_ID_NAV_PVT, big_payload, 8U);
    parser_without_callbacks.feed(big_frame, big_written);
    EXPECT_EQ(parser_without_callbacks.frames_discarded(), 1U);
}

// ---------------------------------------------------------------------------
// reset()
// ---------------------------------------------------------------------------

TEST(UbxParser, ResetClearsStateButKeepsCallbacks)
{
    ubx_parser<> parser;
    int hits = 0;
    parser.set_message_callback([&hits](CASTLE_CONST message_view&) { ++hits; });

    uint8_t payload_data[1U] = {0x01U};
    uint8_t frame[32U];
    castle::size_type written = make_frame(frame, sizeof(frame), UBX_CLASS_MON, UBX_ID_MON_VER, payload_data, 1U);
    parser.feed(frame, written);
    EXPECT_EQ(parser.frames_decoded(), 1U);

    parser.reset();
    EXPECT_EQ(parser.frames_decoded(), 0U);
    EXPECT_EQ(parser.frames_discarded(), 0U);
    EXPECT_EQ(parser.state(), parse_state::wait_sync_1);
    EXPECT_TRUE(parser.has_message_callback());

    parser.feed(frame, written);
    EXPECT_EQ(hits, 2U);
    EXPECT_EQ(parser.frames_decoded(), 1U);
}

TEST(UbxParser, PayloadSizeReflectsPartialAccumulation)
{
    ubx_parser<> parser;

    uint8_t payload_data[4U] = {0x01U, 0x02U, 0x03U, 0x04U};
    uint8_t frame[32U];
    make_frame(frame, sizeof(frame), UBX_CLASS_ESF, UBX_ID_ESF_MEAS, payload_data, 4U);

    // Feed up to and including the 2nd payload byte (6 header bytes + 2 payload bytes).
    parser.feed(frame, 8U);
    EXPECT_EQ(parser.payload_size(), 2U);
}

} // namespace

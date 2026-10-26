#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include "castle_ext/protocols/http/http2.hpp"

using namespace castle::protocols::http;
using castle::size_type;

namespace
{
size_type make_frame(uint8_t* output, uint8_t type, uint8_t flags, uint32_t stream_id,
                     uint8_t const* payload, size_type payload_size)
{
    output[0U] = static_cast<uint8_t>(payload_size >> 16U);
    output[1U] = static_cast<uint8_t>(payload_size >> 8U);
    output[2U] = static_cast<uint8_t>(payload_size);
    output[3U] = type;
    output[4U] = flags;
    detail::write_u32(output + 5U, stream_id);
    for (size_type i = 0U; i < payload_size; ++i) { output[9U + i] = payload[i]; }
    return 9U + payload_size;
}

template <typename Session>
uint32_t open_get(Session& session)
{
    typename Session::headers_type headers;
    uint32_t stream_id = 0U;
    EXPECT_EQ(session.open_stream(method::get, "/resource", "peer.test:80", headers,
                                  string_view(), stream_id), castle::status::ok);
    return stream_id;
}
}

TEST(Http2FrameParser, ParsesFramesAcrossArbitraryInputBoundaries)
{
    http2_frame_parser<> parser;
    const uint8_t payload[] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    uint8_t frame[17U]{};
    size_type frame_size = make_frame(frame, 6U, 0U, 0U, payload, sizeof(payload));
    size_type calls = 0U;
    EXPECT_EQ(parser.feed(frame, 4U, [&calls](http2_frame_view const&) { ++calls; return castle::status::ok; }), castle::status::ok);
    EXPECT_EQ(parser.feed(frame + 4U, frame_size - 4U,
        [&calls](http2_frame_view const& value)
        {
            ++calls;
            EXPECT_EQ(value.type, http2_frame_type::ping);
            EXPECT_EQ(value.length, 8U);
            EXPECT_EQ(value.payload[7U], 8U);
            return castle::status::ok;
        }), castle::status::ok);
    EXPECT_EQ(calls, 1U);
}

TEST(Http2FrameParser, IgnoresReservedStreamIdBit)
{
    http2_frame_parser<> parser;
    uint8_t ping[17U]{};
    const uint8_t opaque[] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    size_type size = make_frame(ping, 6U, 0U, 0U, opaque, sizeof(opaque));
    ping[5U] = 0x80U;
    uint32_t parsed_stream_id = 99U;
    EXPECT_EQ(parser.feed(ping, size, [&parsed_stream_id](http2_frame_view const& frame)
    {
        parsed_stream_id = frame.stream_id;
        return castle::status::ok;
    }), castle::status::ok);
    EXPECT_EQ(parsed_stream_id, 0U);
}

TEST(Http2FrameParser, RejectsOversizedAndStructurallyInvalidFrames)
{
    http2_frame_parser<> parser;
    uint8_t oversized[9U] = {0x00U, 0x40U, 0x01U, 0x06U, 0U, 0U, 0U, 0U, 0U}; // 16,385 bytes
    EXPECT_EQ(parser.feed(oversized, sizeof(oversized), [](http2_frame_view const&) { return castle::status::ok; }), castle::status::out_of_range);
    EXPECT_EQ(parser.last_error(), error_code::frame_size_limit);

    parser.reset();
    uint8_t bad_ping[9U] = {0U, 0U, 7U, 0x06U, 0U, 0U, 0U, 0U, 0U};
    EXPECT_EQ(parser.feed(bad_ping, sizeof(bad_ping), [](http2_frame_view const&) { return castle::status::ok; }), castle::status::invalid_argument);
    EXPECT_EQ(parser.last_error(), error_code::invalid_frame);
}

TEST(Http2Session, SendsMandatoryPrefaceAndStaticTableSetting)
{
    http2_session<2U, 256U, 16384U, 128U, 8U, 32U, 64U, 0U> session;
    EXPECT_EQ(session.start(), castle::status::ok);
    const char preface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    ASSERT_GE(session.output_size(), sizeof(preface) - 1U + 27U);
    for (size_type i = 0U; i < sizeof(preface) - 1U; ++i) { EXPECT_EQ(session.pending_output()[i], static_cast<uint8_t>(preface[i])); }
    const uint8_t* settings = session.pending_output() + (sizeof(preface) - 1U);
    EXPECT_EQ(settings[3U], static_cast<uint8_t>(http2_frame_type::settings));
    EXPECT_EQ(settings[2U], 18U);
    EXPECT_EQ(settings[9U], 0U); EXPECT_EQ(settings[10U], 1U); // HEADER_TABLE_SIZE
    EXPECT_EQ(settings[15U], 0U); EXPECT_EQ(settings[16U], 2U); // ENABLE_PUSH
    EXPECT_EQ(settings[21U], 0U); EXPECT_EQ(settings[22U], 5U); // MAX_FRAME_SIZE
    EXPECT_EQ(detail::read_u32(settings + 23U), 16384U);
}

TEST(Http2Session, MultiplexesOddClientStreamsAndEnforcesCompileTimeLimit)
{
    http2_session<2U, 512U, 16384U, 128U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint32_t first = open_get(session);
    uint32_t second = open_get(session);
    EXPECT_EQ(first, 1U);
    EXPECT_EQ(second, 3U);
    EXPECT_EQ(session.active_stream_count(), 2U);
    typename decltype(session)::headers_type headers;
    uint32_t third = 0U;
    EXPECT_EQ(session.open_stream(method::get, "/", "peer.test", headers, string_view(), third), castle::status::out_of_range);
    EXPECT_EQ(session.last_error(), error_code::stream_limit);

    http2_session<1U, 512U, 16384U, 128U> te_session;
    ASSERT_EQ(te_session.start(), castle::status::ok);
    typename decltype(te_session)::headers_type invalid_te;
    ASSERT_EQ(invalid_te.add("te", "gzip"), castle::status::ok);
    uint32_t invalid_stream = 0U;
    EXPECT_EQ(te_session.open_stream(method::get, "/", "peer.test", invalid_te, string_view(), invalid_stream), castle::status::invalid_argument);
}

TEST(Http2Session, DecodesResponseHeadersAndPaddedData)
{
    http2_session<2U, 256U, 16384U, 16U, 8U, 32U, 64U, 0U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint32_t stream = open_get(session);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    const uint8_t status_200[] = {0x88U};
    uint8_t input[32U]{};
    size_type size = make_frame(input, 1U, 0x04U, stream, status_200, sizeof(status_200));
    EXPECT_EQ(session.feed(input, size), castle::status::ok);
    const uint8_t padded_data[] = {1U, 'o', 'k', 0U};
    size = make_frame(input, 0U, 0x09U, stream, padded_data, sizeof(padded_data));
    EXPECT_EQ(session.feed(input, size), castle::status::ok);
    response_view response = session.response(stream);
    EXPECT_EQ(response.status_code, 200U);
    EXPECT_EQ(response.protocol, version::http_2);
    EXPECT_EQ(response.body.size(), 2U);
    EXPECT_EQ(response.body[0U], 'o');
    EXPECT_EQ(response.body[1U], 'k');
    EXPECT_EQ(session.active_stream_count(), 0U);
    EXPECT_EQ(session.release_stream(stream), castle::status::ok);
}

TEST(Http2Session, StreamsBodyPastRetentionBufferThroughCallback)
{
    http2_session<1U, 256U, 16384U, 2U, 8U, 32U, 64U, 0U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint32_t stream = open_get(session);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    size_type streamed = 0U;
    EXPECT_EQ(session.set_body_callback([&streamed](uint32_t, string_view chunk) { streamed += chunk.size(); }), castle::status::ok);
    const uint8_t status_200[] = {0x88U};
    uint8_t input[32U]{};
    size_type size = make_frame(input, 1U, 0x04U, stream, status_200, sizeof(status_200));
    ASSERT_EQ(session.feed(input, size), castle::status::ok);
    const uint8_t body[] = {'a', 'b', 'c', 'd'};
    size = make_frame(input, 0U, 0x01U, stream, body, sizeof(body));
    EXPECT_EQ(session.feed(input, size), castle::status::ok);
    EXPECT_TRUE(session.response(stream).body_streamed);
    EXPECT_EQ(session.response(stream).body.size(), 2U);
    EXPECT_EQ(streamed, 4U);
}

TEST(Http2Session, AcceptsContentLengthAndTrailerHeaders)
{
    http2_session<1U, 256U, 16384U, 32U, 8U, 32U, 64U, 0U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint32_t stream = open_get(session);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);

    // :status = 200, content-length = "2" (literal without indexing, static name index 28).
    const uint8_t initial_headers[] = {0x88U, 0x0FU, 0x0DU, 0x01U, '2'};
    uint8_t frame[64U]{};
    size_type size = make_frame(frame, 1U, 0x04U, stream, initial_headers, sizeof(initial_headers));
    ASSERT_EQ(session.feed(frame, size), castle::status::ok);

    const uint8_t body[] = {'o', 'k'};
    size = make_frame(frame, 0U, 0U, stream, body, sizeof(body));
    ASSERT_EQ(session.feed(frame, size), castle::status::ok);

    // Trailer x-check: yes, literal name and value, with END_HEADERS | END_STREAM.
    const uint8_t trailers[] = {0x00U, 0x07U, 'x', '-', 'c', 'h', 'e', 'c', 'k', 0x03U, 'y', 'e', 's'};
    size = make_frame(frame, 1U, 0x05U, stream, trailers, sizeof(trailers));
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);

    response_view response = session.response(stream);
    EXPECT_EQ(response.status_code, 200U);
    EXPECT_EQ(response.body.size(), 2U);
    EXPECT_EQ(response.header_count, 2U);
    EXPECT_TRUE(ascii_iequal(response.headers[1U].name, "x-check"));
    EXPECT_TRUE(ascii_iequal(response.headers[1U].value, "yes"));
}

TEST(Http2Session, RejectsContentLengthMismatchAndInformationalEndStream)
{
    http2_session<1U, 256U, 16384U, 32U> mismatch;
    ASSERT_EQ(mismatch.start(), castle::status::ok);
    uint32_t stream = open_get(mismatch);
    const uint8_t length_two[] = {0x88U, 0x0FU, 0x0DU, 0x01U, '2'};
    uint8_t frame[32U]{};
    size_type size = make_frame(frame, 1U, 0x04U, stream, length_two, sizeof(length_two));
    ASSERT_EQ(mismatch.feed(frame, size), castle::status::ok);
    const uint8_t one_byte[] = {'x'};
    size = make_frame(frame, 0U, 0x01U, stream, one_byte, sizeof(one_byte));
    EXPECT_EQ(mismatch.feed(frame, size), castle::status::invalid_argument);
    EXPECT_EQ(mismatch.last_error(), error_code::protocol_error);

    http2_session<1U, 256U, 16384U, 32U> informational;
    ASSERT_EQ(informational.start(), castle::status::ok);
    uint32_t info_stream = open_get(informational);
    const uint8_t status_100[] = {0x08U, 0x03U, '1', '0', '0'}; // Literal :status = 100.
    size = make_frame(frame, 1U, 0x05U, info_stream, status_100, sizeof(status_100));
    EXPECT_EQ(informational.feed(frame, size), castle::status::invalid_argument);
    EXPECT_EQ(informational.last_error(), error_code::protocol_error);

    http2_session<1U, 256U, 16384U, 32U> switching_protocols;
    ASSERT_EQ(switching_protocols.start(), castle::status::ok);
    uint32_t switch_stream = open_get(switching_protocols);
    const uint8_t status_101[] = {0x08U, 0x03U, '1', '0', '1'};
    size = make_frame(frame, 1U, 0x04U, switch_stream, status_101, sizeof(status_101));
    EXPECT_EQ(switching_protocols.feed(frame, size), castle::status::invalid_argument);
    EXPECT_EQ(switching_protocols.last_error(), error_code::protocol_error);
}

TEST(Http2Session, RejectsBodyForNoBodyResponseAndNonTerminalTrailers)
{
    http2_session<1U, 256U, 16384U, 32U> no_body;
    ASSERT_EQ(no_body.start(), castle::status::ok);
    uint32_t no_body_stream = open_get(no_body);
    const uint8_t status_204[] = {0x89U};
    uint8_t frame[32U]{};
    size_type size = make_frame(frame, 1U, 0x04U, no_body_stream, status_204, sizeof(status_204));
    ASSERT_EQ(no_body.feed(frame, size), castle::status::ok);
    const uint8_t body[] = {'x'};
    size = make_frame(frame, 0U, 0x01U, no_body_stream, body, sizeof(body));
    EXPECT_EQ(no_body.feed(frame, size), castle::status::invalid_argument);
    EXPECT_EQ(no_body.last_error(), error_code::protocol_error);

    http2_session<1U, 256U, 16384U, 32U> trailers;
    ASSERT_EQ(trailers.start(), castle::status::ok);
    uint32_t trailer_stream = open_get(trailers);
    const uint8_t status_200[] = {0x88U};
    size = make_frame(frame, 1U, 0x04U, trailer_stream, status_200, sizeof(status_200));
    ASSERT_EQ(trailers.feed(frame, size), castle::status::ok);
    const uint8_t trailer[] = {0x00U, 0x01U, 'x', 0x01U, 'y'};
    size = make_frame(frame, 1U, 0x04U, trailer_stream, trailer, sizeof(trailer));
    EXPECT_EQ(trailers.feed(frame, size), castle::status::invalid_argument);
    EXPECT_EQ(trailers.last_error(), error_code::protocol_error);
}

TEST(Http2Session, RequestBodyWaitsForStreamWindowCredit)
{
    http2_session<1U, 256U, 16384U, 64U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);

    // The peer reduces its initial stream send window to zero before the request is opened.
    const uint8_t zero_window_setting[] = {0x00U, 0x04U, 0x00U, 0x00U, 0x00U, 0x00U};
    uint8_t frame[64U]{};
    size_type size = make_frame(frame, 4U, 0U, 0U, zero_window_setting, sizeof(zero_window_setting));
    ASSERT_EQ(session.feed(frame, size), castle::status::ok);

    typename decltype(session)::headers_type headers;
    uint32_t stream = 0U;
    EXPECT_EQ(session.open_stream(method::post, "/upload", "peer.test", headers, "abc", stream), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    EXPECT_EQ(session.output_size(), 0U); // No DATA frame while the stream has no credit.

    const uint8_t credit[] = {0U, 0U, 0U, 3U};
    size = make_frame(frame, 8U, 0U, stream, credit, sizeof(credit));
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);
    ASSERT_GE(session.output_size(), 12U);
    EXPECT_EQ(session.pending_output()[3U], static_cast<uint8_t>(http2_frame_type::data));
    EXPECT_EQ(session.pending_output()[4U], 0x01U); // END_STREAM on final request DATA.
    EXPECT_EQ(session.pending_output()[9U], 'a');
    EXPECT_EQ(session.pending_output()[10U], 'b');
    EXPECT_EQ(session.pending_output()[11U], 'c');
}

TEST(Http2Session, AcknowledgesSettingsAndPingAndIgnoresUnknownExtension)
{
    http2_session<1U, 256U, 16384U, 128U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    uint8_t frame[32U]{};
    size_type size = make_frame(frame, 4U, 0U, 0U, nullptr, 0U);
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);
    EXPECT_EQ(session.pending_output()[3U], static_cast<uint8_t>(http2_frame_type::settings));
    EXPECT_EQ(session.pending_output()[4U], 0x01U);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    const uint8_t ping[] = {'1','2','3','4','5','6','7','8'};
    size = make_frame(frame, 6U, 0U, 0U, ping, sizeof(ping));
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);
    EXPECT_EQ(session.pending_output()[3U], static_cast<uint8_t>(http2_frame_type::ping));
    EXPECT_EQ(session.pending_output()[4U], 0x01U);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    const uint8_t extension[] = {1U, 2U, 3U};
    size = make_frame(frame, 0xF0U, 0U, 0U, extension, sizeof(extension));
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);
}

TEST(Http2Session, HandlesWindowUpdatesAndRejectsZeroCredit)
{
    http2_session<1U, 256U, 16384U, 128U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint8_t frame[16U]{};
    uint8_t increment[4U] = {0U, 0U, 0U, 1U};
    size_type size = make_frame(frame, 8U, 0U, 0U, increment, sizeof(increment));
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);
    increment[3U] = 0U;
    size = make_frame(frame, 8U, 0U, 0U, increment, sizeof(increment));
    EXPECT_EQ(session.feed(frame, size), castle::status::invalid_argument);
    EXPECT_EQ(session.last_error(), error_code::flow_control_error);
}

TEST(Http2Session, ResetsStreamsAndProcessesGoaway)
{
    http2_session<2U, 256U, 16384U, 128U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint32_t stream = open_get(session);
    castle::status callback_status = castle::status::ok;
    EXPECT_EQ(session.set_response_callback([&callback_status](castle::status result, response_view const&) { callback_status = result; }), castle::status::ok);
    uint8_t frame[24U]{};
    uint8_t reset_code[4U] = {};
    size_type size = make_frame(frame, 3U, 0U, stream, reset_code, sizeof(reset_code));
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);
    EXPECT_EQ(callback_status, castle::status::data_loss);
    EXPECT_EQ(session.last_error(), error_code::stream_reset);
    EXPECT_EQ(session.release_stream(stream), castle::status::ok);

    uint32_t later = open_get(session);
    uint8_t goaway[8U] = {};
    size = make_frame(frame, 7U, 0U, 0U, goaway, sizeof(goaway));
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);
    EXPECT_TRUE(session.goaway_received());
    EXPECT_EQ(session.last_error(), error_code::goaway_received);
    EXPECT_EQ(callback_status, castle::status::data_loss);
    typename decltype(session)::headers_type headers;
    uint32_t rejected = 0U;
    EXPECT_EQ(session.open_stream(method::get, "/", "peer.test", headers, string_view(), rejected), castle::status::data_loss);
    EXPECT_EQ(later, 3U);
}

TEST(Http2Session, ReportsCompressionErrorsAndPrematurePeerClosure)
{
    http2_session<1U, 256U, 16384U, 128U> compression;
    ASSERT_EQ(compression.start(), castle::status::ok);
    uint32_t stream = open_get(compression);
    const uint8_t invalid_index[] = {0x80U};
    uint8_t frame[16U]{};
    size_type size = make_frame(frame, 1U, 0x04U, stream, invalid_index, sizeof(invalid_index));
    EXPECT_EQ(compression.feed(frame, size), castle::status::invalid_argument);
    EXPECT_EQ(compression.last_error(), error_code::compression_error);

    http2_session<1U, 256U, 16384U, 128U> closed;
    ASSERT_EQ(closed.start(), castle::status::ok);
    EXPECT_EQ(closed.feed(nullptr, 0U, true), castle::status::data_loss);
    EXPECT_EQ(closed.last_error(), error_code::connection_closed);
}

TEST(Http2Session, RejectsInvalidPeerSettingsAndHeaderBlocks)
{
    http2_session<1U, 256U, 16384U, 128U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint8_t setting[6U] = {0U, 4U, 0x80U, 0U, 0U, 0U}; // INITIAL_WINDOW_SIZE > 2^31-1
    uint8_t frame[24U]{};
    size_type size = make_frame(frame, 4U, 0U, 0U, setting, sizeof(setting));
    EXPECT_EQ(session.feed(frame, size), castle::status::invalid_argument);
    EXPECT_EQ(session.last_error(), error_code::flow_control_error);

    http2_session<1U, 256U, 16384U, 128U> push;
    ASSERT_EQ(push.start(), castle::status::ok);
    const uint8_t push_payload[] = {0U, 0U, 0U, 1U};
    size = make_frame(frame, 5U, 0U, 1U, push_payload, sizeof(push_payload));
    EXPECT_EQ(push.feed(frame, size), castle::status::invalid_argument);
    EXPECT_EQ(push.last_error(), error_code::protocol_error);
}

TEST(Http2Session, ReturnsReceiveCreditInHalfWindowBatches)
{
    http2_session<1U, 256U, 16384U, 64U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint32_t stream = open_get(session);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    size_type streamed = 0U;
    ASSERT_EQ(session.set_body_callback([&streamed](uint32_t, string_view chunk) { streamed += chunk.size(); }), castle::status::ok);

    static uint8_t frame[16384U + 9U];
    static uint8_t payload[16384U];
    const uint8_t status_200[] = {0x88U};
    size_type size = make_frame(frame, 1U, 0x04U, stream, status_200, sizeof(status_200));
    ASSERT_EQ(session.feed(frame, size), castle::status::ok);
    size = make_frame(frame, 0U, 0U, stream, payload, sizeof(payload));
    ASSERT_EQ(session.feed(frame, size), castle::status::ok);
    EXPECT_EQ(session.output_size(), 0U); // 16 KiB is below the half-window threshold
    ASSERT_EQ(session.feed(frame, size), castle::status::ok);
    EXPECT_EQ(streamed, 32768U);

    ASSERT_EQ(session.output_size(), 26U); // connection and stream WINDOW_UPDATE frames
    const uint8_t* output = session.pending_output();
    EXPECT_EQ(output[3U], static_cast<uint8_t>(http2_frame_type::window_update));
    EXPECT_EQ(detail::read_u32(output + 5U), 0U);
    EXPECT_EQ(detail::read_u32(output + 9U), 32768U);
    EXPECT_EQ(output[13U + 3U], static_cast<uint8_t>(http2_frame_type::window_update));
    EXPECT_EQ(detail::read_u32(output + 13U + 5U), stream);
    EXPECT_EQ(detail::read_u32(output + 13U + 9U), 32768U);
}

TEST(Http2Session, AcceptsServerEnablePushZeroOnly)
{
    http2_session<1U, 256U, 16384U, 64U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint8_t setting[6U] = {0U, 2U, 0U, 0U, 0U, 0U};
    uint8_t frame[24U]{};
    size_type size = make_frame(frame, 4U, 0U, 0U, setting, sizeof(setting));
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);
    EXPECT_FALSE(session.failed());

    setting[5U] = 1U;
    size = make_frame(frame, 4U, 0U, 0U, setting, sizeof(setting));
    EXPECT_EQ(session.feed(frame, size), castle::status::invalid_argument);
    EXPECT_EQ(session.last_error(), error_code::protocol_error);
}

TEST(Http2Session, BodyLimitCancelsOnlyTheAffectedStream)
{
    http2_session<2U, 256U, 16384U, 2U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint32_t first = open_get(session);
    uint32_t second = open_get(session);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    castle::status first_result = castle::status::ok;
    ASSERT_EQ(session.set_response_callback([&first_result, first](castle::status result, response_view const& view)
    {
        if (view.stream_id == first)
        {
            first_result = result;
        }
    }), castle::status::ok);

    const uint8_t status_200[] = {0x88U};
    const uint8_t body[] = {'a', 'b', 'c', 'd'};
    uint8_t frame[32U]{};
    size_type size = make_frame(frame, 1U, 0x04U, first, status_200, sizeof(status_200));
    ASSERT_EQ(session.feed(frame, size), castle::status::ok);
    size = make_frame(frame, 0U, 0U, first, body, sizeof(body));
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);
    EXPECT_EQ(first_result, castle::status::out_of_range);
    EXPECT_EQ(session.last_error(), error_code::body_limit);
    EXPECT_FALSE(session.failed());
    EXPECT_EQ(session.active_stream_count(), 1U);
    ASSERT_EQ(session.output_size(), 13U); // RST_STREAM(CANCEL)
    EXPECT_EQ(session.pending_output()[3U], static_cast<uint8_t>(http2_frame_type::rst_stream));
    EXPECT_EQ(detail::read_u32(session.pending_output() + 5U), first);
    EXPECT_EQ(detail::read_u32(session.pending_output() + 9U), 8U);

    size = make_frame(frame, 0U, 0x01U, first, body, sizeof(body)); // in-flight data is discarded
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);

    size = make_frame(frame, 1U, 0x04U, second, status_200, sizeof(status_200));
    ASSERT_EQ(session.feed(frame, size), castle::status::ok);
    size = make_frame(frame, 0U, 0x01U, second, body, 2U);
    EXPECT_EQ(session.feed(frame, size), castle::status::ok);
    EXPECT_EQ(session.response(second).body.size(), 2U);
    EXPECT_EQ(session.active_stream_count(), 0U);
}

TEST(Http2Session, StreamErrorDoesNotMaskLaterFrameError)
{
    http2_session<1U, 256U, 16384U, 64U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint32_t stream = open_get(session);
    uint8_t frame[24U]{};
    uint8_t reset_code[4U] = {};
    size_type size = make_frame(frame, 3U, 0U, stream, reset_code, sizeof(reset_code));
    ASSERT_EQ(session.feed(frame, size), castle::status::ok);
    ASSERT_EQ(session.last_error(), error_code::stream_reset);

    const uint8_t bad_ping[9U] = {0U, 0U, 7U, 0x06U, 0U, 0U, 0U, 0U, 0U};
    EXPECT_EQ(session.feed(bad_ping, sizeof(bad_ping)), castle::status::invalid_argument);
    EXPECT_EQ(session.last_error(), error_code::invalid_frame);
}

TEST(Http2Session, EncodesRequestWithStaticTableIndexes)
{
    http2_session<1U, 256U, 16384U, 64U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    open_get(session);
    const uint8_t* frame = session.pending_output();
    EXPECT_EQ(frame[3U], static_cast<uint8_t>(http2_frame_type::headers));
    EXPECT_EQ(frame[9U], 0x82U);  // :method GET
    EXPECT_EQ(frame[10U], 0x86U); // :scheme http
}

namespace
{
// Appends one literal field (no indexing) at `at` and returns the new end of `block`.
size_type put_literal(uint8_t* block, size_type at, char const* name, char const* value)
{
    size_type used = 0U;
    EXPECT_EQ(hpack_encoder::encode_literal(name, value, block + at, 1024U - at, used), castle::status::ok);
    return at + used;
}

// Feeds one server frame with `size` payload bytes into `session`.
template <typename Session>
castle::status respond(Session& session, uint32_t stream_id, uint8_t flags, uint8_t const* payload, size_type size,
                       http2_frame_type type = http2_frame_type::headers)
{
    uint8_t frame[2048U]{};
    size_type total = make_frame(frame, static_cast<uint8_t>(type), flags, stream_id, payload, size);
    return session.feed(frame, total);
}

// Opens one GET stream on a started session and returns its identifier.
template <typename Session>
uint32_t start_get(Session& session)
{
    EXPECT_EQ(session.start(), castle::status::ok);
    return open_get(session);
}
}

TEST(Http2FrameParser, ArgumentsCallbackFailuresAndPartialPayloads)
{
    http2_frame_parser<> parser;
    auto accept = [](http2_frame_view const&) { return castle::status::ok; };
    EXPECT_EQ(parser.feed(nullptr, 0U, accept), castle::status::ok);
    EXPECT_EQ(parser.feed(nullptr, 3U, accept), castle::status::invalid_argument);

    uint8_t ack[9U] = {0U, 0U, 0U, 4U, 1U, 0U, 0U, 0U, 0U};
    size_type consumed = 0U;
    EXPECT_EQ(parser.feed(ack, sizeof(ack), [](http2_frame_view const&) { return castle::status::invalid_argument; }, &consumed),
              castle::status::invalid_argument);
    EXPECT_EQ(consumed, 9U);

    const uint8_t payload[] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    uint8_t frame[32U]{};
    size_type frame_size = make_frame(frame, 6U, 0U, 0U, payload, sizeof(payload));
    http2_frame_parser<> partial;
    size_type calls = 0U;
    auto count = [&calls](http2_frame_view const&) { ++calls; return castle::status::ok; };
    EXPECT_EQ(partial.feed(frame, 12U, count), castle::status::ok);
    EXPECT_EQ(calls, 0U);
    EXPECT_EQ(partial.feed(frame + 12U, frame_size - 12U, count, &consumed), castle::status::ok);
    EXPECT_EQ(calls, 1U);
    EXPECT_EQ(consumed, frame_size - 12U);

    http2_frame_parser<> failing;
    EXPECT_EQ(failing.feed(frame, frame_size, [](http2_frame_view const&) { return castle::status::invalid_argument; }, &consumed),
              castle::status::invalid_argument);
    EXPECT_EQ(consumed, frame_size);

    http2_frame_parser<> latched;
    uint8_t oversized[9U] = {0x00U, 0x40U, 0x01U, 0x06U, 0U, 0U, 0U, 0U, 0U};
    EXPECT_EQ(latched.feed(oversized, sizeof(oversized), accept), castle::status::out_of_range);
    EXPECT_EQ(latched.feed(ack, sizeof(ack), accept), castle::status::out_of_range);
}

TEST(Http2FrameParser, RejectsEveryFrameTypeOutsideItsStreamAndLengthRules)
{
    struct sample
    {
        uint8_t type;
        uint8_t flags;
        uint32_t stream_id;
        uint32_t length;
    };
    sample const invalid[] = {
        {0U, 0U, 0U, 0U},   // DATA on stream 0
        {1U, 0U, 0U, 0U},   // HEADERS on stream 0
        {2U, 0U, 0U, 5U},   // PRIORITY on stream 0
        {2U, 0U, 1U, 4U},   // PRIORITY with the wrong length
        {3U, 0U, 0U, 4U},   // RST_STREAM on stream 0
        {3U, 0U, 1U, 5U},   // RST_STREAM with the wrong length
        {4U, 0U, 1U, 0U},   // SETTINGS on a stream
        {4U, 0U, 0U, 5U},   // SETTINGS length not a multiple of 6
        {4U, 1U, 0U, 6U},   // SETTINGS ACK carrying data
        {5U, 0U, 1U, 3U},   // PUSH_PROMISE too short
        {5U, 0U, 0U, 4U},   // PUSH_PROMISE on stream 0
        {6U, 0U, 1U, 8U},   // PING on a stream
        {6U, 0U, 0U, 7U},   // PING with the wrong length
        {7U, 0U, 1U, 8U},   // GOAWAY on a stream
        {7U, 0U, 0U, 7U},   // GOAWAY too short
        {8U, 0U, 0U, 5U},   // WINDOW_UPDATE with the wrong length
        {9U, 0U, 0U, 0U},   // CONTINUATION on stream 0
    };
    auto accept = [](http2_frame_view const&) { return castle::status::ok; };
    for (sample const& value : invalid)
    {
        uint8_t header[9U] = {static_cast<uint8_t>(value.length >> 16U), static_cast<uint8_t>(value.length >> 8U),
                              static_cast<uint8_t>(value.length), value.type, value.flags, 0U, 0U, 0U, 0U};
        detail::write_u32(header + 5U, value.stream_id);
        http2_frame_parser<> parser;
        EXPECT_EQ(parser.feed(header, sizeof(header), accept), castle::status::invalid_argument) << static_cast<int>(value.type);
        EXPECT_EQ(parser.last_error(), error_code::invalid_frame) << static_cast<int>(value.type);
    }

    http2_frame_parser<> valid;
    const uint8_t priority[5U] = {0U, 0U, 0U, 1U, 0U};
    uint8_t frame[32U]{};
    size_type size = make_frame(frame, 2U, 0U, 1U, priority, sizeof(priority));
    EXPECT_EQ(valid.feed(frame, size, accept), castle::status::ok);
    size = make_frame(frame, 9U, 0U, 1U, nullptr, 0U);
    EXPECT_EQ(valid.feed(frame, size, accept), castle::status::ok);
    size = make_frame(frame, 0xF0U, 0U, 0U, nullptr, 0U);
    EXPECT_EQ(valid.feed(frame, size, accept), castle::status::ok);
}

TEST(Http2Session, ArgumentsLifecycleAndRequestFieldRules)
{
    http2_session<2U, 256U, 16384U, 64U> session;
    http2_session<2U, 256U, 16384U, 64U>::headers_type headers;
    uint32_t stream = 0U;
    EXPECT_EQ(session.feed(nullptr, 0U), castle::status::not_configured);
    EXPECT_EQ(session.open_stream(method::get, "/", "h", headers, string_view(), stream), castle::status::not_configured);
    EXPECT_EQ(session.set_response_callback(static_cast<void (*)(castle::status, response_view const&)>(nullptr)),
              castle::status::invalid_callback);
    EXPECT_EQ(session.set_body_callback(static_cast<void (*)(uint32_t, string_view)>(nullptr)), castle::status::invalid_callback);
    ASSERT_EQ(session.start(), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    EXPECT_EQ(session.open_stream(method::custom, "/", "h", headers, string_view(), stream), castle::status::invalid_argument);
    EXPECT_EQ(session.open_stream(method::get, "bad path", "h", headers, string_view(), stream), castle::status::invalid_argument);
    EXPECT_EQ(session.open_stream(method::get, "/", "bad host", headers, string_view(), stream), castle::status::invalid_argument);
    EXPECT_EQ(session.open_stream(method::custom, "/", "h", headers, string_view(), stream, string_view("PURGE")), castle::status::ok);
    EXPECT_EQ(stream, 1U);

    http2_session<8U, 256U, 16384U, 64U> fields;
    http2_session<8U, 256U, 16384U, 64U>::headers_type checks;
    ASSERT_EQ(fields.start(), castle::status::ok);
    ASSERT_EQ(checks.add("Upper", "x"), castle::status::ok);
    EXPECT_EQ(fields.open_stream(method::get, "/", "h", checks, string_view(), stream), castle::status::invalid_argument);
    checks.clear();
    ASSERT_EQ(checks.add(":scheme", "https"), castle::status::ok);
    EXPECT_EQ(fields.open_stream(method::get, "/", "h", checks, string_view(), stream), castle::status::invalid_argument);
    checks.clear();
    ASSERT_EQ(checks.add("te", "gzip"), castle::status::ok);
    EXPECT_EQ(fields.open_stream(method::get, "/", "h", checks, string_view(), stream), castle::status::invalid_argument);
    checks.clear();
    char const* forbidden[] = {"host", "connection", "proxy-connection", "keep-alive", "transfer-encoding", "upgrade"};
    for (char const* name : forbidden)
    {
        checks.clear();
        ASSERT_EQ(checks.add(name, "x"), castle::status::ok);
        EXPECT_EQ(fields.open_stream(method::get, "/", "h", checks, string_view(), stream), castle::status::invalid_argument) << name;
    }
    checks.clear();
    ASSERT_EQ(checks.add("te", "trailers"), castle::status::ok);
    EXPECT_EQ(fields.open_stream(method::get, "/", "h", checks, string_view(), stream), castle::status::ok);
}

TEST(Http2Session, ParserErrorsLatchAndOpenStreamFailsAfterwards)
{
    http2_session<2U, 256U, 16384U, 64U> session;
    http2_session<2U, 256U, 16384U, 64U>::headers_type headers;
    uint32_t stream = 0U;
    ASSERT_EQ(session.start(), castle::status::ok);
    uint8_t bad[9U] = {};
    EXPECT_EQ(session.feed(bad, sizeof(bad)), castle::status::invalid_argument); // DATA on stream 0
    EXPECT_EQ(session.feed(bad, sizeof(bad)), castle::status::invalid_argument);
    EXPECT_EQ(session.open_stream(method::get, "/", "h", headers, string_view(), stream), castle::status::not_configured);
}

TEST(Http2Session, ReportsHeaderBlockAndOutputCapacityFailures)
{
    using one_byte = http2_session<1U, 1U, 16384U, 64U>;
    one_byte::headers_type headers;
    uint32_t stream = 0U;
    one_byte method_too_long;
    ASSERT_EQ(method_too_long.start(), castle::status::ok);
    EXPECT_EQ(method_too_long.open_stream(method::custom, "/", "h", headers, string_view(), stream, string_view("PURGEPURGE")),
              castle::status::out_of_range);
    one_byte scheme_full;
    ASSERT_EQ(scheme_full.start(), castle::status::ok);
    EXPECT_EQ(scheme_full.open_stream(method::get, "/", "h", headers, string_view(), stream), castle::status::out_of_range);

    http2_session<1U, 2U, 16384U, 64U> authority_full;
    ASSERT_EQ(authority_full.start(), castle::status::ok);
    EXPECT_EQ(authority_full.open_stream(method::get, "/", "h", headers, string_view(), stream), castle::status::out_of_range);

    http2_session<1U, 5U, 16384U, 64U> path_full;
    ASSERT_EQ(path_full.start(), castle::status::ok);
    EXPECT_EQ(path_full.open_stream(method::get, "/", "a", headers, string_view(), stream), castle::status::out_of_range);

    http2_session<1U, 6U, 16384U, 64U> field_full;
    ASSERT_EQ(field_full.start(), castle::status::ok);
    http2_session<1U, 6U, 16384U, 64U>::headers_type extra;
    ASSERT_EQ(extra.add("x-a", "1"), castle::status::ok);
    EXPECT_EQ(field_full.open_stream(method::get, "/", "a", extra, string_view(), stream), castle::status::out_of_range);

    using small_queue = http2_session<1U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 64U>;
    small_queue queue;
    small_queue::headers_type long_headers;
    ASSERT_EQ(queue.start(), castle::status::ok);
    std::string long_path(40U, 'p');
    EXPECT_EQ(queue.open_stream(method::get, string_view(long_path.data(), long_path.size()), "h", long_headers, string_view(), stream),
              castle::status::full);
    EXPECT_EQ(queue.last_error(), error_code::output_full);
}

TEST(Http2Session, SplitsLargeHeaderBlocksIntoContinuationFrames)
{
    using large_session = http2_session<1U, 32768U, 16384U, 64U, 24U, 64U, 256U, 0U, 1U, 40000U>;
    large_session session;
    ASSERT_EQ(session.start(), castle::status::ok);
    while (session.output_size() != 0U)
    {
        ASSERT_EQ(session.consume_output(session.pending_output_size()), castle::status::ok);
    }
    std::string path(17000U, 'p');
    large_session::headers_type headers;
    uint32_t stream = 0U;
    static char const body[] = "abc";
    ASSERT_EQ(session.open_stream(method::post, string_view(path.data(), path.size()), "h", headers, string_view(body, 3U), stream),
              castle::status::ok);
    EXPECT_EQ(stream, 1U);
    uint8_t const* out = session.pending_output();
    ASSERT_NE(out, nullptr);
    ASSERT_EQ(session.pending_output_size(), session.output_size());
    EXPECT_EQ(out[3U], static_cast<uint8_t>(http2_frame_type::headers));
    EXPECT_EQ(out[4U] & 0x04U, 0U); // END_HEADERS is only on the last fragment
    size_type first_length = (static_cast<size_type>(out[0U]) << 16U) | (static_cast<size_type>(out[1U]) << 8U) | out[2U];
    EXPECT_EQ(first_length, 16384U);
    uint8_t const* second = out + 9U + first_length;
    EXPECT_EQ(second[3U], static_cast<uint8_t>(http2_frame_type::continuation));
    EXPECT_EQ(second[4U] & 0x04U, 0x04U);
    size_type second_length = (static_cast<size_type>(second[0U]) << 16U) | (static_cast<size_type>(second[1U]) << 8U) | second[2U];
    uint8_t const* data = second + 9U + second_length;
    EXPECT_EQ(data[3U], static_cast<uint8_t>(http2_frame_type::data));
    EXPECT_EQ(data[4U], 0x01U);
}

TEST(Http2Session, QueueWrapsAndBoundsPendingOutput)
{
    using tiny = http2_session<1U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 64U>;
    tiny session;
    tiny::headers_type headers;
    uint32_t stream = 0U;
    ASSERT_EQ(session.start(), castle::status::ok);
    ASSERT_EQ(session.consume_output(40U), castle::status::ok);
    EXPECT_EQ(session.output_size(), 11U);
    ASSERT_EQ(session.open_stream(method::get, "/", "a", headers, string_view(), stream), castle::status::ok);
    EXPECT_EQ(session.output_size(), 26U);
    EXPECT_EQ(session.pending_output_size(), 24U); // the queue wraps: only the tail up to the end of storage is contiguous
    EXPECT_EQ(session.consume_output(25U), castle::status::out_of_range);
    while (session.output_size() != 0U)
    {
        ASSERT_EQ(session.consume_output(session.pending_output_size()), castle::status::ok);
    }
    EXPECT_EQ(session.pending_output(), nullptr);
}

TEST(Http2Session, ReleasesStreamsAndLooksUpUnknownOnes)
{
    http2_session<1U, 256U, 16384U, 64U> session;
    uint32_t stream = start_get(session);
    EXPECT_EQ(session.release_stream(99U), castle::status::not_found);
    EXPECT_EQ(session.release_stream(stream), castle::status::invalid_config);
    EXPECT_EQ(session.response(99U).status_code, 0U);
    uint8_t reset_code[4U] = {};
    EXPECT_EQ(respond(session, stream, 0U, reset_code, sizeof(reset_code), http2_frame_type::rst_stream), castle::status::ok);
    EXPECT_EQ(session.release_stream(stream), castle::status::ok);
}

TEST(Http2Session, SplitsRequestBodiesByFrameSizeAndOutputSpace)
{
    static char body[20000U]{};
    using roomy = http2_session<1U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 65536U>;
    roomy session;
    roomy::headers_type headers;
    uint32_t stream = 0U;
    ASSERT_EQ(session.start(), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    ASSERT_EQ(session.open_stream(method::post, "/", "h", headers, string_view(body, sizeof(body)), stream), castle::status::ok);
    uint8_t const* out = session.pending_output();
    size_type headers_length = (static_cast<size_type>(out[0U]) << 16U) | (static_cast<size_type>(out[1U]) << 8U) | out[2U];
    uint8_t const* first_data = out + 9U + headers_length;
    EXPECT_EQ(first_data[3U], static_cast<uint8_t>(http2_frame_type::data));
    EXPECT_EQ(first_data[4U], 0x00U);
    EXPECT_EQ(first_data[2U], 0x00U); // 16384-byte frame
    uint8_t const* second_data = first_data + 9U + 16384U;
    EXPECT_EQ(second_data[3U], static_cast<uint8_t>(http2_frame_type::data));
    EXPECT_EQ(second_data[4U], 0x01U); // END_STREAM on the final chunk
}

TEST(Http2Session, LimitsRequestBodyToQueueSpaceAndResumes)
{
    using small = http2_session<1U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 128U>;
    small session;
    small::headers_type headers;
    uint32_t stream = 0U;
    static char body[100U]{};
    ASSERT_EQ(session.start(), castle::status::ok);
    ASSERT_EQ(session.open_stream(method::post, "/", "h", headers, string_view(body, sizeof(body)), stream), castle::status::ok);
    EXPECT_EQ(session.output_size(), 128U); // preface, SETTINGS, HEADERS and a 53-byte DATA chunk fill the queue
    EXPECT_EQ(session.consume_output(0U), castle::status::ok); // full queue: no more DATA until space frees up

    std::string captured;
    while (session.output_size() != 0U)
    {
        size_type count = session.pending_output_size();
        captured.append(reinterpret_cast<char const*>(session.pending_output()), count);
        ASSERT_EQ(session.consume_output(count), castle::status::ok);
    }
    ASSERT_GE(captured.size(), 56U);
    size_type tail = captured.size() - 56U;
    EXPECT_EQ(static_cast<uint8_t>(captured[tail + 3U]), static_cast<uint8_t>(http2_frame_type::data));
    EXPECT_EQ(static_cast<uint8_t>(captured[tail + 4U]), 0x01U);
    EXPECT_EQ(static_cast<uint8_t>(captured[tail + 2U]), 47U);
}

TEST(Http2Session, ConnectionWindowLimitsEachStreamSeparately)
{
    using pair = http2_session<2U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 40000U>;
    pair session;
    pair::headers_type headers;
    uint32_t first = 0U;
    uint32_t second = 0U;
    static char body[16000U]{};
    ASSERT_EQ(session.start(), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    ASSERT_EQ(session.open_stream(method::post, "/", "h", headers, string_view(body, sizeof(body)), first), castle::status::ok);
    ASSERT_EQ(session.open_stream(method::post, "/", "h", headers, string_view(body, sizeof(body)), second), castle::status::ok);
    EXPECT_EQ(first, 1U);
    EXPECT_EQ(second, 3U);
    EXPECT_GT(session.output_size(), 2U * 16000U);
}

TEST(Http2Session, KeepsCreditWhenWindowUpdateCannotBeQueued)
{
    http2_session<1U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 128U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    http2_session<1U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 128U>::headers_type headers;
    uint32_t stream = 0U;
    std::string path(50U, 'p');
    ASSERT_EQ(session.open_stream(method::get, string_view(path.data(), path.size()), "h", headers, string_view(), stream), castle::status::ok);
    EXPECT_EQ(session.output_size(), 117U); // 11 bytes of queue space remain, too few for a WINDOW_UPDATE
    ASSERT_EQ(session.set_body_callback([](uint32_t, string_view) {}), castle::status::ok);
    const uint8_t status_200[] = {0x88U};
    ASSERT_EQ(respond(session, stream, 0x04U, status_200, sizeof(status_200)), castle::status::ok);
    static uint8_t payload[16384U]{};
    static uint8_t frame[16384U + 9U]{};
    for (uint32_t i = 0U; i < 3U; ++i)
    {
        size_type total = make_frame(frame, 0U, 0U, stream, payload, sizeof(payload));
        EXPECT_EQ(session.feed(frame, total), castle::status::ok) << i;
    }
    size_type total = make_frame(frame, 0U, 0U, stream, payload, sizeof(payload));
    EXPECT_EQ(session.feed(frame, total), castle::status::invalid_argument);
    EXPECT_EQ(session.last_error(), error_code::flow_control_error);
}

TEST(Http2Session, DiscardsFramesOfReleasedAndResetStreams)
{
    http2_session<1U, 256U, 16384U, 64U> session;
    uint32_t stream = start_get(session);
    const uint8_t complete[] = {0x88U};
    ASSERT_EQ(respond(session, stream, 0x05U, complete, sizeof(complete)), castle::status::ok);
    ASSERT_EQ(session.release_stream(stream), castle::status::ok);
    const uint8_t data[] = {'o', 'k'};
    EXPECT_EQ(respond(session, stream, 0x01U, data, sizeof(data), http2_frame_type::data), castle::status::ok);

    http2_session<2U, 256U, 16384U, 64U> reset;
    uint32_t reset_stream = start_get(reset);
    uint8_t reset_code[4U] = {};
    ASSERT_EQ(respond(reset, reset_stream, 0U, reset_code, sizeof(reset_code), http2_frame_type::rst_stream), castle::status::ok);
    EXPECT_EQ(respond(reset, reset_stream, 0x01U, data, sizeof(data), http2_frame_type::data), castle::status::ok);

    http2_session<2U, 256U, 16384U, 64U> even;
    uint32_t even_stream = start_get(even);
    (void)even_stream;
    EXPECT_EQ(respond(even, 2U, 0x01U, data, sizeof(data), http2_frame_type::data), castle::status::invalid_argument);

    http2_session<2U, 256U, 16384U, 64U> unissued;
    ASSERT_EQ(unissued.start(), castle::status::ok);
    EXPECT_EQ(respond(unissued, 3U, 0x01U, data, sizeof(data), http2_frame_type::data), castle::status::invalid_argument);
}

TEST(Http2Session, ValidatesDataFramingAndBodyLength)
{
    http2_session<1U, 256U, 16384U, 64U> early;
    uint32_t early_stream = start_get(early);
    const uint8_t data[] = {'o', 'k'};
    EXPECT_EQ(respond(early, early_stream, 0x01U, data, sizeof(data), http2_frame_type::data), castle::status::invalid_argument);

    const uint8_t headers_only[] = {0x88U};
    http2_session<1U, 256U, 16384U, 64U> padded_empty;
    uint32_t padded_stream = start_get(padded_empty);
    ASSERT_EQ(respond(padded_empty, padded_stream, 0x04U, headers_only, sizeof(headers_only)), castle::status::ok);
    EXPECT_EQ(respond(padded_empty, padded_stream, 0x08U, nullptr, 0U, http2_frame_type::data), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> padding_overrun;
    uint32_t overrun_stream = start_get(padding_overrun);
    ASSERT_EQ(respond(padding_overrun, overrun_stream, 0x04U, headers_only, sizeof(headers_only)), castle::status::ok);
    const uint8_t overrun[] = {5U, 'a'};
    EXPECT_EQ(respond(padding_overrun, overrun_stream, 0x08U, overrun, sizeof(overrun), http2_frame_type::data), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> padded;
    uint32_t padded_ok = start_get(padded);
    ASSERT_EQ(respond(padded, padded_ok, 0x04U, headers_only, sizeof(headers_only)), castle::status::ok);
    const uint8_t padded_body[] = {1U, 'o', 'k', 0U};
    EXPECT_EQ(respond(padded, padded_ok, 0x09U, padded_body, sizeof(padded_body), http2_frame_type::data), castle::status::ok);
    EXPECT_EQ(padded.response(padded_ok).body.size(), 2U);

    http2_session<1U, 256U, 16384U, 64U> empty_end;
    uint32_t empty_stream = start_get(empty_end);
    ASSERT_EQ(respond(empty_end, empty_stream, 0x04U, headers_only, sizeof(headers_only)), castle::status::ok);
    EXPECT_EQ(respond(empty_end, empty_stream, 0x01U, nullptr, 0U, http2_frame_type::data), castle::status::ok);
    EXPECT_TRUE(empty_end.response(empty_stream).keep_alive);

    http2_session<1U, 256U, 16384U, 64U> excess;
    uint32_t excess_stream = start_get(excess);
    uint8_t length_two[256U]{};
    length_two[0U] = 0x88U;
    size_type at = put_literal(length_two, 1U, "content-length", "2");
    ASSERT_EQ(respond(excess, excess_stream, 0x04U, length_two, at), castle::status::ok);
    const uint8_t three[] = {'a', 'b', 'c'};
    EXPECT_EQ(respond(excess, excess_stream, 0x01U, three, sizeof(three), http2_frame_type::data), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> chunks;
    uint32_t chunk_stream = start_get(chunks);
    uint8_t length_five[256U]{};
    length_five[0U] = 0x88U;
    at = put_literal(length_five, 1U, "content-length", "5");
    ASSERT_EQ(respond(chunks, chunk_stream, 0x04U, length_five, at), castle::status::ok);
    EXPECT_EQ(respond(chunks, chunk_stream, 0x00U, three, sizeof(three), http2_frame_type::data), castle::status::ok);
    EXPECT_EQ(respond(chunks, chunk_stream, 0x00U, three, sizeof(three), http2_frame_type::data), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> mismatch_end;
    uint32_t mismatch_stream = start_get(mismatch_end);
    uint8_t length_mismatch[256U]{};
    length_mismatch[0U] = 0x88U;
    at = put_literal(length_mismatch, 1U, "content-length", "5");
    EXPECT_EQ(respond(mismatch_end, mismatch_stream, 0x05U, length_mismatch, at), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> head;
    http2_session<1U, 256U, 16384U, 64U>::headers_type none;
    ASSERT_EQ(head.start(), castle::status::ok);
    uint32_t head_stream = 0U;
    ASSERT_EQ(head.open_stream(method::head, "/", "h", none, string_view(), head_stream), castle::status::ok);
    uint8_t length_ten[256U]{};
    length_ten[0U] = 0x88U;
    at = put_literal(length_ten, 1U, "content-length", "10");
    ASSERT_EQ(respond(head, head_stream, 0x04U, length_ten, at), castle::status::ok);
    EXPECT_EQ(respond(head, head_stream, 0x01U, nullptr, 0U, http2_frame_type::data), castle::status::ok);

    http2_session<1U, 256U, 16384U, 64U> head_body;
    http2_session<1U, 256U, 16384U, 64U>::headers_type none_body;
    ASSERT_EQ(head_body.start(), castle::status::ok);
    uint32_t head_body_stream = 0U;
    ASSERT_EQ(head_body.open_stream(method::head, "/", "h", none_body, string_view(), head_body_stream), castle::status::ok);
    ASSERT_EQ(respond(head_body, head_body_stream, 0x04U, headers_only, sizeof(headers_only)), castle::status::ok);
    EXPECT_EQ(respond(head_body, head_body_stream, 0x01U, data, sizeof(data), http2_frame_type::data), castle::status::invalid_argument);
}

TEST(Http2Session, ValidatesResponseHeaderBlocks)
{
    uint8_t block[1024U]{};
    size_type at = 0U;
    http2_session<1U, 256U, 16384U, 64U> big;
    uint32_t stream = start_get(big);
    std::string value(300U, 'a');
    at = put_literal(block, 0U, "x-big", value.c_str());
    EXPECT_EQ(respond(big, stream, 0x04U, block, at), castle::status::out_of_range);

    http2_session<1U, 256U, 16384U, 64U> reset;
    uint32_t reset_stream = start_get(reset);
    uint8_t reset_code[4U] = {};
    ASSERT_EQ(respond(reset, reset_stream, 0U, reset_code, sizeof(reset_code), http2_frame_type::rst_stream), castle::status::ok);
    const uint8_t status_200[] = {0x88U};
    EXPECT_EQ(respond(reset, reset_stream, 0x05U, status_200, sizeof(status_200)), castle::status::ok);

    struct case_t
    {
        char const* name;
        char const* value;
    };
    case_t const status_cases[] = {{":status", "2x0"}, {":status", "x00"}, {":status", "20x"}, {":status", "20"},
                                   {":status", "099"}, {":status", "600"}, {":status", "101"}};
    for (case_t const& value_case : status_cases)
    {
        http2_session<1U, 256U, 16384U, 64U> session;
        uint32_t id = start_get(session);
        at = put_literal(block, 0U, value_case.name, value_case.value);
        EXPECT_EQ(respond(session, id, 0x05U, block, at), castle::status::invalid_argument) << value_case.value;
        EXPECT_EQ(session.last_error(), error_code::protocol_error) << value_case.value;
    }

    http2_session<1U, 256U, 16384U, 64U> pseudo_after;
    uint32_t after = start_get(pseudo_after);
    at = put_literal(block, 0U, "x-a", "1");
    block[at++] = 0x88U;
    EXPECT_EQ(respond(pseudo_after, after, 0x05U, block, at), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> other_pseudo;
    uint32_t other = start_get(other_pseudo);
    at = put_literal(block, 0U, ":foo", "1");
    block[at++] = 0x88U;
    EXPECT_EQ(respond(other_pseudo, other, 0x05U, block, at), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> duplicate;
    uint32_t dup = start_get(duplicate);
    const uint8_t twice[] = {0x88U, 0x88U};
    EXPECT_EQ(respond(duplicate, dup, 0x05U, twice, sizeof(twice)), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> uppercase;
    uint32_t upper = start_get(uppercase);
    block[0U] = 0x88U;
    at = put_literal(block, 1U, "X-Up", "1");
    EXPECT_EQ(respond(uppercase, upper, 0x05U, block, at), castle::status::invalid_argument);

    char const* connection_fields[] = {"connection", "proxy-connection", "keep-alive", "transfer-encoding", "upgrade"};
    for (char const* name : connection_fields)
    {
        http2_session<1U, 256U, 16384U, 64U> session;
        uint32_t id = start_get(session);
        block[0U] = 0x88U;
        at = put_literal(block, 1U, name, "x");
        EXPECT_EQ(respond(session, id, 0x05U, block, at), castle::status::invalid_argument) << name;
    }

    http2_session<1U, 256U, 16384U, 64U> bad_length;
    uint32_t bad_length_stream = start_get(bad_length);
    block[0U] = 0x88U;
    at = put_literal(block, 1U, "content-length", "abc");
    EXPECT_EQ(respond(bad_length, bad_length_stream, 0x05U, block, at), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> trailer_length;
    uint32_t trailer_stream = start_get(trailer_length);
    ASSERT_EQ(respond(trailer_length, trailer_stream, 0x04U, status_200, sizeof(status_200)), castle::status::ok);
    at = put_literal(block, 0U, "content-length", "1");
    EXPECT_EQ(respond(trailer_length, trailer_stream, 0x05U, block, at), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> trailer_pseudo;
    uint32_t trailer_pseudo_stream = start_get(trailer_pseudo);
    ASSERT_EQ(respond(trailer_pseudo, trailer_pseudo_stream, 0x04U, status_200, sizeof(status_200)), castle::status::ok);
    EXPECT_EQ(respond(trailer_pseudo, trailer_pseudo_stream, 0x05U, status_200, sizeof(status_200)), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> trailer_open;
    uint32_t trailer_open_stream = start_get(trailer_open);
    ASSERT_EQ(respond(trailer_open, trailer_open_stream, 0x04U, status_200, sizeof(status_200)), castle::status::ok);
    at = put_literal(block, 0U, "x-trailer", "1");
    EXPECT_EQ(respond(trailer_open, trailer_open_stream, 0x04U, block, at), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> mismatch;
    uint32_t mismatch_stream = start_get(mismatch);
    block[0U] = 0x88U;
    at = put_literal(block, 1U, "content-length", "1");
    at = put_literal(block, at, "content-length", "2");
    EXPECT_EQ(respond(mismatch, mismatch_stream, 0x04U, block, at), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> repeated;
    uint32_t repeated_stream = start_get(repeated);
    block[0U] = 0x88U;
    at = put_literal(block, 1U, "content-length", "2");
    at = put_literal(block, at, "content-length", "2");
    ASSERT_EQ(respond(repeated, repeated_stream, 0x04U, block, at), castle::status::ok);
    const uint8_t two[] = {'o', 'k'};
    EXPECT_EQ(respond(repeated, repeated_stream, 0x01U, two, sizeof(two), http2_frame_type::data), castle::status::ok);

    http2_session<1U, 256U, 16384U, 64U> no_status;
    uint32_t no_status_stream = start_get(no_status);
    at = put_literal(block, 0U, "x-a", "1");
    EXPECT_EQ(respond(no_status, no_status_stream, 0x05U, block, at), castle::status::invalid_argument);

    http2_session<1U, 256U, 16384U, 64U> interim;
    uint32_t interim_stream = start_get(interim);
    at = put_literal(block, 0U, ":status", "103");
    ASSERT_EQ(respond(interim, interim_stream, 0x04U, block, at), castle::status::ok);
    EXPECT_EQ(respond(interim, interim_stream, 0x05U, status_200, sizeof(status_200)), castle::status::ok);
    EXPECT_EQ(interim.response(interim_stream).status_code, 200U);

    http2_session<1U, 256U, 16384U, 64U> goaway;
    uint32_t goaway_stream = start_get(goaway);
    ASSERT_EQ(respond(goaway, goaway_stream, 0x05U, status_200, sizeof(status_200)), castle::status::ok);
    uint8_t goaway_payload[8U] = {};
    ASSERT_EQ(respond(goaway, 0U, 0U, goaway_payload, sizeof(goaway_payload), http2_frame_type::goaway), castle::status::ok);
    EXPECT_FALSE(goaway.response(goaway_stream).keep_alive);
}

TEST(Http2Session, HandlesSettingsWindowsAndAcknowledgements)
{
    http2_session<2U, 256U, 16384U, 64U> session;
    uint32_t stream = start_get(session);
    (void)stream;
    EXPECT_EQ(respond(session, 0U, 0x01U, nullptr, 0U, http2_frame_type::settings), castle::status::ok);
    const uint8_t push_off[6U] = {0U, 2U, 0U, 0U, 0U, 0U};
    EXPECT_EQ(respond(session, 0U, 0U, push_off, sizeof(push_off), http2_frame_type::settings), castle::status::ok);
    const uint8_t frame_size[6U] = {0U, 5U, 0U, 0U, 0x40U, 0U};
    EXPECT_EQ(respond(session, 0U, 0U, frame_size, sizeof(frame_size), http2_frame_type::settings), castle::status::ok);
    const uint8_t unpaced[6U] = {0U, 5U, 0U, 0U, 3U, 232U};
    http2_session<1U, 256U, 16384U, 64U> bad_size;
    ASSERT_EQ(bad_size.start(), castle::status::ok);
    EXPECT_EQ(respond(bad_size, 0U, 0U, unpaced, sizeof(unpaced), http2_frame_type::settings), castle::status::invalid_argument);
    const uint8_t oversized_frame[6U] = {0U, 5U, 0x01U, 0U, 0U, 0U};
    http2_session<1U, 256U, 16384U, 64U> too_big;
    ASSERT_EQ(too_big.start(), castle::status::ok);
    EXPECT_EQ(respond(too_big, 0U, 0U, oversized_frame, sizeof(oversized_frame), http2_frame_type::settings), castle::status::invalid_argument);

    const uint8_t push_on[6U] = {0U, 2U, 0U, 0U, 0U, 1U};
    http2_session<1U, 256U, 16384U, 64U> push;
    ASSERT_EQ(push.start(), castle::status::ok);
    EXPECT_EQ(respond(push, 0U, 0U, push_on, sizeof(push_on), http2_frame_type::settings), castle::status::invalid_argument);

    http2_session<2U, 256U, 16384U, 64U> window;
    uint32_t window_stream = start_get(window);
    const uint8_t window_1000[6U] = {0U, 4U, 0U, 0U, 3U, 232U};
    ASSERT_EQ(respond(window, 0U, 0U, window_1000, sizeof(window_1000), http2_frame_type::settings), castle::status::ok);
    const uint8_t grow[4U] = {0x7FU, 0xFFU, 0xFCU, 0x17U};
    ASSERT_EQ(respond(window, window_stream, 0U, grow, sizeof(grow), http2_frame_type::window_update), castle::status::ok);
    const uint8_t window_5000[6U] = {0U, 4U, 0U, 0U, 0x13U, 0x88U};
    EXPECT_EQ(respond(window, 0U, 0U, window_5000, sizeof(window_5000), http2_frame_type::settings), castle::status::invalid_argument);
    EXPECT_EQ(window.last_error(), error_code::flow_control_error);

    http2_session<2U, 256U, 16384U, 64U> huge_window;
    start_get(huge_window);
    const uint8_t huge[6U] = {0U, 4U, 0x80U, 0U, 0U, 0U};
    EXPECT_EQ(respond(huge_window, 0U, 0U, huge, sizeof(huge), http2_frame_type::settings), castle::status::invalid_argument);

    http2_session<2U, 256U, 16384U, 64U> limited;
    ASSERT_EQ(limited.start(), castle::status::ok);
    const uint8_t one_stream[6U] = {0U, 3U, 0U, 0U, 0U, 1U};
    ASSERT_EQ(respond(limited, 0U, 0U, one_stream, sizeof(one_stream), http2_frame_type::settings), castle::status::ok);
    uint32_t first = open_get(limited);
    EXPECT_EQ(first, 1U);
    http2_session<2U, 256U, 16384U, 64U>::headers_type headers;
    uint32_t second = 0U;
    EXPECT_EQ(limited.open_stream(method::get, "/", "peer.test", headers, string_view(), second), castle::status::out_of_range);

    http2_session<2U, 256U, 16384U, 64U> window_limit;
    uint32_t limit_stream = start_get(window_limit);
    (void)limit_stream;
    const uint8_t maximum[4U] = {0x7FU, 0xFFU, 0xFFU, 0xFFU};
    EXPECT_EQ(respond(window_limit, 0U, 0U, maximum, sizeof(maximum), http2_frame_type::window_update), castle::status::invalid_argument);

    http2_session<2U, 256U, 16384U, 64U> stream_limit;
    uint32_t stream_limit_id = start_get(stream_limit);
    EXPECT_EQ(respond(stream_limit, stream_limit_id, 0U, maximum, sizeof(maximum), http2_frame_type::window_update), castle::status::invalid_argument);
    EXPECT_EQ(stream_limit.last_error(), error_code::flow_control_error);

    http2_session<2U, 256U, 16384U, 64U> unknown_window;
    ASSERT_EQ(unknown_window.start(), castle::status::ok);
    EXPECT_EQ(respond(unknown_window, 9U, 0U, grow, sizeof(grow), http2_frame_type::window_update), castle::status::ok);
}

TEST(Http2Session, HandlesControlFramesAndHeaderBlockSequencing)
{
    http2_session<2U, 256U, 16384U, 64U> ping;
    ASSERT_EQ(ping.start(), castle::status::ok);
    ASSERT_EQ(ping.consume_output(ping.output_size()), castle::status::ok);
    const uint8_t payload[8U] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    EXPECT_EQ(respond(ping, 0U, 0x01U, payload, sizeof(payload), http2_frame_type::ping), castle::status::ok);
    EXPECT_EQ(ping.output_size(), 0U); // PING ACK is never answered

    http2_session<2U, 256U, 16384U, 64U> pushed;
    uint32_t pushed_stream = start_get(pushed);
    const uint8_t promised[4U] = {0U, 0U, 0U, 1U};
    EXPECT_EQ(respond(pushed, pushed_stream, 0U, promised, sizeof(promised), http2_frame_type::push_promise), castle::status::invalid_argument);

    http2_session<2U, 256U, 16384U, 64U> priority;
    uint32_t priority_stream = start_get(priority);
    const uint8_t priority_fields[5U] = {0U, 0U, 0U, 0U, 16U};
    EXPECT_EQ(respond(priority, priority_stream, 0U, priority_fields, sizeof(priority_fields), http2_frame_type::priority), castle::status::ok);

    http2_session<2U, 256U, 16384U, 64U> unknown_reset;
    ASSERT_EQ(unknown_reset.start(), castle::status::ok);
    uint8_t reset_code[4U] = {};
    EXPECT_EQ(respond(unknown_reset, 7U, 0U, reset_code, sizeof(reset_code), http2_frame_type::rst_stream), castle::status::ok);
    const uint8_t status_200[] = {0x88U};
    EXPECT_EQ(respond(unknown_reset, 5U, 0x05U, status_200, sizeof(status_200)), castle::status::invalid_argument);

    http2_session<2U, 256U, 16384U, 64U> bad_continuation;
    ASSERT_EQ(bad_continuation.start(), castle::status::ok);
    EXPECT_EQ(respond(bad_continuation, 1U, 0x05U, status_200, sizeof(status_200), http2_frame_type::continuation), castle::status::invalid_argument);

    http2_session<2U, 256U, 16384U, 64U> pending;
    uint32_t pending_stream = start_get(pending);
    EXPECT_EQ(respond(pending, pending_stream, 0x00U, status_200, sizeof(status_200)), castle::status::ok);
    EXPECT_EQ(respond(pending, pending_stream, 0x01U, status_200, sizeof(status_200), http2_frame_type::data), castle::status::invalid_argument);

    http2_session<2U, 256U, 16384U, 64U> mismatch;
    uint32_t mismatch_stream = start_get(mismatch);
    ASSERT_EQ(respond(mismatch, mismatch_stream, 0x00U, status_200, sizeof(status_200)), castle::status::ok);
    EXPECT_EQ(respond(mismatch, 3U, 0x04U, status_200, sizeof(status_200), http2_frame_type::continuation), castle::status::invalid_argument);

    http2_session<4U, 6U, 16384U, 64U> small_block;
    ASSERT_EQ(small_block.start(), castle::status::ok);
    http2_session<4U, 6U, 16384U, 64U>::headers_type small_headers;
    uint32_t small_stream = 0U;
    ASSERT_EQ(small_block.open_stream(method::get, "/", "a", small_headers, string_view(), small_stream), castle::status::ok);
    ASSERT_EQ(respond(small_block, small_stream, 0x00U, status_200, sizeof(status_200)), castle::status::ok);
    const uint8_t six[6U] = {1U, 2U, 3U, 4U, 5U, 6U};
    EXPECT_EQ(respond(small_block, small_stream, 0x04U, six, sizeof(six), http2_frame_type::continuation), castle::status::out_of_range);

    http2_session<2U, 256U, 16384U, 64U> fragments;
    uint32_t fragment_stream = start_get(fragments);
    ASSERT_EQ(respond(fragments, fragment_stream, 0x01U, status_200, sizeof(status_200)), castle::status::ok);
    ASSERT_EQ(respond(fragments, fragment_stream, 0x00U, nullptr, 0U, http2_frame_type::continuation), castle::status::ok);
    EXPECT_EQ(respond(fragments, fragment_stream, 0x04U, nullptr, 0U, http2_frame_type::continuation), castle::status::ok);

    http2_session<2U, 256U, 16384U, 64U> padding;
    uint32_t padding_stream = start_get(padding);
    EXPECT_EQ(respond(padding, padding_stream, 0x08U, nullptr, 0U), castle::status::invalid_argument);
    http2_session<2U, 256U, 16384U, 64U> overrun;
    uint32_t overrun_stream = start_get(overrun);
    const uint8_t pad_too_big[] = {5U, 0x88U};
    EXPECT_EQ(respond(overrun, overrun_stream, 0x08U, pad_too_big, sizeof(pad_too_big)), castle::status::invalid_argument);
    http2_session<2U, 256U, 16384U, 64U> priority_short;
    uint32_t priority_short_stream = start_get(priority_short);
    const uint8_t short_priority[] = {0x88U, 0U, 0U, 0U};
    EXPECT_EQ(respond(priority_short, priority_short_stream, 0x24U, short_priority, sizeof(short_priority)), castle::status::invalid_argument);
    http2_session<2U, 256U, 16384U, 64U> padded_ok;
    uint32_t padded_ok_stream = start_get(padded_ok);
    const uint8_t padded_block[] = {1U, 0x88U, 0U};
    EXPECT_EQ(respond(padded_ok, padded_ok_stream, 0x0DU, padded_block, sizeof(padded_block)), castle::status::ok);
    http2_session<2U, 256U, 16384U, 64U> priority_ok;
    uint32_t priority_ok_stream = start_get(priority_ok);
    const uint8_t priority_block[] = {0U, 0U, 0U, 0U, 0U, 0x88U};
    EXPECT_EQ(respond(priority_ok, priority_ok_stream, 0x25U, priority_block, sizeof(priority_block)), castle::status::ok);

    http2_session<2U, 256U, 16384U, 64U> goaway;
    uint32_t goaway_stream = start_get(goaway);
    uint8_t goaway_payload[8U] = {};
    castle::status callback_status = castle::status::ok;
    ASSERT_EQ(goaway.set_response_callback([&callback_status](castle::status result, response_view const&) { callback_status = result; }), castle::status::ok);
    ASSERT_EQ(respond(goaway, 0U, 0U, goaway_payload, sizeof(goaway_payload), http2_frame_type::goaway), castle::status::ok);
    EXPECT_EQ(callback_status, castle::status::data_loss);
    (void)goaway_stream;
}

TEST(Http2Session, FailsConnectionWhenOutputIsFull)
{
    using full = http2_session<1U, 256U, 16384U, 2U, 8U, 32U, 64U, 0U, 1U, 128U>;
    full body_limit;
    full::headers_type headers;
    uint32_t stream = 0U;
    ASSERT_EQ(body_limit.start(), castle::status::ok);
    std::string path(50U, 'p');
    ASSERT_EQ(body_limit.open_stream(method::get, string_view(path.data(), path.size()), "h", headers, string_view(), stream), castle::status::ok);
    const uint8_t status_200[] = {0x88U};
    ASSERT_EQ(respond(body_limit, stream, 0x04U, status_200, sizeof(status_200)), castle::status::ok);
    const uint8_t three[] = {'a', 'b', 'c'};
    EXPECT_EQ(respond(body_limit, stream, 0x00U, three, sizeof(three), http2_frame_type::data), castle::status::out_of_range);
    EXPECT_EQ(body_limit.last_error(), error_code::output_full);

    full settings;
    ASSERT_EQ(settings.start(), castle::status::ok);
    std::string long_path(60U, 'p');
    ASSERT_EQ(settings.open_stream(method::get, string_view(long_path.data(), long_path.size()), "h", headers, string_view(), stream), castle::status::ok);
    const uint8_t ack[] = {0U};
    (void)ack;
    EXPECT_EQ(respond(settings, 0U, 0U, nullptr, 0U, http2_frame_type::settings), castle::status::out_of_range);
    EXPECT_EQ(settings.last_error(), error_code::output_full);

    full ping;
    ASSERT_EQ(ping.start(), castle::status::ok);
    ASSERT_EQ(ping.open_stream(method::get, string_view(long_path.data(), long_path.size()), "h", headers, string_view(), stream), castle::status::ok);
    const uint8_t payload[8U] = {};
    EXPECT_EQ(respond(ping, 0U, 0U, payload, sizeof(payload), http2_frame_type::ping), castle::status::out_of_range);
    EXPECT_EQ(ping.last_error(), error_code::output_full);
}

TEST(Http2Session, SettlesRequestWindowsAndRejectsOverflows)
{
    http2_session<2U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 65536U> session;
    http2_session<2U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 65536U>::headers_type headers;
    uint32_t stream = 0U;
    ASSERT_EQ(session.start(), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    static char body[60U]{};
    ASSERT_EQ(session.open_stream(method::post, "/", "h", headers, string_view(body, sizeof(body)), stream), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    EXPECT_EQ(session.output_size(), 0U);
    const uint8_t overflow[4U] = {0x7FU, 0xFFU, 0xFFU, 0xFFU};
    EXPECT_EQ(respond(session, stream, 0U, overflow, sizeof(overflow), http2_frame_type::window_update), castle::status::invalid_argument);
    EXPECT_EQ(session.last_error(), error_code::flow_control_error);
}

TEST(Http2Session, UsesStreamHeaderNamesOnlyForKnownStreams)
{
    http2_session<2U, 256U, 16384U, 64U> session;
    uint32_t stream = start_get(session);
    (void)stream;
    const uint8_t status_200[] = {0x88U};
    EXPECT_EQ(respond(session, 5U, 0x05U, status_200, sizeof(status_200)), castle::status::invalid_argument);
    EXPECT_EQ(session.last_error(), error_code::protocol_error);
}

TEST(Http2FrameParser, FeedWithoutConsumedOutputReportsProgress)
{
    http2_frame_parser<> parser;
    uint8_t ping[17U]{};
    const uint8_t opaque[] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    size_type size = make_frame(ping, 6U, 0U, 0U, opaque, sizeof(opaque));
    EXPECT_EQ(parser.feed(ping, size, [](http2_frame_view const&) { return castle::status::ok; }), castle::status::ok);
}

TEST(Http2Session, ExhaustedStreamSlotsAndCompletedStreamRelease)
{
    http2_session<1U, 256U, 16384U, 64U> session;
    uint32_t stream = start_get(session);
    http2_session<1U, 256U, 16384U, 64U>::headers_type headers;
    uint32_t next = 0U;
    EXPECT_EQ(session.open_stream(method::get, "/", "h", headers, string_view(), next), castle::status::out_of_range);
    const uint8_t status_200[] = {0x88U};
    ASSERT_EQ(respond(session, stream, 0x05U, status_200, sizeof(status_200)), castle::status::ok);
    EXPECT_EQ(session.open_stream(method::get, "/", "h", headers, string_view(), next), castle::status::full);
    EXPECT_EQ(session.last_error(), error_code::stream_limit);
    EXPECT_EQ(session.release_stream(stream), castle::status::ok);
    EXPECT_EQ(session.open_stream(method::get, "/", "h", headers, string_view(), next), castle::status::ok);
    EXPECT_EQ(next, 3U);
}

TEST(Http2Session, ResetStreamHoldsItsSlotUntilReleased)
{
    http2_session<1U, 256U, 16384U, 64U> session;
    uint32_t stream = start_get(session);
    const uint8_t code[4U] = {0U, 0U, 0U, 0U};
    ASSERT_EQ(respond(session, stream, 0U, code, sizeof(code), http2_frame_type::rst_stream), castle::status::ok);
    http2_session<1U, 256U, 16384U, 64U>::headers_type headers;
    uint32_t next = 0U;
    EXPECT_EQ(session.open_stream(method::get, "/", "h", headers, string_view(), next), castle::status::full);
    EXPECT_EQ(session.release_stream(stream), castle::status::ok);
    EXPECT_EQ(session.open_stream(method::get, "/", "h", headers, string_view(), next), castle::status::ok);
    EXPECT_EQ(next, 3U);
}

TEST(Http2Session, ResetOfFinishedAndResetStreamsIsIgnored)
{
    http2_session<2U, 256U, 16384U, 64U> session;
    uint32_t done = start_get(session);
    const uint8_t status_200[] = {0x88U};
    ASSERT_EQ(respond(session, done, 0x05U, status_200, sizeof(status_200)), castle::status::ok);
    const uint8_t code[4U] = {0U, 0U, 0U, 0U};
    EXPECT_EQ(respond(session, done, 0U, code, sizeof(code), http2_frame_type::rst_stream), castle::status::ok);
    uint32_t pending = open_get(session);
    EXPECT_EQ(respond(session, pending, 0U, code, sizeof(code), http2_frame_type::rst_stream), castle::status::ok);
    EXPECT_EQ(respond(session, pending, 0U, code, sizeof(code), http2_frame_type::rst_stream), castle::status::ok);
}

TEST(Http2Session, HeadersOnFinishedStreamIsProtocolError)
{
    http2_session<2U, 256U, 16384U, 64U> session;
    uint32_t stream = start_get(session);
    const uint8_t status_200[] = {0x88U};
    ASSERT_EQ(respond(session, stream, 0x05U, status_200, sizeof(status_200)), castle::status::ok);
    EXPECT_EQ(respond(session, stream, 0x05U, status_200, sizeof(status_200)), castle::status::invalid_argument);
    EXPECT_EQ(session.last_error(), error_code::protocol_error);
}

TEST(Http2Session, InitialWindowChangeSkipsFinishedAndResetStreams)
{
    http2_session<2U, 256U, 16384U, 64U> session;
    uint32_t done = start_get(session);
    const uint8_t status_200[] = {0x88U};
    ASSERT_EQ(respond(session, done, 0x05U, status_200, sizeof(status_200)), castle::status::ok);
    uint32_t cancelled = open_get(session);
    const uint8_t code[4U] = {0U, 0U, 0U, 0U};
    ASSERT_EQ(respond(session, cancelled, 0U, code, sizeof(code), http2_frame_type::rst_stream), castle::status::ok);
    const uint8_t window[6U] = {0x00U, 0x04U, 0x00U, 0x01U, 0x00U, 0x00U};
    EXPECT_EQ(respond(session, 0U, 0U, window, sizeof(window), http2_frame_type::settings), castle::status::ok);
}

TEST(Http2Session, GoawayFailsOnlyUnprocessedOpenStreams)
{
    http2_session<3U, 256U, 16384U, 64U> session;
    size_type failed = 0U;
    ASSERT_EQ(session.set_response_callback([&failed](castle::status result, response_view const&)
    {
        if (result == castle::status::data_loss) { ++failed; }
    }), castle::status::ok);
    (void)start_get(session);
    (void)open_get(session);
    uint32_t done = open_get(session);
    const uint8_t status_200[] = {0x88U};
    ASSERT_EQ(respond(session, done, 0x05U, status_200, sizeof(status_200)), castle::status::ok);
    const uint8_t goaway[8U] = {0x00U, 0x00U, 0x00U, 0x01U, 0x00U, 0x00U, 0x00U, 0x00U};
    (void)respond(session, 0U, 0U, goaway, sizeof(goaway), http2_frame_type::goaway);
    EXPECT_EQ(failed, 1U);
    EXPECT_TRUE(session.goaway_received());
    EXPECT_FALSE(session.response(done).keep_alive);
}

TEST(Http2Session, RequestBodiesWaitForCreditAndSkipFinishedStreams)
{
    http2_session<2U, 256U, 16384U, 64U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    const uint8_t zero_window[6U] = {0x00U, 0x04U, 0x00U, 0x00U, 0x00U, 0x00U};
    ASSERT_EQ(respond(session, 0U, 0U, zero_window, sizeof(zero_window), http2_frame_type::settings), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    http2_session<2U, 256U, 16384U, 64U>::headers_type headers;
    static char body[60U]{};
    uint32_t first = 0U;
    uint32_t second = 0U;
    ASSERT_EQ(session.open_stream(method::post, "/", "h", headers, string_view(body, sizeof(body)), first), castle::status::ok);
    ASSERT_EQ(session.open_stream(method::post, "/", "h", headers, string_view(body, sizeof(body)), second), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    const uint8_t complete[] = {0x88U};
    ASSERT_EQ(respond(session, first, 0x05U, complete, sizeof(complete)), castle::status::ok);
    const uint8_t cancel[4U] = {0U, 0U, 0U, 0U};
    ASSERT_EQ(respond(session, second, 0U, cancel, sizeof(cancel), http2_frame_type::rst_stream), castle::status::ok);
    const uint8_t credit[4U] = {0U, 0U, 0U, 100U};
    EXPECT_EQ(respond(session, 0U, 0U, credit, sizeof(credit), http2_frame_type::window_update), castle::status::ok);
    EXPECT_EQ(session.output_size(), 0U);
}

TEST(Http2Session, ExhaustedConnectionWindowHoldsEveryStream)
{
    static char big[65535U]{};
    static char small[60U]{};
    http2_session<2U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 80000U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    http2_session<2U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 80000U>::headers_type headers;
    uint32_t first = 0U;
    uint32_t second = 0U;
    ASSERT_EQ(session.open_stream(method::post, "/", "h", headers, string_view(big, sizeof(big)), first), castle::status::ok);
    ASSERT_EQ(session.consume_output(session.output_size()), castle::status::ok);
    ASSERT_EQ(session.open_stream(method::post, "/", "h", headers, string_view(small, sizeof(small)), second), castle::status::ok);
    EXPECT_GT(session.output_size(), 0U);
    EXPECT_LT(session.output_size(), 9U + sizeof(small));
}

TEST(Http2Session, StreamReceiveWindowUnderflowIsRejected)
{
    http2_session<1U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 79U> session;
    ASSERT_EQ(session.start(), castle::status::ok);
    http2_session<1U, 256U, 16384U, 64U, 8U, 32U, 64U, 0U, 1U, 79U>::headers_type headers;
    uint32_t stream = 0U;
    ASSERT_EQ(session.open_stream(method::get, "/", "h", headers, string_view(), stream), castle::status::ok);
    const uint8_t status_200[] = {0x88U};
    ASSERT_EQ(respond(session, stream, 0x04U, status_200, sizeof(status_200)), castle::status::ok);
    ASSERT_EQ(session.set_body_callback([](uint32_t, string_view) {}), castle::status::ok);
    static uint8_t zeros[16384U]{};
    static uint8_t data_frame[9U + 16384U]{};
    size_type total = make_frame(data_frame, 0U, 0U, stream, zeros, sizeof(zeros));
    EXPECT_EQ(session.feed(data_frame, total), castle::status::ok);
    EXPECT_EQ(session.feed(data_frame, total), castle::status::ok);
    EXPECT_EQ(session.feed(data_frame, total), castle::status::ok);
    EXPECT_NE(session.feed(data_frame, total), castle::status::ok);
    EXPECT_EQ(session.last_error(), error_code::flow_control_error);
}

TEST(Http2Session, NoContentRejectsData)
{
    http2_session<2U, 256U, 16384U, 64U> session;
    uint32_t stream = start_get(session);
    const uint8_t status_204[] = {0x89U};
    ASSERT_EQ(respond(session, stream, 0x04U, status_204, sizeof(status_204)), castle::status::ok);
    const uint8_t data[] = {'x'};
    EXPECT_EQ(respond(session, stream, 0U, data, sizeof(data), http2_frame_type::data), castle::status::invalid_argument);
    EXPECT_EQ(session.last_error(), error_code::protocol_error);
}

TEST(Http2Session, NotModifiedRejectsDataAndAcceptsContentLengthFinish)
{
    http2_session<2U, 256U, 16384U, 64U> rejecting;
    uint32_t first = start_get(rejecting);
    const uint8_t status_304[] = {0x8BU};
    ASSERT_EQ(respond(rejecting, first, 0x04U, status_304, sizeof(status_304)), castle::status::ok);
    const uint8_t data[] = {'x'};
    EXPECT_EQ(respond(rejecting, first, 0U, data, sizeof(data), http2_frame_type::data), castle::status::invalid_argument);

    http2_session<2U, 256U, 16384U, 64U> finishing;
    uint32_t stream = start_get(finishing);
    uint8_t block[64U]{};
    block[0U] = 0x8BU;
    size_type end = put_literal(block, 1U, "content-length", "5");
    EXPECT_EQ(respond(finishing, stream, 0x05U, block, end), castle::status::ok);
    EXPECT_EQ(finishing.response(stream).status_code, 304U);
}

TEST(Http2Session, DataAfterCompletedResponseIsProtocolError)
{
    http2_session<2U, 256U, 16384U, 64U> session;
    uint32_t stream = start_get(session);
    const uint8_t status_200[] = {0x88U};
    ASSERT_EQ(respond(session, stream, 0x05U, status_200, sizeof(status_200)), castle::status::ok);
    const uint8_t data[] = {'x'};
    EXPECT_EQ(respond(session, stream, 0U, data, sizeof(data), http2_frame_type::data), castle::status::invalid_argument);
    EXPECT_EQ(session.last_error(), error_code::protocol_error);
}

TEST(Http2Session, ResponseHeaderCountLimitIsReported)
{
    http2_session<1U, 256U, 16384U, 64U, 2U> session;
    uint32_t stream = start_get(session);
    uint8_t block[64U]{};
    block[0U] = 0x88U;
    size_type end = put_literal(block, 1U, "x-a", "1");
    end = put_literal(block, end, "x-b", "2");
    EXPECT_NE(respond(session, stream, 0x04U, block, end), castle::status::ok);
    EXPECT_EQ(session.last_error(), error_code::header_limit);
}

TEST(Http2Session, RejectsNonDigitStatusValues)
{
    char const* values[] = {"/00", "0/0", "00/", ":00", "0:0", "00:"};
    for (char const* value : values)
    {
        http2_session<1U, 256U, 16384U, 64U> session;
        uint32_t stream = start_get(session);
        uint8_t block[64U]{};
        size_type end = put_literal(block, 0U, ":status", value);
        EXPECT_EQ(respond(session, stream, 0x04U, block, end), castle::status::invalid_argument) << value;
        EXPECT_EQ(session.last_error(), error_code::protocol_error) << value;
    }
}

TEST(Http2Session, UnknownStreamHasNoResponseAndFailedConnectionKeepsResponses)
{
    http2_session<2U, 256U, 16384U, 64U> session;
    uint32_t stream = start_get(session);
    const uint8_t status_200[] = {0x88U};
    ASSERT_EQ(respond(session, stream, 0x05U, status_200, sizeof(status_200)), castle::status::ok);
    EXPECT_EQ(session.response(99U).status_code, 0U);
    EXPECT_EQ(respond(session, 0U, 0U, status_200, 0U, http2_frame_type::data), castle::status::invalid_argument);
    EXPECT_FALSE(session.response(stream).keep_alive);
}

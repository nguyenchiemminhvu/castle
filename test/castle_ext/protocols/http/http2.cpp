#include <gtest/gtest.h>
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

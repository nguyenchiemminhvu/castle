#include <gtest/gtest.h>
#include "castle_ext/comm/http/http1.hpp"

using namespace castle::comm::http;
using castle::size_type;

TEST(Http1Request, BuildsStandardRequestAndAutomaticFields)
{
    basic_headers<4U, 32U, 64U> headers;
    EXPECT_EQ(headers.add("accept", "text/plain"), castle::status::ok);
    http1_request<256U, 4U, 32U, 64U> request;
    EXPECT_EQ(request.build(method::get, "/health", "unit.test:8080", headers), castle::status::ok);
    EXPECT_NE(request.data().find("GET /health HTTP/1.1"), string_view::npos);
    EXPECT_NE(request.data().find("Host: unit.test:8080"), string_view::npos);
    EXPECT_NE(request.data().find("Connection: keep-alive"), string_view::npos);
}

TEST(Http1Request, AddsContentLengthAndSupportsCustomMethods)
{
    basic_headers<4U, 32U, 64U> headers;
    http1_request<256U, 4U, 32U, 64U> request;
    EXPECT_EQ(request.build(method::post, "/items", "host:80", headers, "abc"), castle::status::ok);
    EXPECT_NE(request.data().find("Content-Length: 3"), string_view::npos);
    EXPECT_NE(request.data().find("abc"), string_view::npos);
    EXPECT_EQ(request.build(method::custom, "/", "host:80", headers, string_view(), "PURGE"), castle::status::ok);
    EXPECT_NE(request.data().find("PURGE / HTTP/1.1"), string_view::npos);
    EXPECT_EQ(request.build(method::custom, "/", "host:80", headers), castle::status::invalid_argument);
}

TEST(Http1Request, RejectsInvalidInputsAndCapacity)
{
    basic_headers<4U, 32U, 64U> headers;
    http1_request<64U, 4U, 32U, 64U> request;
    EXPECT_EQ(request.build(method::get, "bad path", "host:80", headers), castle::status::invalid_argument);
    EXPECT_EQ(request.build(method::get, "/", "bad host", headers), castle::status::invalid_argument);
    EXPECT_EQ(request.build(method::post, "/", "host:80", headers, "body"), castle::status::full);
    EXPECT_EQ(headers.add("content-length", "9"), castle::status::ok);
    http1_request<256U, 4U, 32U, 64U> bigger;
    EXPECT_EQ(bigger.build(method::post, "/", "host:80", headers, "body"), castle::status::invalid_argument);
}

TEST(Http1Parser, ParsesContentLengthIncrementallyAndInterimResponse)
{
    http1_response_parser<16U> parser;
    const char part1[] = "HTTP/1.1 100 Continue\r\n\r\nHTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhe";
    const char part2[] = "llo";
    size_type consumed = 0U;
    EXPECT_EQ(parser.feed(part1, sizeof(part1) - 1U, false, &consumed), castle::status::ok);
    EXPECT_EQ(consumed, sizeof(part1) - 1U);
    EXPECT_FALSE(parser.complete());
    EXPECT_EQ(parser.feed(part2, sizeof(part2) - 1U), castle::status::ok);
    EXPECT_TRUE(parser.complete());
    EXPECT_EQ(parser.response().status_code, 200U);
    EXPECT_EQ(parser.response().body.size(), 5U);
    EXPECT_EQ(parser.response().body[0U], 'h');
}

TEST(Http1Parser, ConnectionCloseDominatesKeepAliveTokens)
{
    http1_response_parser<16U> parser;
    const char response[] = "HTTP/1.1 200 OK\r\nConnection: close, keep-alive\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    EXPECT_EQ(parser.feed(response, sizeof(response) - 1U), castle::status::ok);
    EXPECT_TRUE(parser.complete());
    EXPECT_FALSE(parser.response().keep_alive);
}

TEST(Http1Parser, HandlesHttp10CloseDelimitedAndHead)
{
    http1_response_parser<16U> parser;
    const char response[] = "HTTP/1.0 200 OK\r\n\r\nhello";
    EXPECT_EQ(parser.feed(response, sizeof(response) - 1U, true), castle::status::ok);
    EXPECT_TRUE(parser.complete());
    EXPECT_EQ(parser.response().protocol, version::http_1_0);
    EXPECT_FALSE(parser.response().keep_alive);
    EXPECT_EQ(parser.response().body.size(), 5U);

    parser.reset(true);
    const char head[] = "HTTP/1.1 200 OK\r\nContent-Length: 100\r\n\r\n";
    EXPECT_EQ(parser.feed(head, sizeof(head) - 1U), castle::status::ok);
    EXPECT_TRUE(parser.complete());
}

TEST(Http1Parser, RejectsChunkedUnlessEnabledAndCanDecodeWhenEnabled)
{
    const char response[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nhi\r\n0\r\n\r\n";
    http1_response_parser<32U> disabled;
    EXPECT_EQ(disabled.feed(response, sizeof(response) - 1U), castle::status::invalid_config);
    EXPECT_EQ(disabled.last_error(), error_code::chunked_not_supported);

    http1_response_parser<32U, 8U, 32U, 64U, 128U, true> enabled;
    EXPECT_EQ(enabled.feed(response, sizeof(response) - 1U), castle::status::ok);
    EXPECT_TRUE(enabled.complete());
    EXPECT_EQ(enabled.response().body.size(), 2U);
    EXPECT_EQ(enabled.response().body[1U], 'i');

    const char with_trailer[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n1\r\nx\r\n0\r\nX-Check: yes\r\n\r\n";
    http1_response_parser<32U, 8U, 32U, 64U, 128U, true> trailer_parser;
    EXPECT_EQ(trailer_parser.feed(with_trailer, sizeof(with_trailer) - 1U), castle::status::ok);
    EXPECT_EQ(trailer_parser.response().header_count, 2U);
    EXPECT_TRUE(ascii_iequal(trailer_parser.response().headers[1U].name, "X-Check"));
    EXPECT_TRUE(ascii_iequal(trailer_parser.response().headers[1U].value, "yes"));

    const char forbidden_trailer[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nContent-Length: 2\r\n\r\n";
    http1_response_parser<32U, 8U, 32U, 64U, 128U, true> bad_trailer;
    EXPECT_EQ(bad_trailer.feed(forbidden_trailer, sizeof(forbidden_trailer) - 1U), castle::status::invalid_argument);
    EXPECT_EQ(bad_trailer.last_error(), error_code::invalid_chunk);
}

TEST(Http1Parser, RejectsProtocolUpgradeResponse)
{
    http1_response_parser<16U> parser;
    const char response[] = "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nUpgrade: websocket\r\n\r\n";
    EXPECT_EQ(parser.feed(response, sizeof(response) - 1U), castle::status::invalid_config);
    EXPECT_EQ(parser.last_error(), error_code::unsupported_feature);
}

TEST(Http1Parser, StreamsOversizedBodyAndReportsLimitWithoutCallback)
{
    http1_response_parser<2U> no_stream;
    const char oversized[] = "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\n";
    EXPECT_EQ(no_stream.feed(oversized, sizeof(oversized) - 1U), castle::status::out_of_range);
    EXPECT_EQ(no_stream.last_error(), error_code::body_limit);

    http1_response_parser<2U> streaming;
    size_type streamed_bytes = 0U;
    EXPECT_EQ(streaming.set_body_callback([&streamed_bytes](string_view chunk) { streamed_bytes += chunk.size(); }), castle::status::ok);
    const char body[] = "HTTP/1.1 200 OK\r\nContent-Length: 4\r\n\r\ndata";
    EXPECT_EQ(streaming.feed(body, sizeof(body) - 1U), castle::status::ok);
    EXPECT_TRUE(streaming.complete());
    EXPECT_TRUE(streaming.response().body_streamed);
    EXPECT_EQ(streaming.response().body.size(), 2U);
    EXPECT_EQ(streamed_bytes, 4U);
}

TEST(Http1Parser, DetectsMalformedStatusHeadersAndPrematureClose)
{
    http1_response_parser<32U> parser;
    const char bad_status[] = "HTP/1.1 200 OK\r\n\r\n";
    EXPECT_EQ(parser.feed(bad_status, sizeof(bad_status) - 1U), castle::status::invalid_argument);
    EXPECT_EQ(parser.last_error(), error_code::malformed_status_line);
    parser.reset();
    const char bad_header[] = "HTTP/1.1 200 OK\r\nnot-a-header\r\n\r\n";
    EXPECT_EQ(parser.feed(bad_header, sizeof(bad_header) - 1U), castle::status::invalid_argument);
    EXPECT_EQ(parser.last_error(), error_code::invalid_header);
    parser.reset();
    const char partial[] = "HTTP/1.1 200 OK\r\nContent-Length: 10\r\n\r\nabc";
    EXPECT_EQ(parser.feed(partial, sizeof(partial) - 1U, true), castle::status::data_loss);
    EXPECT_EQ(parser.last_error(), error_code::connection_closed);
}

TEST(Http1Parser, RejectsMalformedChunkAndLineOverflow)
{
    http1_response_parser<32U, 8U, 32U, 64U, 128U, true> chunked;
    const char bad_chunk[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nx!\n0\r\n\r\n";
    EXPECT_EQ(chunked.feed(bad_chunk, sizeof(bad_chunk) - 1U), castle::status::invalid_argument);
    EXPECT_EQ(chunked.last_error(), error_code::invalid_chunk);
    http1_response_parser<32U, 8U, 32U, 64U, 16U> line_limited;
    const char long_line[] = "HTTP/1.1 204\r\nX-Long: 12345678901234567890\r\n\r\n";
    EXPECT_EQ(line_limited.feed(long_line, sizeof(long_line) - 1U), castle::status::out_of_range);
    EXPECT_EQ(line_limited.last_error(), error_code::header_line_limit);
}

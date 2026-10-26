#include <gtest/gtest.h>
#include <cstring>
#include <string>
#include "castle_ext/protocols/http/http1.hpp"

using namespace castle::protocols::http;
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

TEST(Http1Request, RejectsTransferEncodingAndControlCharactersInAuthority)
{
    basic_headers<4U, 32U, 64U> headers;
    http1_request<256U, 4U, 32U, 64U> request;
    EXPECT_EQ(request.build(method::get, "/", "peer\x01test", headers), castle::status::invalid_argument);
    ASSERT_EQ(headers.add("Transfer-Encoding", "chunked"), castle::status::ok);
    EXPECT_EQ(request.build(method::post, "/", "host:80", headers, "abc"), castle::status::invalid_argument);
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

TEST(Http1Parser, ValidatesFeedArgumentsAndTerminalStates)
{
    http1_response_parser<16U> parser;
    EXPECT_EQ(parser.feed(nullptr, 0U), castle::status::ok);
    EXPECT_EQ(parser.feed(nullptr, 3U), castle::status::invalid_argument);
    const char response[] = "HTTP/1.1 204 No Content\r\n\r\n";
    size_type consumed = 0U;
    EXPECT_EQ(parser.feed(response, sizeof(response) - 1U, false, &consumed), castle::status::ok);
    EXPECT_EQ(consumed, sizeof(response) - 1U);
    EXPECT_EQ(parser.feed(nullptr, 0U), castle::status::ok);
    EXPECT_EQ(parser.feed("x", 1U), castle::status::invalid_config);

    http1_response_parser<16U> failing;
    const char garbage[] = "HTP/1.1 200 OK\r\n";
    EXPECT_EQ(failing.feed(garbage, sizeof(garbage) - 1U), castle::status::invalid_argument);
    EXPECT_EQ(failing.feed("x", 1U), castle::status::invalid_argument);
}

TEST(Http1Parser, RejectsMalformedLinesAndStatusCodes)
{
    char const* bare_lf[] = {"\n", "AB\n"};
    for (char const* text : bare_lf)
    {
        http1_response_parser<16U> parser;
        EXPECT_EQ(parser.feed(text, std::strlen(text)), castle::status::invalid_argument) << text;
        EXPECT_EQ(parser.last_error(), error_code::malformed_message) << text;
    }
    char const* bad_status[] = {"HTTP/1.1 x00 OK\r\n", "HTTP/1.1 2x0 OK\r\n", "HTTP/1.1 20x OK\r\n",
                                "HTTP/1.1 2000\r\n", "HTTP/1.1 20\r\n", "HTTP/1.2 200 OK\r\n",
                                "HTTP/1.1 099 X\r\n", "HTTP/1.1 600 X\r\n"};
    for (char const* text : bad_status)
    {
        http1_response_parser<16U> parser;
        EXPECT_EQ(parser.feed(text, std::strlen(text)), castle::status::invalid_argument) << text;
        EXPECT_EQ(parser.last_error(), error_code::malformed_status_line) << text;
    }
}

TEST(Http1Parser, ReportsLineLimitWithConsumedBytes)
{
    http1_response_parser<32U, 8U, 32U, 64U, 16U> parser;
    const char long_line[] = "HTTP/1.1 200 OK-----------------------\r\n";
    size_type consumed = 0U;
    EXPECT_EQ(parser.feed(long_line, sizeof(long_line) - 1U, false, &consumed), castle::status::out_of_range);
    EXPECT_EQ(parser.last_error(), error_code::header_line_limit);
    EXPECT_EQ(consumed, 17U);
}

TEST(Http1Parser, ValidatesHeaderFieldsAndCapacity)
{
    const char bad_name[] = "HTTP/1.1 200 OK\r\nbad name: x\r\n\r\n";
    http1_response_parser<16U> bad_name_parser;
    EXPECT_EQ(bad_name_parser.feed(bad_name, sizeof(bad_name) - 1U), castle::status::invalid_argument);
    EXPECT_EQ(bad_name_parser.last_error(), error_code::invalid_header);

    const char control[] = "HTTP/1.1 200 OK\r\nX-A: a\x01" "b\r\n\r\n";
    http1_response_parser<16U> control_parser;
    EXPECT_EQ(control_parser.feed(control, sizeof(control) - 1U), castle::status::invalid_argument);
    EXPECT_EQ(control_parser.last_error(), error_code::invalid_header);

    std::string long_value = "HTTP/1.1 200 OK\r\nX-Long: " + std::string(300U, 'a') + "\r\n\r\n";
    http1_response_parser<16U> long_parser;
    EXPECT_EQ(long_parser.feed(long_value.data(), long_value.size()), castle::status::out_of_range);
    EXPECT_EQ(long_parser.last_error(), error_code::header_limit);

    const char two_fields[] = "HTTP/1.1 200 OK\r\nX-A: 1\r\nX-B: 2\r\n\r\n";
    http1_response_parser<16U, 1U> full_parser;
    EXPECT_EQ(full_parser.feed(two_fields, sizeof(two_fields) - 1U), castle::status::out_of_range);
    EXPECT_EQ(full_parser.last_error(), error_code::header_limit);
}

TEST(Http1Parser, ValidatesContentLengthTransferEncodingAndConnectionTokens)
{
    const char bad_length[] = "HTTP/1.1 200 OK\r\nContent-Length: abc\r\n\r\n";
    http1_response_parser<16U> bad_length_parser;
    EXPECT_EQ(bad_length_parser.feed(bad_length, sizeof(bad_length) - 1U), castle::status::invalid_argument);
    EXPECT_EQ(bad_length_parser.last_error(), error_code::malformed_message);

    const char mismatch[] = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\n";
    http1_response_parser<16U> mismatch_parser;
    EXPECT_EQ(mismatch_parser.feed(mismatch, sizeof(mismatch) - 1U), castle::status::invalid_argument);

    const char repeated[] = "HTTP/1.1 200 OK\r\nContent-Length: 1\r\nContent-Length: 1\r\n\r\nx";
    http1_response_parser<16U> repeated_parser;
    EXPECT_EQ(repeated_parser.feed(repeated, sizeof(repeated) - 1U), castle::status::ok);
    EXPECT_TRUE(repeated_parser.complete());

    const char gzip[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\n\r\n";
    http1_response_parser<16U> gzip_parser;
    EXPECT_EQ(gzip_parser.feed(gzip, sizeof(gzip) - 1U), castle::status::invalid_config);
    EXPECT_EQ(gzip_parser.last_error(), error_code::unsupported_feature);

    const char both[] = "HTTP/1.1 200 OK\r\nContent-Length: 2\r\nTransfer-Encoding: chunked\r\n\r\n";
    http1_response_parser<16U, 8U, 32U, 64U, 128U, true> both_parser;
    EXPECT_EQ(both_parser.feed(both, sizeof(both) - 1U), castle::status::invalid_argument);
    EXPECT_EQ(both_parser.last_error(), error_code::malformed_message);

    const char tokens[] = "HTTP/1.0 200 OK\r\nConnection: Upgrade\r\nConnection: keep-alive\r\nContent-Length: 0\r\n\r\n";
    http1_response_parser<16U> token_parser;
    EXPECT_EQ(token_parser.feed(tokens, sizeof(tokens) - 1U), castle::status::ok);
    EXPECT_TRUE(token_parser.response().keep_alive);
}

TEST(Http1Parser, CompletesNoBodyAndInterimResponses)
{
    const char interim[] = "HTTP/1.1 103 Early Hints\r\nLink: </a>\r\n\r\nHTTP/1.1 204 No Content\r\n\r\n";
    http1_response_parser<16U> parser;
    EXPECT_EQ(parser.feed(interim, sizeof(interim) - 1U), castle::status::ok);
    EXPECT_TRUE(parser.complete());
    EXPECT_EQ(parser.response().status_code, 204U);

    const char not_modified[] = "HTTP/1.1 304 Not Modified\r\nContent-Length: 10\r\n\r\n";
    http1_response_parser<16U> not_modified_parser;
    EXPECT_EQ(not_modified_parser.feed(not_modified, sizeof(not_modified) - 1U), castle::status::ok);
    EXPECT_TRUE(not_modified_parser.complete());
}

TEST(Http1Parser, DecodesChunkSizesExtensionsAndSplitChunks)
{
    using chunked_parser = http1_response_parser<64U, 8U, 32U, 64U, 128U, true>;
    std::string body(26U, 'z');
    std::string response = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n1A\r\n" + body + "\r\n0\r\n\r\n";
    chunked_parser upper;
    EXPECT_EQ(upper.feed(response.data(), response.size()), castle::status::ok);
    EXPECT_TRUE(upper.complete());
    EXPECT_EQ(upper.response().body.size(), 26U);

    const char extension[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2;name=value\r\nhi\r\n0\r\n\r\n";
    chunked_parser with_extension;
    EXPECT_EQ(with_extension.feed(extension, sizeof(extension) - 1U), castle::status::ok);
    EXPECT_EQ(with_extension.response().body.size(), 2U);

    const char first[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nh";
    const char second[] = "i\r\n0\r\n\r\n";
    chunked_parser split;
    EXPECT_EQ(split.feed(first, sizeof(first) - 1U), castle::status::ok);
    EXPECT_FALSE(split.complete());
    EXPECT_EQ(split.feed(second, sizeof(second) - 1U), castle::status::ok);
    EXPECT_TRUE(split.complete());
}

TEST(Http1Parser, RejectsMalformedChunkFraming)
{
    char const* bad_chunks[] = {"\r\nhi", "2g\r\nhi\r\n0\r\n\r\n", "FFFFFFFFFFFFFFFFF\r\n", "2\r\nhiX", "2\r\nhi\rX"};
    for (char const* chunks : bad_chunks)
    {
        std::string response = std::string("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n") + chunks;
        http1_response_parser<64U, 8U, 32U, 64U, 128U, true> parser;
        EXPECT_EQ(parser.feed(response.data(), response.size()), castle::status::invalid_argument) << chunks;
        EXPECT_EQ(parser.last_error(), error_code::invalid_chunk) << chunks;
    }
}

TEST(Http1Parser, ValidatesTrailersAndTheirCapacity)
{
    char const* bad_trailers[] = {"0\r\nbad name: x\r\n\r\n", "0\r\nContent-Length: 1\r\n\r\n",
                                  "0\r\nTransfer-Encoding: chunked\r\n\r\n", "0\r\nHost: x\r\n\r\n",
                                  "0\r\nConnection: close\r\n\r\n", "0\r\nTrailer: x\r\n\r\n",
                                  "0\r\nX-B: a\x01" "b\r\n\r\n"};
    for (char const* trailers : bad_trailers)
    {
        std::string response = std::string("HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n") + trailers;
        http1_response_parser<64U, 8U, 32U, 64U, 128U, true> parser;
        EXPECT_EQ(parser.feed(response.data(), response.size()), castle::status::invalid_argument) << trailers;
        EXPECT_EQ(parser.last_error(), error_code::invalid_chunk) << trailers;
    }

    const char full[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n0\r\nX-B: 2\r\n\r\n";
    http1_response_parser<64U, 1U, 32U, 64U, 128U, true> full_parser;
    EXPECT_EQ(full_parser.feed(full, sizeof(full) - 1U), castle::status::out_of_range);
    EXPECT_EQ(full_parser.last_error(), error_code::header_limit);
}

TEST(Http1Parser, LimitsCloseDelimitedBodyWithoutCallbackAndNullCallbacks)
{
    http1_response_parser<2U> parser;
    const char close_body[] = "HTTP/1.1 200 OK\r\n\r\nhello";
    EXPECT_EQ(parser.feed(close_body, sizeof(close_body) - 1U), castle::status::out_of_range);
    EXPECT_EQ(parser.last_error(), error_code::body_limit);
    EXPECT_EQ(parser.set_body_callback(static_cast<void (*)(string_view)>(nullptr)), castle::status::invalid_callback);

    http1_response_parser<16U> closed;
    const char complete_at_close[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    EXPECT_EQ(closed.feed(complete_at_close, sizeof(complete_at_close) - 1U, true), castle::status::ok);
    EXPECT_TRUE(closed.complete());
}

TEST(Http1Request, HonorsCallerHeadersAndFramingRules)
{
    basic_headers<4U, 32U, 64U> headers;
    http1_request<256U, 4U, 32U, 64U> request;
    ASSERT_EQ(headers.add("Host", "custom.test"), castle::status::ok);
    EXPECT_EQ(request.build(method::get, "/", "ignored:80", headers), castle::status::ok);
    EXPECT_NE(request.data().find("Host: custom.test"), string_view::npos);
    EXPECT_EQ(request.data().find("ignored"), string_view::npos);

    basic_headers<4U, 32U, 64U> bad_length;
    ASSERT_EQ(bad_length.add("Content-Length", "abc"), castle::status::ok);
    EXPECT_EQ(request.build(method::post, "/", "host:80", bad_length, "abc"), castle::status::invalid_argument);

    basic_headers<4U, 32U, 64U> matching;
    ASSERT_EQ(matching.add("Content-Length", "3"), castle::status::ok);
    EXPECT_EQ(request.build(method::post, "/", "host:80", matching, "abc"), castle::status::ok);
    EXPECT_NE(request.data().find("Content-Length: 3"), string_view::npos);

    basic_headers<4U, 32U, 64U> closing;
    ASSERT_EQ(closing.add("Connection", "close"), castle::status::ok);
    EXPECT_EQ(request.build(method::get, "/", "host:80", closing), castle::status::ok);
    EXPECT_NE(request.data().find("Connection: close"), string_view::npos);

    basic_headers<4U, 32U, 64U> none;
    EXPECT_EQ(request.build(method::get, "/", "host:80", none, string_view(), string_view(), false), castle::status::ok);
    EXPECT_NE(request.data().find("Connection: close"), string_view::npos);
    EXPECT_EQ(request.build(method::put, "/", "host:80", none), castle::status::ok);
    EXPECT_NE(request.data().find("Content-Length: 0"), string_view::npos);
    EXPECT_EQ(request.build(method::patch, "/", "host:80", none), castle::status::ok);
    EXPECT_NE(request.data().find("Content-Length: 0"), string_view::npos);
    EXPECT_EQ(request.build(method::get, "/", "host:80", none, "x"), castle::status::ok);
    EXPECT_NE(request.data().find("Content-Length: 1"), string_view::npos);

    basic_headers<4U, 32U, 64U> pseudo;
    ASSERT_EQ(pseudo.add(":scheme", "http"), castle::status::ok);
    EXPECT_EQ(request.build(method::get, "/", "host:80", pseudo), castle::status::invalid_argument);
}

TEST(Http1Request, SweepsEveryCapacityBoundary)
{
    basic_headers<4U, 32U, 64U> headers;
    http1_request<64U, 4U, 32U, 64U> request;
    std::string path(80U, 'p');
    std::string token(80U, 'T');
    std::string body(80U, 'b');
    for (size_type n = 1U; n <= 80U; ++n)
    {
        castle::status result = request.build(method::get, string_view(path.data(), n), "h", headers);
        EXPECT_TRUE(result == castle::status::ok || result == castle::status::full) << n;
        result = request.build(method::custom, "/", "h", headers, string_view(), string_view(token.data(), n));
        EXPECT_TRUE(result == castle::status::ok || result == castle::status::full) << n;
        result = request.build(method::get, "/", string_view(path.data(), n), headers);
        EXPECT_TRUE(result == castle::status::ok || result == castle::status::full) << n;
    }
    for (size_type n = 0U; n <= 80U; ++n)
    {
        castle::status result = request.build(method::post, "/", "h", headers, string_view(body.data(), n));
        EXPECT_TRUE(result == castle::status::ok || result == castle::status::full) << n;
    }
}

TEST(Http1Request, SweepsHeaderAndFramingBoundaries)
{
    http1_request<64U, 4U, 32U, 64U> request;
    for (size_type n = 0U; n <= 60U; ++n)
    {
        basic_headers<4U, 32U, 64U> headers;
        std::string pad(n, 'v');
        ASSERT_EQ(headers.add("X-Pad", string_view(pad.data(), pad.size())), castle::status::ok);
        castle::status result = request.build(method::post, "/", "h", headers, "x");
        EXPECT_TRUE(result == castle::status::ok || result == castle::status::full) << n;
    }
    basic_headers<4U, 32U, 64U> closing;
    ASSERT_EQ(closing.add("Connection", "close"), castle::status::ok);
    std::string body(80U, 'b');
    for (size_type n = 0U; n <= 80U; ++n)
    {
        castle::status result = request.build(method::post, "/", "h", closing, string_view(body.data(), n));
        EXPECT_TRUE(result == castle::status::ok || result == castle::status::full) << n;
    }
}

TEST(Http1Parser, TrimsOptionalWhitespaceAroundFieldValues)
{
    const char response[] = "HTTP/1.1 200 OK\r\nX-A:   \r\nX-B:  a \t \r\nContent-Length:  \t5 \t \r\n\r\nhello";
    http1_response_parser<16U> parser;
    EXPECT_EQ(parser.feed(response, sizeof(response) - 1U), castle::status::ok);
    EXPECT_EQ(parser.response().body.size(), 5U);
}

TEST(Http1Parser, AcceptsLowercaseChunkDigits)
{
    const char response[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\na\r\n0123456789\r\n0\r\n\r\n";
    http1_response_parser<64U, 8U, 32U, 64U, 128U, true> parser;
    EXPECT_EQ(parser.feed(response, sizeof(response) - 1U), castle::status::ok);
    EXPECT_EQ(parser.response().body.size(), 10U);
}

TEST(Http1Parser, RejectsOutOfRangeStatusDigits)
{
    char const* lines[] = {"HTTP/1.1 /00 X\r\n\r\n", "HTTP/1.1 :00 X\r\n\r\n", "HTTP/1.1 2/0 X\r\n\r\n",
                           "HTTP/1.1 2:0 X\r\n\r\n", "HTTP/1.1 20/ X\r\n\r\n", "HTTP/1.1 20: X\r\n\r\n"};
    for (char const* text : lines)
    {
        http1_response_parser<16U> parser;
        EXPECT_EQ(parser.feed(text, std::strlen(text)), castle::status::invalid_argument) << text;
    }
}

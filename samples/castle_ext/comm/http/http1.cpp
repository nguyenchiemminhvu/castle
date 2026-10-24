#include "sample_support.hpp"
#include "castle_ext/comm/http/http1.hpp"

using namespace castle::comm::http;

int main()
{
    basic_headers<8U, 32U, 96U> request_headers;
    CASTLE_SAMPLE_CHECK(request_headers.add("accept", "text/plain") == castle::status::ok);
    http1_request<512U, 8U, 32U, 96U> request;
    CASTLE_SAMPLE_CHECK(request.build(method::get, "/health", "device.local:80", request_headers) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(request.data().find("Host: device.local:80") != string_view::npos);

    http1_response_parser<64U> parser;
    const char response[] = "HTTP/1.1 200 OK\r\nContent-Length: 5\r\n\r\nhello";
    CASTLE_SAMPLE_CHECK(parser.feed(response, sizeof(response) - 1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(parser.complete());
    CASTLE_SAMPLE_CHECK(parser.response().status_code == 200U);
    CASTLE_SAMPLE_CHECK(parser.response().body.size() == 5U);

    http1_response_parser<64U, 8U, 32U, 96U, 384U, true> chunked;
    const char chunked_response[] = "HTTP/1.1 200 OK\r\nTransfer-Encoding: chunked\r\n\r\n2\r\nhi\r\n0\r\n\r\n";
    CASTLE_SAMPLE_CHECK(chunked.feed(chunked_response, sizeof(chunked_response) - 1U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(chunked.complete());
    CASTLE_SAMPLE_CHECK(chunked.response().body.size() == 2U);
    return 0;
}

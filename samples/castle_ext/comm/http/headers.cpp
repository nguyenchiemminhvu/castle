#include "sample_support.hpp"
#include "castle_ext/comm/http/headers.hpp"

using namespace castle::comm::http;

int main()
{
    basic_headers<2U, 16U, 32U> headers;
    CASTLE_SAMPLE_CHECK(headers.empty());
    CASTLE_SAMPLE_CHECK(headers.add("accept", "application/json") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(headers.set("user-agent", "castle/1") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(headers.full());
    CASTLE_SAMPLE_CHECK(headers.contains("ACCEPT"));
    CASTLE_SAMPLE_CHECK(ascii_iequal(headers.get("accept"), string_view("application/json")));
    CASTLE_SAMPLE_CHECK(headers.add("x-overflow", "x") == castle::status::full);
    CASTLE_SAMPLE_CHECK(headers.remove("accept"));
    CASTLE_SAMPLE_CHECK(headers.add(":status", "200") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(headers.add("bad name", "x") == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(headers.add("x-safe", "bad\r\nvalue") == castle::status::invalid_argument);
    return 0;
}

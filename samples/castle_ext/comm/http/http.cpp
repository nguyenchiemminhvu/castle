#include "sample_support.hpp"
#include "castle_ext/comm/http/http.hpp"

int main()
{
    castle::comm::http::basic_headers<4U, 32U, 96U> headers;
    CASTLE_SAMPLE_CHECK(headers.add("accept", "*/*") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(headers.contains("Accept"));
    CASTLE_SAMPLE_CHECK(castle::comm::http::method_token(castle::comm::http::method::options).size() == 7U);
    return 0;
}

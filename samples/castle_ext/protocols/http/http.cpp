#include "sample_support.hpp"
#include "castle_ext/protocols/http/http.hpp"

int main()
{
    castle::protocols::http::basic_headers<4U, 32U, 96U> headers;
    CASTLE_SAMPLE_CHECK(headers.add("accept", "*/*") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(headers.contains("Accept"));
    CASTLE_SAMPLE_CHECK(castle::protocols::http::method_token(castle::protocols::http::method::options).size() == 7U);
    return 0;
}

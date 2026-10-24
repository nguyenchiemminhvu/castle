#include "sample_support.hpp"
#include "castle_ext/protocols/http/types.hpp"

using namespace castle::protocols::http;

int main()
{
    CASTLE_SAMPLE_CHECK(method_token(method::get).size() == 3U);
    CASTLE_SAMPLE_CHECK(ascii_iequal(method_token(method::get), string_view("get")));
    CASTLE_SAMPLE_CHECK(ascii_lowercase(string_view("content-type")));
    CASTLE_SAMPLE_CHECK(!ascii_lowercase(string_view("Content-Type")));
    CASTLE_SAMPLE_CHECK(to_status(error_code::body_limit) == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(to_status(error_code::connection_closed) == castle::status::data_loss);
    response_view response{};
    response.protocol = version::http_2;
    response.status_code = 200U;
    CASTLE_SAMPLE_CHECK(response.protocol == version::http_2 && response.status_code == 200U);
    CASTLE_SAMPLE_CHECK(method_token(method::custom).empty());
    return 0;
}

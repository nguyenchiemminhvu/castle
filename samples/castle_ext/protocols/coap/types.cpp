#include "sample_support.hpp"
#include "castle_ext/protocols/coap/types.hpp"

int main()
{
    using namespace castle::protocols::coap;
    message<> request;
    request.code_value = static_cast<uint8_t>(code::get);
    const uint8_t token[] = {0x12U, 0x34U};
    CASTLE_SAMPLE_CHECK(request.set_token(token, sizeof(token)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(append_uri_path(request, "/devices/led/state") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(append_uri_query(request, "verbose=1&format=text") == castle::status::ok);
    CASTLE_SAMPLE_CHECK(uri_path_matches(request, "devices/led/state"));
    CASTLE_SAMPLE_CHECK(request.option_count == 5U);
    CASTLE_SAMPLE_CHECK(is_critical_option(11U));
    CASTLE_SAMPLE_CHECK(is_unsafe_option(11U));
    CASTLE_SAMPLE_CHECK(is_no_cache_key_option(28U));
    CASTLE_SAMPLE_CHECK(!is_no_cache_key_option(14U));
    return 0;
}

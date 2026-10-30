#include "sample_support.hpp"
#include "castle_ext/protocols/coap/codec.hpp"

int main()
{
    using namespace castle::protocols::coap;
    message<> request;
    request.type = message_type::confirmable;
    request.code_value = static_cast<uint8_t>(code::get);
    request.message_id = 0x1234U;
    const uint8_t token[] = {0xAAU, 0x55U};
    CASTLE_SAMPLE_CHECK(request.set_token(token, sizeof(token)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(append_uri_path(request, "well-known/core") == castle::status::ok);
    uint8_t wire[256U]{};
    castle::size_type wire_size = 0U;
    CASTLE_SAMPLE_CHECK(encode_message(request, wire, sizeof(wire), wire_size) == castle::status::ok);
    message<> parsed;
    CASTLE_SAMPLE_CHECK(decode_message(wire, wire_size, parsed) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(parsed.message_id == request.message_id);
    CASTLE_SAMPLE_CHECK(parsed.token_length == 2U && parsed.token[1U] == 0x55U);
    CASTLE_SAMPLE_CHECK(uri_path_matches(parsed, "well-known/core"));
    return 0;
}

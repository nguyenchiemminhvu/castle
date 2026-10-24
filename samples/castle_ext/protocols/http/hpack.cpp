#include "sample_support.hpp"
#include "castle_ext/protocols/http/hpack.hpp"
#include "castle_ext/protocols/http/headers.hpp"

using namespace castle::protocols::http;

int main()
{
    const uint8_t indexed_get[] = {0x82U};
    basic_headers<8U, 64U, 128U> decoded;
    hpack_decoder<0U, 128U> decoder;
    CASTLE_SAMPLE_CHECK(decoder.decode_block(indexed_get, sizeof(indexed_get), decoded) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(ascii_iequal(decoded.get(":method"), string_view("GET")));

    uint8_t encoded[96U]{};
    castle::size_type used = 0U;
    CASTLE_SAMPLE_CHECK(hpack_encoder::encode_literal("x-mode", "compact", encoded, sizeof(encoded), used) == castle::status::ok);
    basic_headers<8U, 64U, 128U> round_trip;
    CASTLE_SAMPLE_CHECK(decoder.decode_block(encoded, used, round_trip) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(ascii_iequal(round_trip.get("x-mode"), string_view("compact")));
    CASTLE_SAMPLE_CHECK(decoder.decode_block(nullptr, 0U, round_trip) == castle::status::ok);
    return 0;
}

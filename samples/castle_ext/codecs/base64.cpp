#include <assert.h>
#include <stdint.h>
#include "castle_ext/codecs/base64.hpp"

int main()
{
    const uint8_t input[] = {'f','o','o','b','a','r'};
    char encoded[32] = {};
    uint8_t decoded[16] = {};
    size_t encoded_size = 0U;
    size_t decoded_size = 0U;

    assert(castle::codecs::base64_encoded_size(sizeof(input)) == 8U);
    assert(castle::codecs::base64_decoded_size(8U) == 6U);
    assert(castle::codecs::base64_encode(input, sizeof(input), encoded, sizeof(encoded), encoded_size) == castle::status::ok);
    assert(encoded_size == 8U);
    assert(encoded[0] == 'Z' && encoded[7] == 'y');
    assert(castle::codecs::base64_decode(encoded, encoded_size, decoded, sizeof(decoded), decoded_size) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(input); ++i) assert(decoded[i] == input[i]);

    assert(castle::codecs::base64_encode(input, 2U, encoded, sizeof(encoded), encoded_size, false, true) == castle::status::ok);
    assert(encoded_size == 3U);
    assert(castle::codecs::base64_decode(encoded, encoded_size, decoded, sizeof(decoded), decoded_size, true) == castle::status::ok);
    assert(decoded_size == 2U && decoded[0] == 'f' && decoded[1] == 'o');
    return 0;
}

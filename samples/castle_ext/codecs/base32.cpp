#include <assert.h>
#include <stdint.h>
#include "castle_ext/codecs/base32.hpp"

int main()
{
    const uint8_t input[] = {'f','o','o','b','a','r'};
    char encoded[32] = {};
    uint8_t decoded[16] = {};
    size_t encoded_size = 0U;
    size_t decoded_size = 0U;

    assert(castle::codecs::base32_encoded_size(sizeof(input)) == 16U);
    assert(castle::codecs::base32_encoded_size(sizeof(input), false) == 10U);
    assert(castle::codecs::base32_decode(nullptr, 0U, decoded, sizeof(decoded), decoded_size) == castle::status::ok);
    assert(castle::codecs::base32_encode(input, sizeof(input), encoded, sizeof(encoded), encoded_size) == castle::status::ok);
    assert(encoded_size == 16U);
    assert(encoded[0] == 'M' && encoded[15] == '=');
    assert(castle::codecs::base32_decode(encoded, encoded_size, decoded, sizeof(decoded), decoded_size) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(input); ++i) assert(decoded[i] == input[i]);

    assert(castle::codecs::base32_encode(input, sizeof(input), encoded, sizeof(encoded), encoded_size, false) == castle::status::ok);
    assert(encoded_size == 10U);
    assert(castle::codecs::base32_decode(encoded, encoded_size, decoded, sizeof(decoded), decoded_size) == castle::status::ok);
    return 0;
}

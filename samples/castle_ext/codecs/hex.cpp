#include <assert.h>
#include <stdint.h>
#include "castle_ext/codecs/hex.hpp"

int main()
{
    const uint8_t input[] = {0x00U,0x12U,0xABU,0xFFU};
    char encoded[16] = {};
    uint8_t decoded[8] = {};
    size_t encoded_size = 0U;
    size_t decoded_size = 0U;

    assert(castle::codecs::hex_encoded_size(sizeof(input)) == 8U);
    assert(castle::codecs::hex_encode(input, sizeof(input), encoded, sizeof(encoded), encoded_size) == castle::status::ok);
    assert(encoded_size == 8U && encoded[0] == '0' && encoded[7] == 'f');
    assert(castle::codecs::hex_encode(input, sizeof(input), encoded, sizeof(encoded), encoded_size, true) == castle::status::ok);
    assert(encoded[4] == 'A' && encoded[7] == 'F');
    assert(castle::codecs::hex_decode(encoded, encoded_size, decoded, sizeof(decoded), decoded_size) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(input); ++i) assert(decoded[i] == input[i]);
    return 0;
}

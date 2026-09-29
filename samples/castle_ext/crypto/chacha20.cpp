#include <assert.h>
#include <stdint.h>
#include "castle_ext/crypto/chacha20.hpp"

int main()
{
    uint8_t key[32];
    uint8_t nonce[12] = {0U};
    for (size_t i = 0U; i < sizeof(key); ++i) key[i] = static_cast<uint8_t>(i);
    nonce[3] = 9U;
    nonce[7] = 0x4AU;
    castle::crypto::chacha20 cipher(key, nonce, 1U);
    assert(cipher.counter() == 1U);

    uint8_t data[64] = {0U};
    assert(cipher.crypt(data, 13U) == castle::status::ok);
    assert(cipher.crypt(data + 13U, sizeof(data) - 13U) == castle::status::ok);
    assert(data[0] == 0x10U && data[1] == 0xF1U && data[2] == 0xE7U);

    assert(cipher.reset(key, nonce, 1U) == castle::status::ok);
    uint8_t round_trip[64] = {0U};
    assert(cipher.crypt(round_trip, sizeof(round_trip)) == castle::status::ok);
    assert(cipher.reset(key, nonce, 1U) == castle::status::ok);
    assert(cipher.crypt(round_trip, sizeof(round_trip)) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(round_trip); ++i) assert(round_trip[i] == 0U);
    assert(cipher.set_counter(7U) == castle::status::ok);
    assert(cipher.counter() == 7U);
    return 0;
}

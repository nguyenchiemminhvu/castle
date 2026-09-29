#include <assert.h>
#include <stdint.h>
#include "castle_ext/crypto/hash/sha512.hpp"

int main()
{
    const uint8_t message[] = {'a','b','c'};
    castle::crypto::hash::sha512 hash;
    assert(hash.update(message, 1U) == castle::status::ok);
    assert(hash.update(message + 1U, 2U) == castle::status::ok);
    auto digest = hash.digest();
    assert(digest[0] == 0xDDU && digest[1] == 0xAFU && digest[2] == 0x35U);
    uint8_t output[64];
    assert(hash.final(output, sizeof(output)) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(output); ++i) assert(output[i] == digest[i]);
    auto one_shot = castle::crypto::hash::sha512::calculate(message, sizeof(message));
    for (size_t i = 0U; i < digest.size(); ++i) assert(one_shot[i] == digest[i]);
    hash.reset();
    assert(hash.digest()[0] == 0xCFU);
    return 0;
}

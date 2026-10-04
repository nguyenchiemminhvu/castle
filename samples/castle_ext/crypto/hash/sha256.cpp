#include <assert.h>
#include <stdint.h>
#include "castle_ext/crypto/hash/sha256.hpp"

int main()
{
    const uint8_t message[] = {'a','b','c'};
    const uint8_t expected_first = 0xBAU;
    castle::crypto::hash::sha256 hash;
    assert(hash.update(message, sizeof(message)) == castle::status::ok);
    auto digest = hash.digest();
    assert(digest[0] == expected_first);
    uint8_t output[32];
    assert(hash.final(output, sizeof(output)) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(output); ++i) assert(output[i] == digest[i]);
    auto one_shot = castle::crypto::hash::sha256::calculate(message, sizeof(message));
    for (size_t i = 0U; i < digest.size(); ++i) assert(one_shot[i] == digest[i]);
    hash.reset();
    assert(hash.digest()[0] == 0xE3U);
    return 0;
}

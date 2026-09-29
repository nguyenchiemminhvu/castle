#include <assert.h>
#include <stdint.h>
#include "castle_ext/crypto/hash/md5.hpp"

int main()
{
    const uint8_t message[] = {'a','b','c'};
    const uint8_t expected[16] = {0x90U,0x01U,0x50U,0x98U,0x3cU,0xd2U,0x4fU,0xb0U,0xd6U,0x96U,0x3fU,0x7dU,0x28U,0xe1U,0x7fU,0x72U};
    castle::crypto::hash::md5 hash;
    assert(hash.update(message, 1U) == castle::status::ok);
    assert(hash.update(message + 1U, 2U) == castle::status::ok);
    auto digest = hash.digest();
    for (size_t i = 0U; i < sizeof(expected); ++i) assert(digest[i] == expected[i]);
    uint8_t output[16];
    assert(hash.final(output, sizeof(output)) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(expected); ++i) assert(output[i] == expected[i]);
    auto one_shot = castle::crypto::hash::md5::calculate(message, sizeof(message));
    for (size_t i = 0U; i < digest.size(); ++i) assert(one_shot[i] == digest[i]);
    hash.reset();
    assert(hash.digest()[0] == 0xD4U);
    return 0;
}

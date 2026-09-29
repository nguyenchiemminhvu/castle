#include <assert.h>
#include <stdint.h>
#include "castle_ext/crypto/sm4.hpp"

int main()
{
    const uint8_t key[16] = {
        0x01U,0x23U,0x45U,0x67U,0x89U,0xABU,0xCDU,0xEFU,
        0xFEU,0xDCU,0xBAU,0x98U,0x76U,0x54U,0x32U,0x10U
    };
    const uint8_t plaintext[16] = {
        0x01U,0x23U,0x45U,0x67U,0x89U,0xABU,0xCDU,0xEFU,
        0xFEU,0xDCU,0xBAU,0x98U,0x76U,0x54U,0x32U,0x10U
    };
    const uint8_t expected[16] = {
        0x68U,0x1EU,0xDFU,0x34U,0xD2U,0x06U,0x96U,0x5EU,
        0x86U,0xB3U,0xE9U,0x4FU,0x53U,0x6EU,0x42U,0x46U
    };

    castle::crypto::sm4 cipher;
    assert(cipher.set_key(key) == castle::status::ok);

    uint8_t encrypted[16];
    uint8_t decrypted[16];
    cipher.encrypt_block(plaintext, encrypted);
    for (size_t i = 0U; i < sizeof(expected); ++i) assert(encrypted[i] == expected[i]);

    cipher.decrypt_block(encrypted, decrypted);
    for (size_t i = 0U; i < sizeof(plaintext); ++i) assert(decrypted[i] == plaintext[i]);

    uint8_t in_place[16];
    for (size_t i = 0U; i < sizeof(in_place); ++i) in_place[i] = plaintext[i];
    cipher.encrypt_block(in_place, in_place);
    cipher.decrypt_block(in_place, in_place);
    for (size_t i = 0U; i < sizeof(in_place); ++i) assert(in_place[i] == plaintext[i]);

    uint8_t message[37];
    uint8_t original[37];
    uint8_t encrypt_counter[16] = {0U};
    uint8_t decrypt_counter[16] = {0U};
    for (size_t i = 0U; i < sizeof(message); ++i)
    {
        message[i] = static_cast<uint8_t>(i);
        original[i] = message[i];
    }
    assert(cipher.crypt_ctr(message, sizeof(message), encrypt_counter) == castle::status::ok);
    assert(cipher.crypt_ctr(message, sizeof(message), decrypt_counter) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(message); ++i) assert(message[i] == original[i]);

    return 0;
}

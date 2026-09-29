#include <assert.h>
#include <stdint.h>
#include "castle_ext/crypto/aes.hpp"

int main()
{
    const uint8_t key[16] = {0x00U,0x01U,0x02U,0x03U,0x04U,0x05U,0x06U,0x07U,0x08U,0x09U,0x0AU,0x0BU,0x0CU,0x0DU,0x0EU,0x0FU};
    const uint8_t plain[16] = {0x00U,0x11U,0x22U,0x33U,0x44U,0x55U,0x66U,0x77U,0x88U,0x99U,0xAAU,0xBBU,0xCCU,0xDDU,0xEEU,0xFFU};
    const uint8_t expected[16] = {0x69U,0xC4U,0xE0U,0xD8U,0x6AU,0x7BU,0x04U,0x30U,0xD8U,0xCDU,0xB7U,0x80U,0x70U,0xB4U,0xC5U,0x5AU};
    castle::crypto::aes<128> cipher;
    assert(cipher.set_key(key) == castle::status::ok);
    uint8_t encrypted[16];
    uint8_t decrypted[16];
    cipher.encrypt_block(plain, encrypted);
    for (size_t i = 0U; i < sizeof(expected); ++i) assert(encrypted[i] == expected[i]);
    cipher.decrypt_block(encrypted, decrypted);
    for (size_t i = 0U; i < sizeof(plain); ++i) assert(decrypted[i] == plain[i]);

    uint8_t ctr_block[16] = {0U};
    uint8_t data[] = {'C','a','s','t','l','e'};
    uint8_t original[sizeof(data)];
    for (size_t i = 0U; i < sizeof(data); ++i) original[i] = data[i];
    assert(cipher.crypt_ctr(data, sizeof(data), ctr_block) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(ctr_block); ++i) assert(ctr_block[i] == 0U || i == 15U);
    uint8_t ctr_block_2[16] = {0U};
    assert(cipher.crypt_ctr(data, sizeof(data), ctr_block_2) == castle::status::ok);
    for (size_t i = 0U; i < sizeof(data); ++i) assert(data[i] == original[i]);
    return 0;
}

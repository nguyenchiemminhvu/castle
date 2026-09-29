#include "../test_support.hpp"
#include "castle_ext/crypto/aes.hpp"

namespace
{
const uint8_t plaintext[16] = {
    0x00U,0x11U,0x22U,0x33U,0x44U,0x55U,0x66U,0x77U,
    0x88U,0x99U,0xaaU,0xbbU,0xccU,0xddU,0xeeU,0xffU
};
const uint8_t key128[16] = {0x00U,0x01U,0x02U,0x03U,0x04U,0x05U,0x06U,0x07U,0x08U,0x09U,0x0aU,0x0bU,0x0cU,0x0dU,0x0eU,0x0fU};
const uint8_t key192[24] = {0x00U,0x01U,0x02U,0x03U,0x04U,0x05U,0x06U,0x07U,0x08U,0x09U,0x0aU,0x0bU,0x0cU,0x0dU,0x0eU,0x0fU,0x10U,0x11U,0x12U,0x13U,0x14U,0x15U,0x16U,0x17U};
const uint8_t key256[32] = {0x00U,0x01U,0x02U,0x03U,0x04U,0x05U,0x06U,0x07U,0x08U,0x09U,0x0aU,0x0bU,0x0cU,0x0dU,0x0eU,0x0fU,0x10U,0x11U,0x12U,0x13U,0x14U,0x15U,0x16U,0x17U,0x18U,0x19U,0x1aU,0x1bU,0x1cU,0x1dU,0x1eU,0x1fU};
}

TEST(CastleExtAes, KnownBlockVectors)
{
    const uint8_t expected128[16] = {0x69U,0xc4U,0xe0U,0xd8U,0x6aU,0x7bU,0x04U,0x30U,0xd8U,0xcdU,0xb7U,0x80U,0x70U,0xb4U,0xc5U,0x5aU};
    const uint8_t expected192[16] = {0xddU,0xa9U,0x7cU,0xa4U,0x86U,0x4cU,0xdfU,0xe0U,0x6eU,0xafU,0x70U,0xa0U,0xecU,0x0dU,0x71U,0x91U};
    const uint8_t expected256[16] = {0x8eU,0xa2U,0xb7U,0xcaU,0x51U,0x67U,0x45U,0xbfU,0xeaU,0xfcU,0x49U,0x90U,0x4bU,0x49U,0x60U,0x89U};
    uint8_t encrypted[16];
    uint8_t decrypted[16];

    castle::crypto::aes<128> a128(key128);
    EXPECT_EQ(a128.set_key(key128), castle::status::ok);
    a128.encrypt_block(plaintext, encrypted);
    expect_bytes(encrypted, sizeof(encrypted), expected128, sizeof(expected128));
    a128.decrypt_block(encrypted, decrypted);
    expect_bytes(decrypted, sizeof(decrypted), plaintext, sizeof(plaintext));
    EXPECT_EQ(a128.set_key(nullptr), castle::status::invalid_argument);

    castle::crypto::aes<192> a192(key192);
    a192.encrypt_block(plaintext, encrypted);
    expect_bytes(encrypted, sizeof(encrypted), expected192, sizeof(expected192));
    a192.decrypt_block(encrypted, decrypted);
    expect_bytes(decrypted, sizeof(decrypted), plaintext, sizeof(plaintext));

    castle::crypto::aes<256> a256(key256);
    a256.encrypt_block(plaintext, encrypted);
    expect_bytes(encrypted, sizeof(encrypted), expected256, sizeof(expected256));
    a256.decrypt_block(encrypted, decrypted);
    expect_bytes(decrypted, sizeof(decrypted), plaintext, sizeof(plaintext));
}

TEST(CastleExtAes, CtrRoundTripAndNullBranches)
{
    uint8_t counter[16] = {0U};
    const uint8_t original[] = "Castle AES CTR test data";
    uint8_t encrypted[sizeof(original)];
    uint8_t decrypted[sizeof(original)];
    for (size_t i = 0U; i < sizeof(original); ++i) encrypted[i] = original[i];

    castle::crypto::aes<128> aes(key128);
    EXPECT_EQ(aes.crypt_ctr(encrypted, sizeof(original), counter), castle::status::ok);
    uint8_t counter_for_decrypt[16] = {0U};
    for (size_t i = 0U; i < sizeof(original); ++i) decrypted[i] = encrypted[i];
    EXPECT_EQ(aes.crypt_ctr(decrypted, sizeof(original), counter_for_decrypt), castle::status::ok);
    expect_bytes(decrypted, sizeof(original), original, sizeof(original));

    EXPECT_EQ(aes.crypt_ctr(nullptr, 0U, counter), castle::status::ok);
    aes.encrypt_block(nullptr, encrypted);
    aes.decrypt_block(nullptr, decrypted);
    EXPECT_EQ(aes.crypt_ctr(nullptr, 1U, counter), castle::status::invalid_argument);
    EXPECT_EQ(aes.crypt_ctr(encrypted, 0U, nullptr), castle::status::invalid_argument);

    uint8_t max_counter[16] = {0U};
    max_counter[12] = 0xFFU;
    max_counter[13] = 0xFFU;
    max_counter[14] = 0xFFU;
    max_counter[15] = 0xFFU;
    uint8_t untouched[1] = {0xA5U};
    EXPECT_EQ(aes.crypt_ctr(untouched, sizeof(untouched), max_counter), castle::status::out_of_range);
    EXPECT_EQ(untouched[0], 0xA5U);
    EXPECT_EQ(max_counter[15], 0xFFU);
}

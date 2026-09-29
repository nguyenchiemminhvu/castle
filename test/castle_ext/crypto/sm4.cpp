#include "../test_support.hpp"
#include "castle_ext/crypto/sm4.hpp"

namespace
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
}

TEST(CastleExtSm4, KnownVectorAndRoundTrip)
{
    castle::crypto::sm4 cipher(key);
    uint8_t encrypted[16];
    uint8_t decrypted[16];

    EXPECT_EQ(decltype(cipher)::block_size, 16U);
    EXPECT_EQ(decltype(cipher)::key_size, 16U);
    EXPECT_EQ(decltype(cipher)::rounds, 32U);
    EXPECT_EQ(cipher.set_key(key), castle::status::ok);

    cipher.encrypt_block(plaintext, encrypted);
    expect_bytes(encrypted, sizeof(encrypted), expected, sizeof(expected));

    cipher.decrypt_block(encrypted, decrypted);
    expect_bytes(decrypted, sizeof(decrypted), plaintext, sizeof(plaintext));

    EXPECT_EQ(cipher.set_key(nullptr), castle::status::invalid_argument);
}

TEST(CastleExtSm4, NullAndInPlaceBranches)
{
    castle::crypto::sm4 cipher;
    uint8_t block[16];
    for (size_t i = 0U; i < sizeof(block); ++i) block[i] = static_cast<uint8_t>(i);
    const uint8_t original[16] = {
        0U,1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U,12U,13U,14U,15U
    };

    cipher.encrypt_block(block, block);
    cipher.decrypt_block(block, block);
    expect_bytes(block, sizeof(block), original, sizeof(original));

    cipher.encrypt_block(nullptr, block);
    cipher.decrypt_block(block, nullptr);

    castle::crypto::sm4 null_key(static_cast<const uint8_t*>(nullptr));
    null_key.encrypt_block(block, block);
    null_key.decrypt_block(block, block);
    expect_bytes(block, sizeof(block), original, sizeof(original));
}

TEST(CastleExtSm4, CtrKnownVectorVariableLengthAndCounterBounds)
{
    castle::crypto::sm4 cipher(key);
    uint8_t counter[16];
    for (size_t i = 0U; i < sizeof(counter); ++i) counter[i] = plaintext[i];
    uint8_t zero_block[16] = {0U};

    EXPECT_EQ(cipher.crypt_ctr(zero_block, sizeof(zero_block), counter), castle::status::ok);
    expect_bytes(zero_block, sizeof(zero_block), expected, sizeof(expected));
    EXPECT_EQ(counter[15], 0x11U);

    uint8_t original[37];
    uint8_t data[37];
    uint8_t encrypt_counter[16] = {0U};
    uint8_t decrypt_counter[16] = {0U};
    encrypt_counter[0] = 0xA5U;
    decrypt_counter[0] = 0xA5U;
    for (size_t i = 0U; i < sizeof(original); ++i)
    {
        original[i] = static_cast<uint8_t>(i * 7U);
        data[i] = original[i];
    }

    EXPECT_EQ(cipher.crypt_ctr(data, sizeof(data), encrypt_counter), castle::status::ok);
    EXPECT_EQ(cipher.crypt_ctr(data, sizeof(data), decrypt_counter), castle::status::ok);
    expect_bytes(data, sizeof(data), original, sizeof(original));
    EXPECT_EQ(encrypt_counter[15], 3U);

    EXPECT_EQ(cipher.crypt_ctr(nullptr, 0U, decrypt_counter), castle::status::ok);
    EXPECT_EQ(cipher.crypt_ctr(nullptr, 1U, decrypt_counter), castle::status::invalid_argument);
    EXPECT_EQ(cipher.crypt_ctr(data, 1U, nullptr), castle::status::invalid_argument);

    uint8_t overflow_counter[16] = {0U};
    for (size_t i = 12U; i < sizeof(overflow_counter); ++i) overflow_counter[i] = 0xFFU;
    uint8_t unchanged = 0x5AU;
    EXPECT_EQ(cipher.crypt_ctr(&unchanged, 1U, overflow_counter), castle::status::out_of_range);
    EXPECT_EQ(unchanged, 0x5AU);
    EXPECT_EQ(overflow_counter[15], 0xFFU);
}

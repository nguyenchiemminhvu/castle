#include "../test_support.hpp"
#include "castle_ext/crypto/camellia.hpp"

namespace
{
const uint8_t plaintext[16] = {
    0x01U,0x23U,0x45U,0x67U,0x89U,0xABU,0xCDU,0xEFU,
    0xFEU,0xDCU,0xBAU,0x98U,0x76U,0x54U,0x32U,0x10U
};
const uint8_t key128[16] = {
    0x01U,0x23U,0x45U,0x67U,0x89U,0xABU,0xCDU,0xEFU,
    0xFEU,0xDCU,0xBAU,0x98U,0x76U,0x54U,0x32U,0x10U
};
const uint8_t key192[24] = {
    0x01U,0x23U,0x45U,0x67U,0x89U,0xABU,0xCDU,0xEFU,
    0xFEU,0xDCU,0xBAU,0x98U,0x76U,0x54U,0x32U,0x10U,
    0x00U,0x11U,0x22U,0x33U,0x44U,0x55U,0x66U,0x77U
};
const uint8_t key256[32] = {
    0x01U,0x23U,0x45U,0x67U,0x89U,0xABU,0xCDU,0xEFU,
    0xFEU,0xDCU,0xBAU,0x98U,0x76U,0x54U,0x32U,0x10U,
    0x00U,0x11U,0x22U,0x33U,0x44U,0x55U,0x66U,0x77U,
    0x88U,0x99U,0xAAU,0xBBU,0xCCU,0xDDU,0xEEU,0xFFU
};
const uint8_t expected128[16] = {
    0x67U,0x67U,0x31U,0x38U,0x54U,0x96U,0x69U,0x73U,
    0x08U,0x57U,0x06U,0x56U,0x48U,0xEAU,0xBEU,0x43U
};
const uint8_t expected192[16] = {
    0xB4U,0x99U,0x34U,0x01U,0xB3U,0xE9U,0x96U,0xF8U,
    0x4EU,0xE5U,0xCEU,0xE7U,0xD7U,0x9BU,0x09U,0xB9U
};
const uint8_t expected256[16] = {
    0x9AU,0xCCU,0x23U,0x7DU,0xFFU,0x16U,0xD7U,0x6CU,
    0x20U,0xEFU,0x7CU,0x91U,0x9EU,0x3AU,0x75U,0x09U
};
}

TEST(CastleExtCamellia, KnownVectorsForAllKeySizes)
{
    uint8_t encrypted[16];
    uint8_t decrypted[16];

    castle::crypto::camellia<128> c128(key128);
    EXPECT_EQ(c128.set_key(key128), castle::status::ok);
    EXPECT_EQ(decltype(c128)::block_size, 16U);
    EXPECT_EQ(decltype(c128)::key_size, 16U);
    EXPECT_EQ(decltype(c128)::rounds, 18U);
    c128.encrypt_block(plaintext, encrypted);
    expect_bytes(encrypted, sizeof(encrypted), expected128, sizeof(expected128));
    c128.decrypt_block(encrypted, decrypted);
    expect_bytes(decrypted, sizeof(decrypted), plaintext, sizeof(plaintext));

    castle::crypto::camellia<192> c192(key192);
    EXPECT_EQ(decltype(c192)::key_size, 24U);
    EXPECT_EQ(decltype(c192)::rounds, 24U);
    c192.encrypt_block(plaintext, encrypted);
    expect_bytes(encrypted, sizeof(encrypted), expected192, sizeof(expected192));
    c192.decrypt_block(encrypted, decrypted);
    expect_bytes(decrypted, sizeof(decrypted), plaintext, sizeof(plaintext));

    castle::crypto::camellia<256> c256(key256);
    EXPECT_EQ(decltype(c256)::key_size, 32U);
    EXPECT_EQ(decltype(c256)::rounds, 24U);
    c256.encrypt_block(plaintext, encrypted);
    expect_bytes(encrypted, sizeof(encrypted), expected256, sizeof(expected256));
    c256.decrypt_block(encrypted, decrypted);
    expect_bytes(decrypted, sizeof(decrypted), plaintext, sizeof(plaintext));
}

TEST(CastleExtCamellia, NullAndInPlaceBranches)
{
    castle::crypto::camellia<128> cipher;
    uint8_t block[16];
    const uint8_t original[16] = {
        0U,1U,2U,3U,4U,5U,6U,7U,8U,9U,10U,11U,12U,13U,14U,15U
    };
    for (size_t i = 0U; i < sizeof(block); ++i) block[i] = original[i];

    cipher.encrypt_block(block, block);
    cipher.decrypt_block(block, block);
    expect_bytes(block, sizeof(block), original, sizeof(original));

    cipher.encrypt_block(nullptr, block);
    cipher.decrypt_block(block, nullptr);
    EXPECT_EQ(cipher.set_key(nullptr), castle::status::invalid_argument);

    castle::crypto::camellia<256> null_key(static_cast<const uint8_t*>(nullptr));
    null_key.encrypt_block(block, block);
    null_key.decrypt_block(block, block);
    expect_bytes(block, sizeof(block), original, sizeof(original));
}

TEST(CastleExtCamellia, CtrKnownVectorVariableLengthAndCounterBounds)
{
    castle::crypto::camellia<128> cipher(key128);
    uint8_t counter[16];
    for (size_t i = 0U; i < sizeof(counter); ++i) counter[i] = plaintext[i];
    uint8_t zero_block[16] = {0U};

    EXPECT_EQ(cipher.crypt_ctr(zero_block, sizeof(zero_block), counter), castle::status::ok);
    expect_bytes(zero_block, sizeof(zero_block), expected128, sizeof(expected128));
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

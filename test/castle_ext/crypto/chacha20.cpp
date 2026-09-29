#include "../test_support.hpp"
#include "castle_ext/crypto/chacha20.hpp"

TEST(CastleExtChaCha20, Rfc8439BlockVectorAndStreaming)
{
    const uint8_t key[32] = {
        0x00U,0x01U,0x02U,0x03U,0x04U,0x05U,0x06U,0x07U,0x08U,0x09U,0x0aU,0x0bU,0x0cU,0x0dU,0x0eU,0x0fU,
        0x10U,0x11U,0x12U,0x13U,0x14U,0x15U,0x16U,0x17U,0x18U,0x19U,0x1aU,0x1bU,0x1cU,0x1dU,0x1eU,0x1fU
    };
    const uint8_t nonce[12] = {0x00U,0x00U,0x00U,0x09U,0x00U,0x00U,0x00U,0x4aU,0x00U,0x00U,0x00U,0x00U};
    const uint8_t expected[64] = {
        0x10U,0xf1U,0xe7U,0xe4U,0xd1U,0x3bU,0x59U,0x15U,0x50U,0x0fU,0xddU,0x1fU,0xa3U,0x20U,0x71U,0xc4U,
        0xc7U,0xd1U,0xf4U,0xc7U,0x33U,0xc0U,0x68U,0x03U,0x04U,0x22U,0xaaU,0x9aU,0xc3U,0xd4U,0x6cU,0x4eU,
        0xd2U,0x82U,0x64U,0x46U,0x07U,0x9fU,0xaaU,0x09U,0x14U,0xc2U,0xd7U,0x05U,0xd9U,0x8bU,0x02U,0xa2U,
        0xb5U,0x12U,0x9cU,0xd1U,0xdeU,0x16U,0x4eU,0xb9U,0xcbU,0xd0U,0x83U,0xe8U,0xa2U,0x50U,0x3cU,0x4eU
    };
    uint8_t actual[64] = {0U};
    castle::crypto::chacha20 cipher(key, nonce, 1U);
    EXPECT_EQ(cipher.counter(), 1U);
    EXPECT_EQ(cipher.crypt(actual, 7U), castle::status::ok);
    EXPECT_EQ(cipher.crypt(actual + 7U, 57U), castle::status::ok);
    expect_bytes(actual, sizeof(actual), expected, sizeof(expected));

    uint8_t round_trip[64];
    for (size_t i = 0U; i < sizeof(round_trip); ++i) round_trip[i] = static_cast<uint8_t>(i);
    uint8_t original[64];
    for (size_t i = 0U; i < sizeof(original); ++i) original[i] = round_trip[i];
    cipher.reset(key, nonce, 1U);
    EXPECT_EQ(cipher.crypt(round_trip, sizeof(round_trip)), castle::status::ok);
    cipher.reset(key, nonce, 1U);
    EXPECT_EQ(cipher.crypt(round_trip, sizeof(round_trip)), castle::status::ok);
    expect_bytes(round_trip, sizeof(round_trip), original, sizeof(original));

    EXPECT_EQ(cipher.set_counter(7U), castle::status::ok);
    EXPECT_EQ(cipher.counter(), 7U);
    EXPECT_EQ(cipher.reset(nullptr, nonce), castle::status::invalid_argument);
    EXPECT_EQ(cipher.reset(key, nullptr), castle::status::invalid_argument);
    EXPECT_EQ(cipher.crypt(nullptr, 0U), castle::status::ok);
    EXPECT_EQ(cipher.crypt(nullptr, 1U), castle::status::invalid_argument);
}

TEST(CastleExtChaCha20, CounterExhaustionPreservesCachedKeystream)
{
    castle::crypto::chacha20 cipher;
    EXPECT_EQ(cipher.set_counter(0xFFFFFFFFU), castle::status::ok);

    uint8_t first_byte = 0U;
    EXPECT_EQ(cipher.crypt(&first_byte, 1U), castle::status::ok);
    EXPECT_EQ(cipher.counter(), 0xFFFFFFFFU);

    uint8_t rejected[64];
    uint8_t original[64];
    for (size_t i = 0U; i < sizeof(rejected); ++i)
    {
        rejected[i] = static_cast<uint8_t>(i);
        original[i] = rejected[i];
    }
    EXPECT_EQ(cipher.crypt(rejected, sizeof(rejected)), castle::status::out_of_range);
    expect_bytes(rejected, sizeof(rejected), original, sizeof(original));

    uint8_t final_cached_bytes[63] = {0U};
    EXPECT_EQ(cipher.crypt(final_cached_bytes, sizeof(final_cached_bytes)), castle::status::ok);
    uint8_t after_exhaustion[1] = {0xA5U};
    EXPECT_EQ(cipher.crypt(after_exhaustion, sizeof(after_exhaustion)), castle::status::out_of_range);
    EXPECT_EQ(after_exhaustion[0], 0xA5U);
}

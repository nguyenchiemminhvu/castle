#include "../../test_support.hpp"
#include "castle_ext/crypto/hash/sha256.hpp"

TEST(CastleExtSha256, KnownVectorStreamingAndFinal)
{
    const uint8_t abc[] = {'a','b','c'};
    const uint8_t expected[32] = {
        0xbaU,0x78U,0x16U,0xbfU,0x8fU,0x01U,0xcfU,0xeaU,
        0x41U,0x41U,0x40U,0xdeU,0x5dU,0xaeU,0x22U,0x23U,
        0xb0U,0x03U,0x61U,0xa3U,0x96U,0x17U,0x7aU,0x9cU,
        0xb4U,0x10U,0xffU,0x61U,0xf2U,0x00U,0x15U,0xadU
    };

    castle::crypto::hash::sha256 hash;
    EXPECT_EQ(hash.update(abc, sizeof(abc)), castle::status::ok);
    auto digest = hash.digest();
    expect_bytes(digest.data(), digest.size(), expected, sizeof(expected));
    uint8_t output[32];
    EXPECT_EQ(hash.final(output, sizeof(output)), castle::status::ok);
    expect_bytes(output, sizeof(output), expected, sizeof(expected));
    EXPECT_EQ(hash.final(output, 31U), castle::status::full);
    EXPECT_EQ(hash.update(static_cast<const uint8_t*>(nullptr), 2U), castle::status::invalid_argument);

    hash.reset();
    EXPECT_EQ(hash.update("abc", 3U), castle::status::ok);
    const auto snapshot = hash.digest();
    expect_bytes(snapshot.data(), snapshot.size(), digest.data(), digest.size());

    const uint8_t empty_expected[32] = {
        0xe3U,0xb0U,0xc4U,0x42U,0x98U,0xfcU,0x1cU,0x14U,0x9aU,0xfbU,0xf4U,0xc8U,0x99U,0x6fU,0xb9U,0x24U,
        0x27U,0xaeU,0x41U,0xe4U,0x64U,0x9bU,0x93U,0x4cU,0xa4U,0x95U,0x99U,0x1bU,0x78U,0x52U,0xb8U,0x55U
    };
    auto empty = castle::crypto::hash::sha256::calculate(static_cast<const uint8_t*>(nullptr), 0U);
    expect_bytes(empty.data(), empty.size(), empty_expected, sizeof(empty_expected));
}

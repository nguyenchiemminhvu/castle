#include "../../test_support.hpp"
#include "castle_ext/crypto/hash/md5.hpp"

TEST(CastleExtMd5, KnownVectorsAndStreaming)
{
    const uint8_t abc[] = {'a','b','c'};
    const uint8_t expected[16] = {
        0x90U,0x01U,0x50U,0x98U,0x3cU,0xd2U,0x4fU,0xb0U,
        0xd6U,0x96U,0x3fU,0x7dU,0x28U,0xe1U,0x7fU,0x72U
    };

    castle::crypto::hash::md5 hash;
    EXPECT_EQ(hash.update(abc, 1U), castle::status::ok);
    EXPECT_EQ(hash.update(abc + 1U, 2U), castle::status::ok);
    auto digest = hash.digest();
    expect_bytes(digest.data(), digest.size(), expected, sizeof(expected));
    EXPECT_EQ(hash.update(static_cast<const uint8_t*>(nullptr), 1U), castle::status::invalid_argument);

    uint8_t output[16];
    EXPECT_EQ(hash.final(output, sizeof(output)), castle::status::ok);
    expect_bytes(output, sizeof(output), expected, sizeof(expected));
    EXPECT_EQ(hash.final(output, 15U), castle::status::full);
    EXPECT_EQ(hash.final(nullptr, 16U), castle::status::invalid_argument);

    auto empty = castle::crypto::hash::md5::calculate(static_cast<const uint8_t*>(nullptr), 0U);
    const uint8_t empty_expected[16] = {
        0xd4U,0x1dU,0x8cU,0xd9U,0x8fU,0x00U,0xb2U,0x04U,
        0xe9U,0x80U,0x09U,0x98U,0xecU,0xf8U,0x42U,0x7eU
    };
    expect_bytes(empty.data(), empty.size(), empty_expected, sizeof(empty_expected));
}

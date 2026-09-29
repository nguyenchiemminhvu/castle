#include "../test_support.hpp"
#include "castle_ext/crypto/xor.hpp"

namespace
{

const uint8_t k_key[2] = {0xAAU, 0x55U};
const uint8_t k_plain[5] = {0x00U, 0x01U, 0x02U, 0x03U, 0x04U};
const uint8_t k_cipher[5] = {0xAAU, 0x54U, 0xA8U, 0x56U, 0xAEU};

} // namespace

TEST(CastleExtXor, FreeFunctionKnownVectorAndRoundTrip)
{
    uint8_t data[5];
    for (size_t i = 0U; i < sizeof(data); ++i) data[i] = k_plain[i];

    EXPECT_EQ(castle::crypto::xor_encrypt(data, sizeof(data), k_key, sizeof(k_key)), castle::status::ok);
    expect_bytes(data, sizeof(data), k_cipher, sizeof(k_cipher));

    EXPECT_EQ(castle::crypto::xor_decrypt(data, sizeof(data), k_key, sizeof(k_key)), castle::status::ok);
    expect_bytes(data, sizeof(data), k_plain, sizeof(k_plain));
}

TEST(CastleExtXor, FreeFunctionSingleByteKeyAndKeyLongerThanData)
{
    const uint8_t one[1] = {0xFFU};
    uint8_t data[3] = {0x00U, 0x0FU, 0xF0U};
    const uint8_t expected_one[3] = {0xFFU, 0xF0U, 0x0FU};
    EXPECT_EQ(castle::crypto::xor_encrypt(data, sizeof(data), one, sizeof(one)), castle::status::ok);
    expect_bytes(data, sizeof(data), expected_one, sizeof(expected_one));

    const uint8_t long_key[8] = {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};
    uint8_t zeros[3] = {0U, 0U, 0U};
    EXPECT_EQ(castle::crypto::xor_encrypt(zeros, sizeof(zeros), long_key, sizeof(long_key)), castle::status::ok);
    expect_bytes(zeros, sizeof(zeros), long_key, sizeof(zeros));
}

TEST(CastleExtXor, FreeFunctionOutOfPlaceKeepsInputIntact)
{
    uint8_t output[5] = {0U};
    EXPECT_EQ(castle::crypto::xor_encrypt(k_plain, output, sizeof(output), k_key, sizeof(k_key)), castle::status::ok);
    expect_bytes(output, sizeof(output), k_cipher, sizeof(k_cipher));
    EXPECT_EQ(k_plain[1], 0x01U);

    uint8_t restored[5] = {0U};
    EXPECT_EQ(castle::crypto::xor_decrypt(output, restored, sizeof(restored), k_key, sizeof(k_key)), castle::status::ok);
    expect_bytes(restored, sizeof(restored), k_plain, sizeof(k_plain));
}

TEST(CastleExtXor, FreeFunctionRejectsInvalidArguments)
{
    uint8_t data[2] = {0x11U, 0x22U};
    uint8_t out[2] = {0x77U, 0x77U};

    EXPECT_EQ(castle::crypto::xor_encrypt(data, 2U, nullptr, 2U), castle::status::invalid_argument);
    EXPECT_EQ(castle::crypto::xor_encrypt(data, 2U, k_key, 0U), castle::status::invalid_argument);
    EXPECT_EQ(castle::crypto::xor_encrypt(nullptr, 2U, k_key, 2U), castle::status::invalid_argument);
    EXPECT_EQ(castle::crypto::xor_encrypt(nullptr, out, 2U, k_key, 2U), castle::status::invalid_argument);
    EXPECT_EQ(castle::crypto::xor_encrypt(data, nullptr, 2U, k_key, 2U), castle::status::invalid_argument);
    EXPECT_EQ(data[0], 0x11U);
    EXPECT_EQ(data[1], 0x22U);
    EXPECT_EQ(out[0], 0x77U);

    EXPECT_EQ(castle::crypto::xor_decrypt(data, 2U, nullptr, 2U), castle::status::invalid_argument);
    EXPECT_EQ(castle::crypto::xor_decrypt(nullptr, out, 2U, k_key, 2U), castle::status::invalid_argument);

    // Empty input is a valid no-op even with null buffers.
    EXPECT_EQ(castle::crypto::xor_encrypt(nullptr, 0U, k_key, 2U), castle::status::ok);
    EXPECT_EQ(castle::crypto::xor_encrypt(nullptr, nullptr, 0U, k_key, 2U), castle::status::ok);
    EXPECT_EQ(castle::crypto::xor_decrypt(nullptr, 0U, k_key, 2U), castle::status::ok);
}

TEST(CastleExtXor, CipherStartsUnconfigured)
{
    castle::crypto::xor_cipher<> cipher;
    uint8_t data[2] = {0x11U, 0x22U};

    EXPECT_FALSE(cipher.configured());
    EXPECT_EQ(cipher.key_size(), 0U);
    EXPECT_EQ(cipher.position(), 0U);
    EXPECT_EQ(decltype(cipher)::max_key_size, 32U);
    EXPECT_EQ(cipher.encrypt(data, 2U), castle::status::not_configured);
    EXPECT_EQ(cipher.seek(1U), castle::status::not_configured);
    EXPECT_EQ(data[0], 0x11U);
    EXPECT_EQ(data[1], 0x22U);
}

TEST(CastleExtXor, CipherMatchesKnownVectorInPlace)
{
    castle::crypto::xor_cipher<8U> cipher(k_key, sizeof(k_key));
    uint8_t data[5];
    for (size_t i = 0U; i < sizeof(data); ++i) data[i] = k_plain[i];

    ASSERT_TRUE(cipher.configured());
    EXPECT_EQ(cipher.key_size(), 2U);
    EXPECT_EQ(cipher.encrypt(data, sizeof(data)), castle::status::ok);
    expect_bytes(data, sizeof(data), k_cipher, sizeof(k_cipher));
    EXPECT_EQ(cipher.position(), 1U);

    cipher.reset();
    EXPECT_EQ(cipher.position(), 0U);
    EXPECT_EQ(cipher.encrypt(data, sizeof(data)), castle::status::ok);
    expect_bytes(data, sizeof(data), k_plain, sizeof(k_plain));
}

TEST(CastleExtXor, CipherOutOfPlace)
{
    castle::crypto::xor_cipher<8U> cipher(k_key, sizeof(k_key));
    uint8_t output[5] = {0U};
    EXPECT_EQ(cipher.encrypt(k_plain, output, sizeof(output)), castle::status::ok);
    expect_bytes(output, sizeof(output), k_cipher, sizeof(k_cipher));
}

TEST(CastleExtXor, CipherDecryptRestoresPlaintext)
{
    castle::crypto::xor_cipher<8U> cipher(k_key, sizeof(k_key));
    uint8_t data[5];
    for (size_t i = 0U; i < sizeof(data); ++i) data[i] = k_plain[i];
    EXPECT_EQ(cipher.encrypt(data, sizeof(data)), castle::status::ok);

    cipher.reset();
    uint8_t restored[5] = {0U};
    EXPECT_EQ(cipher.decrypt(data, restored, sizeof(restored)), castle::status::ok);
    expect_bytes(restored, sizeof(restored), k_plain, sizeof(k_plain));

    cipher.reset();
    EXPECT_EQ(cipher.decrypt(data, sizeof(data)), castle::status::ok);
    expect_bytes(data, sizeof(data), k_plain, sizeof(k_plain));
}

TEST(CastleExtXor, CipherChunkedEqualsOneShot)
{
    const uint8_t key[3] = {0x13U, 0x37U, 0x42U};
    uint8_t whole[10];
    uint8_t chunked[10];
    for (size_t i = 0U; i < sizeof(whole); ++i)
    {
        whole[i] = static_cast<uint8_t>(i * 7U + 1U);
        chunked[i] = whole[i];
    }

    castle::crypto::xor_cipher<4U> one_shot(key);
    EXPECT_EQ(one_shot.encrypt(whole, sizeof(whole)), castle::status::ok);

    castle::crypto::xor_cipher<4U> pieces(key);
    EXPECT_EQ(pieces.encrypt(chunked, 1U), castle::status::ok);
    EXPECT_EQ(pieces.encrypt(chunked + 1U, 0U), castle::status::ok);
    EXPECT_EQ(pieces.encrypt(chunked + 1U, 4U), castle::status::ok);
    EXPECT_EQ(pieces.encrypt(chunked + 5U, 5U), castle::status::ok);
    expect_bytes(chunked, sizeof(chunked), whole, sizeof(whole));
    EXPECT_EQ(pieces.position(), one_shot.position());
    EXPECT_EQ(pieces.position(), 1U);
}

TEST(CastleExtXor, CipherSeekRepositionsStream)
{
    castle::crypto::xor_cipher<8U> cipher(k_key, sizeof(k_key));
    uint8_t tail[2];
    tail[0] = k_cipher[3];
    tail[1] = k_cipher[4];

    EXPECT_EQ(cipher.seek(3U), castle::status::ok);
    EXPECT_EQ(cipher.position(), 1U);
    EXPECT_EQ(cipher.encrypt(tail, sizeof(tail)), castle::status::ok);
    EXPECT_EQ(tail[0], k_plain[3]);
    EXPECT_EQ(tail[1], k_plain[4]);

    EXPECT_EQ(cipher.seek(4U), castle::status::ok);
    EXPECT_EQ(cipher.position(), 0U);
}

TEST(CastleExtXor, CipherConstructorsHandleInvalidKeys)
{
    castle::crypto::xor_cipher<4U> null_key(nullptr, 2U);
    EXPECT_FALSE(null_key.configured());

    castle::crypto::xor_cipher<4U> zero_length(k_key, 0U);
    EXPECT_FALSE(zero_length.configured());

    const uint8_t big[5] = {1U, 2U, 3U, 4U, 5U};
    castle::crypto::xor_cipher<4U> too_long(big, sizeof(big));
    EXPECT_FALSE(too_long.configured());

    castle::crypto::xor_cipher<4U> exact(big, 4U);
    EXPECT_TRUE(exact.configured());
    EXPECT_EQ(exact.key_size(), 4U);
}

TEST(CastleExtXor, CipherSetKeyValidatesAndKeepsPreviousKeyOnError)
{
    castle::crypto::xor_cipher<4U> cipher(k_key, sizeof(k_key));
    uint8_t data[1] = {0x00U};
    ASSERT_EQ(cipher.encrypt(data, 1U), castle::status::ok);
    ASSERT_EQ(cipher.position(), 1U);

    const uint8_t big[5] = {1U, 2U, 3U, 4U, 5U};
    EXPECT_EQ(cipher.set_key(nullptr, 2U), castle::status::invalid_argument);
    EXPECT_EQ(cipher.set_key(k_key, 0U), castle::status::invalid_argument);
    EXPECT_EQ(cipher.set_key(big, sizeof(big)), castle::status::out_of_range);
    EXPECT_EQ(cipher.key_size(), 2U);
    EXPECT_EQ(cipher.position(), 1U);

    // Old key still active: stream continues at key[1].
    data[0] = 0x00U;
    EXPECT_EQ(cipher.encrypt(data, 1U), castle::status::ok);
    EXPECT_EQ(data[0], 0x55U);

    const uint8_t fresh[3] = {0x01U, 0x02U, 0x03U};
    EXPECT_EQ(cipher.set_key(fresh), castle::status::ok);
    EXPECT_EQ(cipher.key_size(), 3U);
    EXPECT_EQ(cipher.position(), 0U);
    data[0] = 0x00U;
    EXPECT_EQ(cipher.encrypt(data, 1U), castle::status::ok);
    EXPECT_EQ(data[0], 0x01U);
}

TEST(CastleExtXor, CipherCryptRejectsNullBuffersWithoutAdvancing)
{
    castle::crypto::xor_cipher<4U> cipher(k_key, sizeof(k_key));
    uint8_t buffer[2] = {0x10U, 0x20U};

    EXPECT_EQ(cipher.encrypt(nullptr, 2U), castle::status::invalid_argument);
    EXPECT_EQ(cipher.encrypt(nullptr, buffer, 2U), castle::status::invalid_argument);
    EXPECT_EQ(cipher.encrypt(buffer, nullptr, 2U), castle::status::invalid_argument);
    EXPECT_EQ(cipher.position(), 0U);
    EXPECT_EQ(buffer[0], 0x10U);

    EXPECT_EQ(cipher.encrypt(nullptr, 0U), castle::status::ok);
    EXPECT_EQ(cipher.position(), 0U);
}

TEST(CastleExtXor, CipherClearWipesKey)
{
    castle::crypto::xor_cipher<4U> cipher(k_key, sizeof(k_key));
    uint8_t data[1] = {0x00U};
    ASSERT_EQ(cipher.encrypt(data, 1U), castle::status::ok);

    cipher.clear();
    EXPECT_FALSE(cipher.configured());
    EXPECT_EQ(cipher.key_size(), 0U);
    EXPECT_EQ(cipher.position(), 0U);
    EXPECT_EQ(cipher.encrypt(data, 1U), castle::status::not_configured);

    EXPECT_EQ(cipher.set_key(k_key, sizeof(k_key)), castle::status::ok);
    EXPECT_TRUE(cipher.configured());
}

TEST(CastleExtXor, CipherCopyKeepsKeyAndPosition)
{
    castle::crypto::xor_cipher<4U> original(k_key, sizeof(k_key));
    uint8_t data[1] = {0x00U};
    ASSERT_EQ(original.encrypt(data, 1U), castle::status::ok);

    castle::crypto::xor_cipher<4U> copy(original);
    original.clear();
    EXPECT_TRUE(copy.configured());
    EXPECT_EQ(copy.position(), 1U);
    data[0] = 0x00U;
    EXPECT_EQ(copy.encrypt(data, 1U), castle::status::ok);
    EXPECT_EQ(data[0], 0x55U);
}

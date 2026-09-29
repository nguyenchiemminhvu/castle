#include "../test_support.hpp"
#include "castle_ext/codecs/base64.hpp"

TEST(CastleExtBase64, EncodeDecodePaddedAndUnpadded)
{
    const uint8_t input[] = {'f','o','o','b','a','r'};
    char encoded[32];
    uint8_t decoded[16];
    size_t encoded_size = 0U;
    size_t decoded_size = 0U;

    EXPECT_EQ(castle::codecs::base64_encoded_size(6U), 8U);
    EXPECT_EQ(castle::codecs::base64_encoded_size(2U, false), 3U);
    EXPECT_EQ(castle::codecs::base64_decoded_size(8U), 6U);
    EXPECT_EQ(castle::codecs::base64_encode(input, sizeof(input), encoded, sizeof(encoded), encoded_size), castle::status::ok);
    EXPECT_EQ(encoded_size, 8U);
    expect_text(encoded, encoded_size, "Zm9vYmFy", 8U);
    EXPECT_EQ(castle::codecs::base64_decode(encoded, encoded_size, decoded, sizeof(decoded), decoded_size), castle::status::ok);
    expect_bytes(decoded, decoded_size, input, sizeof(input));

    EXPECT_EQ(castle::codecs::base64_encode(input, 1U, encoded, sizeof(encoded), encoded_size, true, false), castle::status::ok);
    expect_text(encoded, encoded_size, "Zg==", 4U);
    EXPECT_EQ(castle::codecs::base64_decode(encoded, encoded_size, decoded, sizeof(decoded), decoded_size), castle::status::ok);
    EXPECT_EQ(decoded_size, 1U);
    EXPECT_EQ(decoded[0], static_cast<uint8_t>('f'));

    EXPECT_EQ(castle::codecs::base64_encode(input, 2U, encoded, sizeof(encoded), encoded_size, false, false), castle::status::ok);
    expect_text(encoded, encoded_size, "Zm8", 3U);
    EXPECT_EQ(castle::codecs::base64_decode(encoded, encoded_size, decoded, sizeof(decoded), decoded_size), castle::status::ok);
    EXPECT_EQ(decoded_size, 2U);
    EXPECT_EQ(decoded[0], static_cast<uint8_t>('f'));
    EXPECT_EQ(decoded[1], static_cast<uint8_t>('o'));
}

TEST(CastleExtBase64, InvalidFormsAndCapacity)
{
    uint8_t decoded[8];
    size_t decoded_size = 77U;
    EXPECT_EQ(castle::codecs::base64_decode("A", 1U, decoded, sizeof(decoded), decoded_size), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::base64_decode("AA=A", 4U, decoded, sizeof(decoded), decoded_size), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::base64_decode("AB==", 4U, decoded, sizeof(decoded), decoded_size), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::base64_decode("AAA=", 4U, decoded, 1U, decoded_size), castle::status::full);
    EXPECT_EQ(decoded_size, 0U);
    EXPECT_EQ(castle::codecs::base64_decode("AA", 2U, decoded, sizeof(decoded), decoded_size, false, false), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::base64_decode(nullptr, 1U, decoded, sizeof(decoded), decoded_size), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::base64_encode(nullptr, 1U, reinterpret_cast<char*>(decoded), sizeof(decoded), decoded_size), castle::status::invalid_argument);

    char url[8];
    size_t url_size = 0U;
    const uint8_t bytes[] = {0xFBU, 0xFFU};
    EXPECT_EQ(castle::codecs::base64_encode(bytes, sizeof(bytes), url, sizeof(url), url_size, false, true), castle::status::ok);
    expect_text(url, url_size, "-_8", 3U);
    EXPECT_EQ(castle::codecs::base64_decode(url, url_size, decoded, sizeof(decoded), decoded_size, true), castle::status::ok);
    expect_bytes(decoded, decoded_size, bytes, sizeof(bytes));
}

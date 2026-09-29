#include "../test_support.hpp"
#include "castle_ext/codecs/hex.hpp"

TEST(CastleExtHex, EncodeDecodeCaseAndCapacity)
{
    const uint8_t input[] = {0x00U, 0x01U, 0xA5U, 0xFFU};
    char encoded[16];
    uint8_t decoded[8];
    size_t encoded_size = 0U;
    size_t decoded_size = 0U;

    EXPECT_EQ(castle::codecs::hex_encoded_size(sizeof(input)), 8U);
    EXPECT_EQ(castle::codecs::hex_encode(input, sizeof(input), encoded, sizeof(encoded), encoded_size), castle::status::ok);
    expect_text(encoded, encoded_size, "0001a5ff", 8U);
    EXPECT_EQ(castle::codecs::hex_encode(input, sizeof(input), encoded, sizeof(encoded), encoded_size, true), castle::status::ok);
    expect_text(encoded, encoded_size, "0001A5FF", 8U);
    EXPECT_EQ(castle::codecs::hex_decode("0001a5Ff", 8U, decoded, sizeof(decoded), decoded_size), castle::status::ok);
    expect_bytes(decoded, decoded_size, input, sizeof(input));

    EXPECT_EQ(castle::codecs::hex_decode("00G1", 4U, decoded, sizeof(decoded), decoded_size), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::hex_decode("0", 1U, decoded, sizeof(decoded), decoded_size), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::hex_decode("0011", 4U, decoded, 1U, decoded_size), castle::status::full);
    EXPECT_EQ(decoded_size, 0U);
    EXPECT_EQ(castle::codecs::hex_encode(nullptr, 1U, encoded, sizeof(encoded), encoded_size), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::hex_decode(nullptr, 1U, decoded, sizeof(decoded), decoded_size), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::hex_encode(nullptr, 0U, nullptr, 0U, encoded_size), castle::status::ok);
}

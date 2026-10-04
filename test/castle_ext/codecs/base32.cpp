#include "../test_support.hpp"
#include "castle_ext/codecs/base32.hpp"

TEST(CastleExtBase32, EncodeDecodePaddedAndUnpadded)
{
    const uint8_t input[] = {'f','o','o','b','a','r'};
    char encoded[32];
    uint8_t decoded[16];
    size_t encoded_size = 0U;
    size_t decoded_size = 0U;

    EXPECT_EQ(castle::codecs::base32_encoded_size(6U), 16U);
    EXPECT_EQ(castle::codecs::base32_encoded_size(3U, false), 5U);
    EXPECT_EQ(castle::codecs::base32_decoded_size(8U), 5U);
    EXPECT_EQ(castle::codecs::base32_encode(input, sizeof(input), encoded, sizeof(encoded), encoded_size), castle::status::ok);
    EXPECT_EQ(encoded_size, 16U);
    expect_text(encoded, encoded_size, "MZXW6YTBOI======", 16U);
    EXPECT_EQ(castle::codecs::base32_decode(encoded, encoded_size, decoded, sizeof(decoded), decoded_size), castle::status::ok);
    expect_bytes(decoded, decoded_size, input, sizeof(input));

    EXPECT_EQ(castle::codecs::base32_encode(input, sizeof(input), encoded, sizeof(encoded), encoded_size, false), castle::status::ok);
    EXPECT_EQ(encoded_size, 10U);
    expect_text(encoded, encoded_size, "MZXW6YTBOI", 10U);
    EXPECT_EQ(castle::codecs::base32_decode(encoded, encoded_size, decoded, sizeof(decoded), decoded_size), castle::status::ok);
    expect_bytes(decoded, decoded_size, input, sizeof(input));

    encoded[0] = 'm';
    EXPECT_EQ(castle::codecs::base32_decode("MZXW6YTBOI======", 16U, decoded, sizeof(decoded), decoded_size), castle::status::ok);
    EXPECT_EQ(castle::codecs::base32_decode("MZXW6Y!BOI======", 16U, decoded, sizeof(decoded), decoded_size), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::base32_decode("A", 1U, decoded, sizeof(decoded), decoded_size), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::base32_decode("MZXW6YTB=", 9U, decoded, sizeof(decoded), decoded_size), castle::status::invalid_argument);
}

TEST(CastleExtBase32, CapacityAndNullArguments)
{
    const uint8_t input[] = {0x01U, 0x02U};
    char output[8];
    size_t output_size = 99U;
    EXPECT_EQ(castle::codecs::base32_encode(input, sizeof(input), output, 3U, output_size), castle::status::full);
    EXPECT_EQ(output_size, 0U);
    EXPECT_EQ(castle::codecs::base32_encode(nullptr, 1U, output, sizeof(output), output_size), castle::status::invalid_argument);
    EXPECT_EQ(castle::codecs::base32_decode(nullptr, 1U, reinterpret_cast<uint8_t*>(output), sizeof(output), output_size), castle::status::invalid_argument);
}

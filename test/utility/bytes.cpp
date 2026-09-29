#include <gtest/gtest.h>

#include "castle/utility/bytes.hpp"

#include <stdint.h>

namespace
{

TEST(BytesTest, LoadsLittleAndBigEndianIntegers)
{
    const uint8_t input[8] = {0x01U, 0x23U, 0x45U, 0x67U, 0x89U, 0xABU, 0xCDU, 0xEFU};

    EXPECT_EQ(castle::read_le8(input), 0x01U);
    EXPECT_EQ(castle::read_be8(input), 0x01U);
    EXPECT_EQ(castle::read_le16(input), 0x2301U);
    EXPECT_EQ(castle::read_be16(input), 0x0123U);
    EXPECT_EQ(castle::read_le32(input), 0x67452301U);
    EXPECT_EQ(castle::read_be32(input), 0x01234567U);
    EXPECT_EQ(castle::read_le64(input), 0xEFCDAB8967452301ULL);
    EXPECT_EQ(castle::read_be64(input), 0x0123456789ABCDEFULL);
}

TEST(BytesTest, LoadsSignedLittleAndBigEndianIntegers)
{
    const uint8_t input_le[8] = {0xFEU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU};
    const uint8_t input_be[8] = {0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFEU};

    EXPECT_EQ(castle::read_le8s(input_le), -2);
    EXPECT_EQ(castle::read_be8s(input_be + 7U), -2);
    EXPECT_EQ(castle::read_le16s(input_le), -2);
    EXPECT_EQ(castle::read_be16s(input_be + 6U), -2);
    EXPECT_EQ(castle::read_le32s(input_le), -2);
    EXPECT_EQ(castle::read_be32s(input_be + 4U), -2);
    EXPECT_EQ(castle::read_le64s(input_le), -2);
    EXPECT_EQ(castle::read_be64s(input_be), -2);
}

TEST(BytesTest, StoresLittleAndBigEndianIntegers)
{
    uint8_t output[8] = {};

    castle::write_le8(output, 0xABU);
    EXPECT_EQ(output[0], 0xABU);
    castle::write_be8(output, 0xCDU);
    EXPECT_EQ(output[0], 0xCDU);

    castle::write_le16(output, 0x1234U);
    EXPECT_EQ(output[0], 0x34U);
    EXPECT_EQ(output[1], 0x12U);
    castle::write_be16(output, 0x1234U);
    EXPECT_EQ(output[0], 0x12U);
    EXPECT_EQ(output[1], 0x34U);

    castle::write_le32(output, 0x12345678U);
    EXPECT_EQ(castle::read_le32(output), 0x12345678U);
    castle::write_be32(output, 0x12345678U);
    EXPECT_EQ(castle::read_be32(output), 0x12345678U);
    castle::write_le64(output, 0x0123456789ABCDEFULL);
    EXPECT_EQ(castle::read_le64(output), 0x0123456789ABCDEFULL);
    castle::write_be64(output, 0x0123456789ABCDEFULL);
    EXPECT_EQ(castle::read_be64(output), 0x0123456789ABCDEFULL);
}

TEST(BytesTest, StoresSignedLittleAndBigEndianIntegers)
{
    uint8_t output[8] = {};

    castle::write_le8s(output, static_cast<int8_t>(-2));
    EXPECT_EQ(output[0], 0xFEU);
    castle::write_be8s(output, static_cast<int8_t>(-2));
    EXPECT_EQ(output[0], 0xFEU);

    castle::write_le16s(output, static_cast<int16_t>(-2));
    EXPECT_EQ(output[0], 0xFEU);
    EXPECT_EQ(output[1], 0xFFU);
    castle::write_be16s(output, static_cast<int16_t>(-2));
    EXPECT_EQ(output[0], 0xFFU);
    EXPECT_EQ(output[1], 0xFEU);

    castle::write_le32s(output, static_cast<int32_t>(-2));
    EXPECT_EQ(castle::read_le32(output), 0xFFFFFFFEU);
    castle::write_be32s(output, static_cast<int32_t>(-2));
    EXPECT_EQ(castle::read_be32(output), 0xFFFFFFFEU);
    castle::write_le64s(output, static_cast<int64_t>(-2));
    EXPECT_EQ(castle::read_le64(output), 0xFFFFFFFFFFFFFFFEULL);
    castle::write_be64s(output, static_cast<int64_t>(-2));
    EXPECT_EQ(castle::read_be64(output), 0xFFFFFFFFFFFFFFFEULL);
}

TEST(BytesTest, RotatesAndSecurelyClears)
{
    EXPECT_EQ(castle::rotl32(0x00000001U, 1U), 0x00000002U);
    EXPECT_EQ(castle::rotr32(0x00000002U, 1U), 0x00000001U);
    EXPECT_EQ(castle::rotr64(0x0000000000000002ULL, 1U), 0x0000000000000001ULL);

    uint8_t data[3] = {1U, 2U, 3U};
    castle::secure_zero(data, sizeof(data));
    EXPECT_EQ(data[0], 0U);
    EXPECT_EQ(data[1], 0U);
    EXPECT_EQ(data[2], 0U);
}

} // namespace
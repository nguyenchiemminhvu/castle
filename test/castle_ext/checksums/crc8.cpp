#include "../test_support.hpp"
#include "castle_ext/checksums/crc8.hpp"

TEST(CastleExtCrc8, KnownVectorAndStreaming)
{
    const uint8_t message[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::crc8 crc;
    crc.update(message, 4U);
    crc.update(message + 4U, 5U);
    EXPECT_EQ(crc.value(), 0xF4U);
    EXPECT_EQ(crc.checksum(), 0xF4U);
    EXPECT_EQ(castle::checksums::crc8::calculate(message, 9U), 0xF4U);
    crc.reset();
    EXPECT_EQ(crc.value(), 0x00U);
    EXPECT_EQ(castle::checksums::crc8::calculate(static_cast<const void*>(message), 9U), 0xF4U);
    crc.update(static_cast<const uint8_t*>(nullptr), 0U);
    EXPECT_EQ(crc.value(), 0x00U);
}

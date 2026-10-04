#include "../test_support.hpp"
#include "castle_ext/checksums/crc16.hpp"

TEST(CastleExtCrc16, KnownVectorAndStreaming)
{
    const uint8_t message[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::crc16 crc;
    crc.update(message, 1U);
    for (size_t i = 1U; i < 9U; ++i) crc.update(message + i, 1U);
    EXPECT_EQ(crc.value(), 0x29B1U);
    EXPECT_EQ(crc.checksum(), 0x29B1U);
    EXPECT_EQ(castle::checksums::crc16::calculate(message, 9U), 0x29B1U);
    crc.reset();
    EXPECT_EQ(crc.value(), 0xFFFFU);
    EXPECT_EQ(castle::checksums::crc16::calculate(static_cast<const void*>(message), 9U), 0x29B1U);
}

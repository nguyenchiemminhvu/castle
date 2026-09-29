#include "../test_support.hpp"
#include "castle_ext/checksums/crc32.hpp"

TEST(CastleExtCrc32, KnownVectorAndStreaming)
{
    const uint8_t message[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::crc32 crc;
    crc.update(static_cast<const uint8_t*>(nullptr), 0U);
    EXPECT_EQ(crc.value(), 0x00000000U);
    crc.update(message, 3U);
    crc.update(message + 3U, 6U);
    EXPECT_EQ(crc.value(), 0xCBF43926UL);
    EXPECT_EQ(crc.checksum(), 0xCBF43926UL);
    EXPECT_EQ(castle::checksums::crc32::calculate(message, 9U), 0xCBF43926UL);
    crc.reset();
    EXPECT_EQ(crc.value(), 0x00000000UL);
}

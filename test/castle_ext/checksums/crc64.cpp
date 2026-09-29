#include "../test_support.hpp"
#include "castle_ext/checksums/crc64.hpp"

TEST(CastleExtCrc64, KnownVectorAndStreaming)
{
    const uint8_t message[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::crc64 crc;
    crc.update(message, 0U);
    EXPECT_EQ(crc.value(), 0ULL);
    crc.update(message, 2U);
    crc.update(message + 2U, 7U);
    EXPECT_EQ(crc.value(), 0x6C40DF5F0B497347ULL);
    EXPECT_EQ(crc.checksum(), 0x6C40DF5F0B497347ULL);
    EXPECT_EQ(castle::checksums::crc64::calculate(message, 9U), 0x6C40DF5F0B497347ULL);
    crc.reset();
    EXPECT_EQ(crc.value(), 0ULL);
}

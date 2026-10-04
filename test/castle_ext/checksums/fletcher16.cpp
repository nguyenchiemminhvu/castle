#include "../test_support.hpp"
#include "castle_ext/checksums/fletcher16.hpp"

TEST(CastleExtFletcher16, KnownVectorAndReset)
{
    const uint8_t message[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::fletcher16 checksum;
    checksum.update(message, 4U);
    checksum.update(message + 4U, 5U);
    EXPECT_EQ(checksum.value(), 0x1EDEU);
    EXPECT_EQ(checksum.checksum(), 0x1EDEU);
    EXPECT_EQ(castle::checksums::fletcher16::calculate(message, 9U), 0x1EDEU);
    EXPECT_EQ(castle::checksums::fletcher16::calculate(static_cast<const void*>(message), 9U), 0x1EDEU);
    checksum.reset();
    EXPECT_EQ(checksum.value(), 0U);
    checksum.update(static_cast<const uint8_t*>(nullptr), 0U);
    EXPECT_EQ(checksum.value(), 0U);
}

TEST(CastleExtFletcher16, ReferenceVectors)
{
    EXPECT_EQ(castle::checksums::fletcher16::calculate(reinterpret_cast<const uint8_t*>("abcde"), 5U), 0xC8F0U);
    EXPECT_EQ(castle::checksums::fletcher16::calculate(reinterpret_cast<const uint8_t*>("abcdef"), 6U), 0x2057U);
    EXPECT_EQ(castle::checksums::fletcher16::calculate(reinterpret_cast<const uint8_t*>("abcdefgh"), 8U), 0x0627U);
}

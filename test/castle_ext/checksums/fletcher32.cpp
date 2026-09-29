#include "../test_support.hpp"
#include "castle_ext/checksums/fletcher32.hpp"

TEST(CastleExtFletcher32, KnownVectorsLittleEndian)
{
    EXPECT_EQ(castle::checksums::fletcher32::calculate(reinterpret_cast<const uint8_t*>("abcde"), 5U), 0xF04FC729UL);
    EXPECT_EQ(castle::checksums::fletcher32::calculate(reinterpret_cast<const uint8_t*>("abcdef"), 6U), 0x56502D2AUL);
    EXPECT_EQ(castle::checksums::fletcher32::calculate(reinterpret_cast<const uint8_t*>("abcdefgh"), 8U), 0xEBE19591UL);
    EXPECT_EQ(castle::checksums::fletcher32::calculate(reinterpret_cast<const uint8_t*>("123456789"), 9U), 0xDF09D509UL);
}

TEST(CastleExtFletcher32, BigEndianBlocks)
{
    using be_fletcher32 = castle::checksums::basic_fletcher32<false>;
    EXPECT_EQ(be_fletcher32::calculate(reinterpret_cast<const uint8_t*>("abcde"), 5U), 0x4FF029C7UL);
    EXPECT_EQ(be_fletcher32::calculate(reinterpret_cast<const uint8_t*>("abcdef"), 6U), 0x50562A2DUL);
    EXPECT_EQ(be_fletcher32::calculate(reinterpret_cast<const uint8_t*>("abcdefgh"), 8U), 0xE1EB9195UL);
}

TEST(CastleExtFletcher32, StreamingAcrossBlockBoundaries)
{
    const uint8_t message[] = {'1','2','3','4','5','6','7','8','9'};
    const uint32_t expected = castle::checksums::fletcher32::calculate(message, 9U);
    EXPECT_EQ(expected, 0xDF09D509UL);

    for (size_t split = 0U; split <= 9U; ++split)
    {
        castle::checksums::fletcher32 checksum;
        checksum.update(message, split);
        checksum.update(message + split, 9U - split);
        EXPECT_EQ(checksum.value(), expected);
    }

    castle::checksums::fletcher32 bytewise;
    for (size_t i = 0U; i < 9U; ++i) bytewise.update(message + i, 1U);
    EXPECT_EQ(bytewise.checksum(), expected);
}

TEST(CastleExtFletcher32, ValueDoesNotConsumePartialBlock)
{
    const uint8_t message[] = {'a','b','c','d','e'};
    castle::checksums::fletcher32 checksum;
    checksum.update(message, 3U);
    (void)checksum.value();
    checksum.update(message + 3U, 2U);
    EXPECT_EQ(checksum.value(), 0xF04FC729UL);
    checksum.reset();
    EXPECT_EQ(checksum.value(), 0U);
}

TEST(CastleExtFletcher32, SaturatedBlocksStayReduced)
{
    uint8_t message[64];
    for (size_t i = 0U; i < sizeof(message); ++i) message[i] = 0xFFU;
    // 0xFFFF equals the modulus, so every block is congruent to zero.
    EXPECT_EQ(castle::checksums::fletcher32::calculate(message, sizeof(message)), 0U);
    EXPECT_EQ(castle::checksums::fletcher32::calculate(static_cast<const void*>(message), sizeof(message)), 0U);
}

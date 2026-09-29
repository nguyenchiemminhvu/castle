#include "../test_support.hpp"
#include "castle_ext/checksums/fletcher64.hpp"

TEST(CastleExtFletcher64, KnownVectorsLittleEndian)
{
    EXPECT_EQ(castle::checksums::fletcher64::calculate(reinterpret_cast<const uint8_t*>("abcde"), 5U), 0xC8C6C527646362C6ULL);
    EXPECT_EQ(castle::checksums::fletcher64::calculate(reinterpret_cast<const uint8_t*>("abcdef"), 6U), 0xC8C72B276463C8C6ULL);
    EXPECT_EQ(castle::checksums::fletcher64::calculate(reinterpret_cast<const uint8_t*>("abcdefgh"), 8U), 0x312E2B28CCCAC8C6ULL);
    EXPECT_EQ(castle::checksums::fletcher64::calculate(reinterpret_cast<const uint8_t*>("123456789"), 9U), 0x0D0803376C6A689FULL);
}

TEST(CastleExtFletcher64, BigEndianBlocks)
{
    using be_fletcher64 = castle::checksums::basic_fletcher64<false>;
    EXPECT_EQ(be_fletcher64::calculate(reinterpret_cast<const uint8_t*>("abcde"), 5U), 0x27C4C6C9C6626364ULL);
    EXPECT_EQ(be_fletcher64::calculate(reinterpret_cast<const uint8_t*>("abcdefgh"), 8U), 0x282B2E31C6C8CACCULL);
    EXPECT_EQ(be_fletcher64::calculate(reinterpret_cast<const uint8_t*>("123456789"), 9U), 0x3703080D9F686A6CULL);
}

TEST(CastleExtFletcher64, StreamingAcrossBlockBoundaries)
{
    const uint8_t message[] = {'1','2','3','4','5','6','7','8','9'};
    const uint64_t expected = castle::checksums::fletcher64::calculate(static_cast<const void*>(message), 9U);
    EXPECT_EQ(expected, 0x0D0803376C6A689FULL);

    for (size_t split = 0U; split <= 9U; ++split)
    {
        castle::checksums::fletcher64 checksum;
        checksum.update(message, split);
        checksum.update(message + split, 9U - split);
        EXPECT_EQ(checksum.value(), expected);
    }

    castle::checksums::fletcher64 checksum;
    checksum.update(static_cast<const uint8_t*>(nullptr), 4U);
    EXPECT_EQ(checksum.value(), 0U);
    checksum.update(message, 3U);
    checksum.reset();
    EXPECT_EQ(checksum.checksum(), 0U);
}

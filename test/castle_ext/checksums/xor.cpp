#include "../test_support.hpp"
#include "castle_ext/checksums/xor.hpp"

TEST(CastleExtXorChecksum, KnownVectorInitialValueAndReset)
{
    const uint8_t message[] = {'1','2','3','4','5','6','7','8','9'};
    castle::checksums::xor_checksum checksum(0xFFU);
    checksum.update(message, 4U);
    checksum.update(message + 4U, 5U);
    EXPECT_EQ(checksum.value(), static_cast<uint8_t>(0xCEU));
    EXPECT_EQ(castle::checksums::xor_checksum::calculate(message, 9U), static_cast<uint8_t>(0x31U));
    EXPECT_EQ(castle::checksums::xor_checksum::calculate(static_cast<const void*>(message), 9U, 0xFFU), static_cast<uint8_t>(0xCEU));
    checksum.reset();
    EXPECT_EQ(checksum.checksum(), 0U);
    checksum.update(static_cast<const uint8_t*>(nullptr), 0U);
    EXPECT_EQ(checksum.value(), 0U);
}

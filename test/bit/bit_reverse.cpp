#include <gtest/gtest.h>

#include "castle/bit/bit_reverse.h"

#include <cstdint>

namespace cb = castle::bit;

namespace
{

// ── reverse_bits ────────────────────────────────────────────────

TEST(BitReverse, ReverseBits8)
{
    EXPECT_EQ(cb::reverse_bits(uint8_t{0x00}), uint8_t{0x00});
    EXPECT_EQ(cb::reverse_bits(uint8_t{0xFF}), uint8_t{0xFF});
    EXPECT_EQ(cb::reverse_bits(uint8_t{0x01}), uint8_t{0x80});
    EXPECT_EQ(cb::reverse_bits(uint8_t{0x80}), uint8_t{0x01});
    EXPECT_EQ(cb::reverse_bits(uint8_t{0xB0}), uint8_t{0x0D}); // 0b10110000 -> 0b00001101
    EXPECT_EQ(cb::reverse_bits(uint8_t{0xAA}), uint8_t{0x55});
    static_assert(cb::reverse_bits(uint8_t{0x01}) == uint8_t{0x80}, "rev 0x01");
}

TEST(BitReverse, ReverseBits16)
{
    EXPECT_EQ(cb::reverse_bits(uint16_t{0x0001}), uint16_t{0x8000});
    EXPECT_EQ(cb::reverse_bits(uint16_t{0xFFFF}), uint16_t{0xFFFF});
    EXPECT_EQ(cb::reverse_bits(uint16_t{0xAAAA}), uint16_t{0x5555});
    static_assert(cb::reverse_bits(uint16_t{0x0001}) == uint16_t{0x8000}, "rev16");
}

TEST(BitReverse, ReverseBits32)
{
    EXPECT_EQ(cb::reverse_bits(uint32_t{0x00000001U}), uint32_t{0x80000000U});
    EXPECT_EQ(cb::reverse_bits(uint32_t{0xFFFFFFFFU}), uint32_t{0xFFFFFFFFU});
    EXPECT_EQ(cb::reverse_bits(uint32_t{0x0000FFFFU}), uint32_t{0xFFFF0000U});
    static_assert(cb::reverse_bits(uint32_t{0x00000001U}) == uint32_t{0x80000000U}, "rev32");
}

TEST(BitReverse, ReverseBits64)
{
    EXPECT_EQ(cb::reverse_bits(uint64_t{0x1ULL}), uint64_t{0x8000000000000000ULL});
    EXPECT_EQ(cb::reverse_bits(uint64_t{0xFFFFFFFFFFFFFFFFULL}), uint64_t{0xFFFFFFFFFFFFFFFFULL});
    static_assert(cb::reverse_bits(uint64_t{0x1ULL}) == uint64_t{0x8000000000000000ULL}, "rev64");
}

TEST(BitReverse, ReverseBitsTemplateSigned)
{
    // Generic template dispatch for signed types operates on the unsigned repr.
    EXPECT_EQ(cb::reverse_bits(int8_t{0x01}), int8_t{static_cast<int8_t>(0x80)});
}

TEST(BitReverse, ReverseBitsTwiceIsIdentity)
{
    EXPECT_EQ(cb::reverse_bits(cb::reverse_bits(uint8_t{0xB4})), uint8_t{0xB4});
    EXPECT_EQ(cb::reverse_bits(cb::reverse_bits(uint32_t{0x12345678U})), uint32_t{0x12345678U});
}

// ── byte_swap ───────────────────────────────────────────────────

TEST(BitReverse, ByteSwap)
{
    EXPECT_EQ(cb::byte_swap(uint8_t{0xAB}), uint8_t{0xAB}); // no-op
    EXPECT_EQ(cb::byte_swap(uint16_t{0x1234}), uint16_t{0x3412});
    EXPECT_EQ(cb::byte_swap(uint32_t{0x12345678U}), uint32_t{0x78563412U});
    EXPECT_EQ(cb::byte_swap(uint64_t{0x0123456789ABCDEFULL}), uint64_t{0xEFCDAB8967452301ULL});
    static_assert(cb::byte_swap(uint16_t{0x1234}) == uint16_t{0x3412}, "bswap16");
    static_assert(cb::byte_swap(uint32_t{0x12345678U}) == uint32_t{0x78563412U}, "bswap32");
}

TEST(BitReverse, ByteSwapTwiceIsIdentity)
{
    EXPECT_EQ(cb::byte_swap(cb::byte_swap(uint16_t{0x1234})), uint16_t{0x1234});
    EXPECT_EQ(cb::byte_swap(cb::byte_swap(uint64_t{0x0123456789ABCDEFULL})),
              uint64_t{0x0123456789ABCDEFULL});
}

// ── reverse_bytes (alias) ───────────────────────────────────────

TEST(BitReverse, ReverseBytesAlias)
{
    EXPECT_EQ(cb::reverse_bytes(uint16_t{0x1234}), uint16_t{0x3412});
    EXPECT_EQ(cb::reverse_bytes(uint32_t{0x12345678U}), uint32_t{0x78563412U});
    EXPECT_EQ(cb::reverse_bytes(uint32_t{0x12345678U}), cb::byte_swap(uint32_t{0x12345678U}));
    static_assert(cb::reverse_bytes(uint16_t{0x1234}) == uint16_t{0x3412}, "reverse_bytes");
}

} // namespace

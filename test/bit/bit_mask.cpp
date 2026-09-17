#include <gtest/gtest.h>

#include "castle/bit/bit_mask.h"

#include <cstdint>

namespace cb = castle::bit;

namespace
{

// ── all_bits_mask ───────────────────────────────────────────────

TEST(BitMask, AllBitsMask)
{
    EXPECT_EQ(cb::all_bits_mask<uint8_t>::value, uint8_t{0xFF});
    EXPECT_EQ(cb::all_bits_mask<uint16_t>::value, uint16_t{0xFFFF});
    EXPECT_EQ(cb::all_bits_mask<uint32_t>::value, uint32_t{0xFFFFFFFFU});
    EXPECT_EQ(cb::all_bits_mask<uint64_t>::value, uint64_t{0xFFFFFFFFFFFFFFFFULL});
    static_assert(cb::all_bits_mask<uint8_t>::value == uint8_t{0xFF}, "all bits");
}

// ── single_bit_mask ─────────────────────────────────────────────

TEST(BitMask, SingleBitMaskRuntime)
{
    EXPECT_EQ(cb::single_bit_mask<uint8_t>(0), uint8_t{0x01});
    EXPECT_EQ(cb::single_bit_mask<uint8_t>(3), uint8_t{0x08});
    EXPECT_EQ(cb::single_bit_mask<uint8_t>(7), uint8_t{0x80});
    EXPECT_EQ(cb::single_bit_mask<uint32_t>(31), uint32_t{0x80000000U});
    static_assert(cb::single_bit_mask<uint8_t>(3) == uint8_t{0x08}, "single bit 3");
}

TEST(BitMask, SingleBitMaskConst)
{
    EXPECT_EQ((cb::single_bit_mask_const<0, uint8_t>::value), uint8_t{0x01});
    EXPECT_EQ((cb::single_bit_mask_const<7, uint8_t>::value), uint8_t{0x80});
    EXPECT_EQ((cb::single_bit_mask_const<15, uint16_t>::value), uint16_t{0x8000});
    static_assert(cb::single_bit_mask_const<3, uint8_t>::value == uint8_t{0x08}, "const bit 3");
}

// ── low_bits_mask ───────────────────────────────────────────────

TEST(BitMask, LowBitsMaskRuntime)
{
    EXPECT_EQ(cb::low_bits_mask<uint8_t>(0), uint8_t{0x00});
    EXPECT_EQ(cb::low_bits_mask<uint8_t>(3), uint8_t{0x07});
    EXPECT_EQ(cb::low_bits_mask<uint8_t>(8), uint8_t{0xFF});   // >= width -> all ones
    EXPECT_EQ(cb::low_bits_mask<uint8_t>(100), uint8_t{0xFF}); // saturates
    EXPECT_EQ(cb::low_bits_mask<uint16_t>(4), uint16_t{0x000F});
    EXPECT_EQ(cb::low_bits_mask<uint32_t>(16), uint32_t{0x0000FFFFU});
    static_assert(cb::low_bits_mask<uint8_t>(3) == uint8_t{0x07}, "low 3");
}

TEST(BitMask, LowBitsMaskConst)
{
    EXPECT_EQ((cb::low_bits_mask_const<3, uint8_t>::value), uint8_t{0x07});
    EXPECT_EQ((cb::low_bits_mask_const<8, uint8_t>::value), uint8_t{0xFF});
    EXPECT_EQ((cb::low_bits_mask_const<0, uint8_t>::value), uint8_t{0x00});
    static_assert(cb::low_bits_mask_const<4, uint16_t>::value == uint16_t{0x000F}, "low const 4");
}

// ── high_bits_mask ──────────────────────────────────────────────

TEST(BitMask, HighBitsMaskRuntime)
{
    EXPECT_EQ(cb::high_bits_mask<uint8_t>(0), uint8_t{0x00});
    EXPECT_EQ(cb::high_bits_mask<uint8_t>(3), uint8_t{0xE0});
    EXPECT_EQ(cb::high_bits_mask<uint8_t>(8), uint8_t{0xFF});
    EXPECT_EQ(cb::high_bits_mask<uint8_t>(100), uint8_t{0xFF});
    EXPECT_EQ(cb::high_bits_mask<uint16_t>(4), uint16_t{0xF000});
    EXPECT_EQ(cb::high_bits_mask<uint32_t>(16), uint32_t{0xFFFF0000U});
    static_assert(cb::high_bits_mask<uint8_t>(3) == uint8_t{0xE0}, "high 3");
}

TEST(BitMask, HighBitsMaskConst)
{
    EXPECT_EQ((cb::high_bits_mask_const<0, uint8_t>::value), uint8_t{0x00});
    EXPECT_EQ((cb::high_bits_mask_const<3, uint8_t>::value), uint8_t{0xE0});
    EXPECT_EQ((cb::high_bits_mask_const<8, uint8_t>::value), uint8_t{0xFF});
    static_assert(cb::high_bits_mask_const<4, uint16_t>::value == uint16_t{0xF000}, "high const 4");
}

// ── range_mask ──────────────────────────────────────────────────

TEST(BitMask, RangeMaskRuntime)
{
    EXPECT_EQ(cb::range_mask<uint8_t>(2, 3), uint8_t{0x1C}); // 0b00011100
    EXPECT_EQ(cb::range_mask<uint8_t>(0, 8), uint8_t{0xFF});
    EXPECT_EQ(cb::range_mask<uint8_t>(0, 0), uint8_t{0x00});
    EXPECT_EQ(cb::range_mask<uint16_t>(4, 4), uint16_t{0x00F0});
    EXPECT_EQ(cb::range_mask<uint32_t>(8, 8), uint32_t{0x0000FF00U});
    static_assert(cb::range_mask<uint8_t>(2, 3) == uint8_t{0x1C}, "range 2,3");
}

TEST(BitMask, RangeMaskConst)
{
    EXPECT_EQ((cb::range_mask_const<2, 3, uint8_t>::value), uint8_t{0x1C});
    EXPECT_EQ((cb::range_mask_const<4, 4, uint16_t>::value), uint16_t{0x00F0});
    static_assert(cb::range_mask_const<2, 3, uint8_t>::value == uint8_t{0x1C}, "range const");
}

} // namespace

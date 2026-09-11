#include <gtest/gtest.h>

#include "castle/bit/bit_utils.h"

#include <cstdint>

namespace cb = castle::bit;

namespace
{

// ── extract_lowest_set_bit ──────────────────────────────────────

TEST(BitUtils, ExtractLowestSetBit)
{
    EXPECT_EQ(cb::extract_lowest_set_bit(uint8_t{0xB4}), uint8_t{0x04}); // 0b10110100
    EXPECT_EQ(cb::extract_lowest_set_bit(uint8_t{0x01}), uint8_t{0x01});
    EXPECT_EQ(cb::extract_lowest_set_bit(uint8_t{0x80}), uint8_t{0x80});
    EXPECT_EQ(cb::extract_lowest_set_bit(uint8_t{0x00}), uint8_t{0x00});
    EXPECT_EQ(cb::extract_lowest_set_bit(uint32_t{0x00F00000U}), uint32_t{0x00100000U});
    static_assert(cb::extract_lowest_set_bit(uint8_t{0xB4}) == uint8_t{0x04}, "lowest set");
}

// ── extract_highest_set_bit ─────────────────────────────────────

TEST(BitUtils, ExtractHighestSetBit)
{
    EXPECT_EQ(cb::extract_highest_set_bit(uint8_t{0xB4}), uint8_t{0x80});
    EXPECT_EQ(cb::extract_highest_set_bit(uint8_t{0x01}), uint8_t{0x01});
    EXPECT_EQ(cb::extract_highest_set_bit(uint8_t{0x00}), uint8_t{0x00});
    EXPECT_EQ(cb::extract_highest_set_bit(uint8_t{0x0F}), uint8_t{0x08});
    EXPECT_EQ(cb::extract_highest_set_bit(uint32_t{0x00F00F00U}), uint32_t{0x00800000U});
    static_assert(cb::extract_highest_set_bit(uint8_t{0xB4}) == uint8_t{0x80}, "highest set");
}

// ── extract_field ───────────────────────────────────────────────

TEST(BitUtils, ExtractFieldRuntime)
{
    // 0xB4 = 0b10110100; bits [2..4] -> 0b101 = 5
    EXPECT_EQ(cb::extract_field(uint8_t{0xB4}, 2, 3), uint8_t{0x05});
    EXPECT_EQ(cb::extract_field(uint8_t{0xFF}, 0, 4), uint8_t{0x0F});
    EXPECT_EQ(cb::extract_field(uint8_t{0xFF}, 4, 4), uint8_t{0x0F});
    EXPECT_EQ(cb::extract_field(uint32_t{0x12345678U}, 8, 8), uint32_t{0x56});
    static_assert(cb::extract_field(uint8_t{0xB4}, 2, 3) == uint8_t{0x05}, "field");
}

TEST(BitUtils, ExtractFieldEdgeCases)
{
    EXPECT_EQ(cb::extract_field(uint8_t{0xFF}, 0, 0), uint8_t{0x00}); // zero width
    EXPECT_EQ(cb::extract_field(uint8_t{0xFF}, 8, 3), uint8_t{0x00}); // start out of range
    EXPECT_EQ(cb::extract_field(uint8_t{0xFF}, 0, 8), uint8_t{0xFF}); // full width
    EXPECT_EQ(cb::extract_field(uint8_t{0xFF}, 2, 100), uint8_t{0x3F}); // width >= bits
}

TEST(BitUtils, ExtractFieldTemplate)
{
    EXPECT_EQ((cb::extract_field<2, 3>(uint8_t{0xB4})), uint8_t{0x05});
    EXPECT_EQ((cb::extract_field<0, 8>(uint8_t{0xFF})), uint8_t{0xFF});
    static_assert(cb::extract_field<2, 3>(uint8_t{0xB4}) == uint8_t{0x05}, "field tpl");
}

// ── insert_field ────────────────────────────────────────────────

TEST(BitUtils, InsertFieldRuntime)
{
    EXPECT_EQ(cb::insert_field(uint8_t{0x00}, uint8_t{0x05}, 2, 3), uint8_t{0x14});
    EXPECT_EQ(cb::insert_field(uint8_t{0xFF}, uint8_t{0x00}, 2, 3), uint8_t{0xE3});
    // field_val is clamped to width
    EXPECT_EQ(cb::insert_field(uint8_t{0x00}, uint8_t{0xFF}, 0, 4), uint8_t{0x0F});
}

TEST(BitUtils, InsertFieldEdgeCases)
{
    EXPECT_EQ(cb::insert_field(uint8_t{0xAB}, uint8_t{0x0F}, 0, 0), uint8_t{0xAB}); // zero width
    EXPECT_EQ(cb::insert_field(uint8_t{0xAB}, uint8_t{0x0F}, 8, 3), uint8_t{0xAB}); // start OOR
}

TEST(BitUtils, InsertFieldTemplate)
{
    EXPECT_EQ((cb::insert_field<2, 3>(uint8_t{0x00}, uint8_t{0x05})), uint8_t{0x14});
    static_assert(cb::insert_field<2, 3>(uint8_t{0x00}, uint8_t{0x05}) == uint8_t{0x14}, "insert tpl");
}

TEST(BitUtils, ExtractInsertRoundTrip)
{
    const uint32_t reg = 0xDEADBEEFU;
    const uint32_t field = cb::extract_field(reg, 8, 12);
    const uint32_t rebuilt = cb::insert_field(reg, field, 8, 12);
    EXPECT_EQ(rebuilt, reg);
    EXPECT_EQ(cb::extract_field(rebuilt, 8, 12), field);
}

} // namespace

#include <gtest/gtest.h>

#include "castle/bit/bit_count.h"

#include <cstdint>

namespace cb = castle::bit;

namespace
{

// ── popcount ────────────────────────────────────────────────────

TEST(BitCount, Popcount64Direct)
{
    EXPECT_EQ(cb::popcount(uint64_t{0}), 0u);
    EXPECT_EQ(cb::popcount(uint64_t{0xFFFFFFFFFFFFFFFFULL}), 64u);
    EXPECT_EQ(cb::popcount(uint64_t{0x1}), 1u);
    EXPECT_EQ(cb::popcount(uint64_t{0xAAAAAAAAAAAAAAAAULL}), 32u);
    EXPECT_EQ(cb::popcount(uint64_t{0x5555555555555555ULL}), 32u);
    static_assert(cb::popcount(uint64_t{0xFF}) == 8u, "popcount 0xFF");
}

TEST(BitCount, PopcountWidths)
{
    EXPECT_EQ(cb::popcount(uint8_t{0x00}), 0u);
    EXPECT_EQ(cb::popcount(uint8_t{0xFF}), 8u);
    EXPECT_EQ(cb::popcount(uint8_t{0xAA}), 4u);
    EXPECT_EQ(cb::popcount(uint16_t{0xFFFF}), 16u);
    EXPECT_EQ(cb::popcount(uint16_t{0x00FF}), 8u);
    EXPECT_EQ(cb::popcount(uint32_t{0xFFFFFFFFU}), 32u);
    EXPECT_EQ(cb::popcount(uint32_t{0x55555555U}), 16u);
}

TEST(BitCount, PopcountSignedUsesUnsignedRepr)
{
    // -1 has all bits set in its unsigned representation
    EXPECT_EQ(cb::popcount(int8_t{-1}), 8u);
    EXPECT_EQ(cb::popcount(int16_t{-1}), 16u);
}

// ── count_ones / count_zeros ────────────────────────────────────

TEST(BitCount, CountOnes)
{
    EXPECT_EQ(cb::count_ones(uint8_t{0xAA}), 4u);
    EXPECT_EQ(cb::count_ones(uint32_t{0xFFFFFFFFU}), 32u);
    static_assert(cb::count_ones(uint8_t{0x0F}) == 4u, "count_ones");
}

TEST(BitCount, CountZeros)
{
    EXPECT_EQ(cb::count_zeros(uint8_t{0x00}), 8u);
    EXPECT_EQ(cb::count_zeros(uint8_t{0xFF}), 0u);
    EXPECT_EQ(cb::count_zeros(uint8_t{0xAA}), 4u);
    EXPECT_EQ(cb::count_zeros(uint16_t{0x00FF}), 8u);
    EXPECT_EQ(cb::count_zeros(uint32_t{0}), 32u);
    static_assert(cb::count_zeros(uint8_t{0x0F}) == 4u, "count_zeros");
}

// ── count_leading_zeros ─────────────────────────────────────────

TEST(BitCount, CountLeadingZeros)
{
    EXPECT_EQ(cb::count_leading_zeros(uint8_t{0x00}), 8u);
    EXPECT_EQ(cb::count_leading_zeros(uint8_t{0x01}), 7u);
    EXPECT_EQ(cb::count_leading_zeros(uint8_t{0x80}), 0u);
    EXPECT_EQ(cb::count_leading_zeros(uint8_t{0xFF}), 0u);
    EXPECT_EQ(cb::count_leading_zeros(uint16_t{0x0001}), 15u);
    EXPECT_EQ(cb::count_leading_zeros(uint32_t{0x00000001U}), 31u);
    EXPECT_EQ(cb::count_leading_zeros(uint32_t{0x80000000U}), 0u);
    EXPECT_EQ(cb::count_leading_zeros(uint64_t{0x1}), 63u);
    EXPECT_EQ(cb::count_leading_zeros(uint64_t{0}), 64u);
    static_assert(cb::count_leading_zeros(uint8_t{0x01}) == 7u, "clz 0x01");
}

// ── count_trailing_zeros ────────────────────────────────────────

TEST(BitCount, CountTrailingZeros)
{
    EXPECT_EQ(cb::count_trailing_zeros(uint8_t{0x00}), 8u);
    EXPECT_EQ(cb::count_trailing_zeros(uint8_t{0x01}), 0u);
    EXPECT_EQ(cb::count_trailing_zeros(uint8_t{0x80}), 7u);
    EXPECT_EQ(cb::count_trailing_zeros(uint8_t{0x0C}), 2u); // 0b00001100
    EXPECT_EQ(cb::count_trailing_zeros(uint16_t{0x8000}), 15u);
    EXPECT_EQ(cb::count_trailing_zeros(uint32_t{0x80000000U}), 31u);
    EXPECT_EQ(cb::count_trailing_zeros(uint64_t{0x8000000000000000ULL}), 63u);
    EXPECT_EQ(cb::count_trailing_zeros(uint64_t{0}), 64u);
    static_assert(cb::count_trailing_zeros(uint8_t{0x80}) == 7u, "ctz 0x80");
}

// ── bit_width ───────────────────────────────────────────────────

TEST(BitCount, BitWidth)
{
    EXPECT_EQ(cb::bit_width(uint8_t{0x00}), 0u);
    EXPECT_EQ(cb::bit_width(uint8_t{0x01}), 1u);
    EXPECT_EQ(cb::bit_width(uint8_t{0x05}), 3u); // 0b101
    EXPECT_EQ(cb::bit_width(uint8_t{0x80}), 8u);
    EXPECT_EQ(cb::bit_width(uint8_t{0xFF}), 8u);
    EXPECT_EQ(cb::bit_width(uint32_t{0x00010000U}), 17u);
    static_assert(cb::bit_width(uint8_t{0x05}) == 3u, "bit_width 5");
}

// ── log2_floor ──────────────────────────────────────────────────

TEST(BitCount, Log2Floor)
{
    EXPECT_EQ(cb::log2_floor(uint8_t{0x01}), 0u);
    EXPECT_EQ(cb::log2_floor(uint8_t{0x08}), 3u);
    EXPECT_EQ(cb::log2_floor(uint8_t{0xFF}), 7u);
    EXPECT_EQ(cb::log2_floor(uint32_t{0x40000000U}), 30u);
    static_assert(cb::log2_floor(uint8_t{0x08}) == 3u, "log2_floor 8");
}

// ── parity ──────────────────────────────────────────────────────

TEST(BitCount, Parity)
{
    EXPECT_EQ(cb::parity(uint8_t{0x00}), 0u);
    EXPECT_EQ(cb::parity(uint8_t{0x07}), 1u); // 3 bits -> odd
    EXPECT_EQ(cb::parity(uint8_t{0x03}), 0u); // 2 bits -> even
    EXPECT_EQ(cb::parity(uint8_t{0xFF}), 0u); // 8 bits -> even
    EXPECT_EQ(cb::parity(uint8_t{0x01}), 1u);
    static_assert(cb::parity(uint8_t{0x07}) == 1u, "parity 0x07");
}

TEST(BitCount, ParityTemplate)
{
    EXPECT_EQ(cb::parity<0x07>(), 1u);
    EXPECT_EQ(cb::parity<0x03>(), 0u);
    static_assert(cb::parity<0xFF>() == 0u, "parity<0xFF>");
}

} // namespace

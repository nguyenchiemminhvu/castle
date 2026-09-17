#include <gtest/gtest.h>

#include "castle/bit/bit_core.h"

#include <cstdint>

namespace cb = castle::bit;

namespace
{

// ── test ────────────────────────────────────────────────────────

TEST(BitCore, TestRuntime)
{
    const uint8_t v = 0x0A; // 0b00001010
    EXPECT_FALSE(cb::test(v, 0));
    EXPECT_TRUE(cb::test(v, 1));
    EXPECT_FALSE(cb::test(v, 2));
    EXPECT_TRUE(cb::test(v, 3));
    EXPECT_FALSE(cb::test(v, 4));
}

TEST(BitCore, TestOutOfRangeReturnsFalse)
{
    EXPECT_FALSE(cb::test(uint8_t{0xFF}, 8));
    EXPECT_FALSE(cb::test(uint8_t{0xFF}, 100));
    EXPECT_FALSE(cb::test(uint16_t{0xFFFF}, 16));
}

TEST(BitCore, TestWidths)
{
    EXPECT_TRUE(cb::test(uint16_t{0x8000}, 15));
    EXPECT_TRUE(cb::test(uint32_t{0x80000000U}, 31));
    EXPECT_TRUE(cb::test(uint64_t{0x8000000000000000ULL}, 63));
    EXPECT_FALSE(cb::test(uint64_t{0}, 0));
}

TEST(BitCore, TestSigned)
{
    const int8_t v = static_cast<int8_t>(0x80); // MSB set
    EXPECT_TRUE(cb::test(v, 7));
    EXPECT_FALSE(cb::test(v, 0));
}

TEST(BitCore, TestTemplateIndex)
{
    const uint8_t v = 0x0A;
    EXPECT_FALSE(cb::test<0>(v));
    EXPECT_TRUE(cb::test<1>(v));
    EXPECT_TRUE(cb::test<3>(v));
    static_assert(cb::test<1>(uint8_t{0x0A}), "bit 1 set");
    static_assert(!cb::test<0>(uint8_t{0x0A}), "bit 0 clear");
}

// ── set ─────────────────────────────────────────────────────────

TEST(BitCore, SetRuntime)
{
    EXPECT_EQ(cb::set(uint8_t{0x00}, 3), uint8_t{0x08});
    EXPECT_EQ(cb::set(uint8_t{0x08}, 3), uint8_t{0x08}); // already set
    EXPECT_EQ(cb::set(uint8_t{0xFF}, 2), uint8_t{0xFF});
    EXPECT_EQ(cb::set(uint32_t{0}, 31), uint32_t{0x80000000U});
    EXPECT_EQ(cb::set(uint64_t{0}, 63), uint64_t{0x8000000000000000ULL});
}

TEST(BitCore, SetOutOfRangeUnchanged)
{
    EXPECT_EQ(cb::set(uint8_t{0x0A}, 8), uint8_t{0x0A});
    EXPECT_EQ(cb::set(uint8_t{0x0A}, 99), uint8_t{0x0A});
}

TEST(BitCore, SetTemplateIndex)
{
    EXPECT_EQ(cb::set<0>(uint8_t{0x00}), uint8_t{0x01});
    EXPECT_EQ(cb::set<7>(uint8_t{0x00}), uint8_t{0x80});
    static_assert(cb::set<3>(uint8_t{0}) == uint8_t{0x08}, "set bit 3");
}

// ── clear ───────────────────────────────────────────────────────

TEST(BitCore, ClearRuntime)
{
    EXPECT_EQ(cb::clear(uint8_t{0xFF}, 0), uint8_t{0xFE});
    EXPECT_EQ(cb::clear(uint8_t{0xFF}, 7), uint8_t{0x7F});
    EXPECT_EQ(cb::clear(uint8_t{0x00}, 3), uint8_t{0x00}); // already clear
    EXPECT_EQ(cb::clear(uint32_t{0xFFFFFFFFU}, 31), uint32_t{0x7FFFFFFFU});
}

TEST(BitCore, ClearOutOfRangeUnchanged)
{
    EXPECT_EQ(cb::clear(uint8_t{0xFF}, 8), uint8_t{0xFF});
    EXPECT_EQ(cb::clear(uint8_t{0xFF}, 42), uint8_t{0xFF});
}

TEST(BitCore, ClearTemplateIndex)
{
    EXPECT_EQ(cb::clear<0>(uint8_t{0xFF}), uint8_t{0xFE});
    EXPECT_EQ(cb::clear<7>(uint8_t{0xFF}), uint8_t{0x7F});
    static_assert(cb::clear<0>(uint8_t{0xFF}) == uint8_t{0xFE}, "clear bit 0");
}

// ── toggle ──────────────────────────────────────────────────────

TEST(BitCore, ToggleRuntime)
{
    EXPECT_EQ(cb::toggle(uint8_t{0x00}, 0), uint8_t{0x01});
    EXPECT_EQ(cb::toggle(uint8_t{0x01}, 0), uint8_t{0x00});
    EXPECT_EQ(cb::toggle(uint8_t{0xFF}, 3), uint8_t{0xF7});
    EXPECT_EQ(cb::toggle(uint64_t{0}, 63), uint64_t{0x8000000000000000ULL});
}

TEST(BitCore, ToggleOutOfRangeUnchanged)
{
    EXPECT_EQ(cb::toggle(uint8_t{0x0A}, 8), uint8_t{0x0A});
}

TEST(BitCore, ToggleTwiceIsIdentity)
{
    const uint16_t v = 0x1234;
    EXPECT_EQ(cb::toggle(cb::toggle(v, 5), 5), v);
}

TEST(BitCore, ToggleTemplateIndex)
{
    EXPECT_EQ(cb::toggle<3>(uint8_t{0xFF}), uint8_t{0xF7});
    static_assert(cb::toggle<0>(uint8_t{0}) == uint8_t{0x01}, "toggle bit 0");
}

} // namespace

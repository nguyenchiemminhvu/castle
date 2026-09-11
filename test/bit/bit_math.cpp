#include <gtest/gtest.h>

#include "castle/bit/bit_math.h"

#include <cstdint>

namespace cb = castle::bit;

namespace
{

// ── is_even / is_odd ────────────────────────────────────────────

TEST(BitMath, IsEvenOdd)
{
    EXPECT_TRUE(cb::is_even(uint8_t{0}));
    EXPECT_TRUE(cb::is_even(uint8_t{4}));
    EXPECT_FALSE(cb::is_even(uint8_t{5}));
    EXPECT_TRUE(cb::is_odd(uint8_t{5}));
    EXPECT_FALSE(cb::is_odd(uint8_t{4}));
    EXPECT_TRUE(cb::is_odd(int32_t{-1}));
    static_assert(cb::is_even(uint8_t{4}), "4 even");
    static_assert(cb::is_odd(uint8_t{5}), "5 odd");
}

TEST(BitMath, IsEvenOddConst)
{
    static_assert(cb::is_even_const<4>::value, "4 even const");
    static_assert(!cb::is_even_const<5>::value, "5 not even const");
    static_assert(cb::is_odd_const<7>::value, "7 odd const");
    EXPECT_TRUE(cb::is_even_const<10>::value);
    EXPECT_TRUE(cb::is_odd_const<11>::value);
}

// ── is_power_of_two ─────────────────────────────────────────────

TEST(BitMath, IsPowerOfTwo)
{
    EXPECT_FALSE(cb::is_power_of_two(uint8_t{0}));
    EXPECT_TRUE(cb::is_power_of_two(uint8_t{1}));
    EXPECT_TRUE(cb::is_power_of_two(uint8_t{2}));
    EXPECT_TRUE(cb::is_power_of_two(uint8_t{4}));
    EXPECT_TRUE(cb::is_power_of_two(uint8_t{128}));
    EXPECT_FALSE(cb::is_power_of_two(uint8_t{3}));
    EXPECT_FALSE(cb::is_power_of_two(uint8_t{6}));
    EXPECT_FALSE(cb::is_power_of_two(int8_t{-4})); // negative
    static_assert(cb::is_power_of_two(uint32_t{1024}), "1024 pow2");
    static_assert(!cb::is_power_of_two(uint32_t{1000}), "1000 not pow2");
}

TEST(BitMath, IsPowerOfTwoConst)
{
    static_assert(cb::is_power_of_two_const<16>::value, "16 pow2");
    static_assert(!cb::is_power_of_two_const<0>::value, "0 not pow2");
    static_assert(!cb::is_power_of_two_const<15>::value, "15 not pow2");
    EXPECT_TRUE(cb::is_power_of_two_const<64>::value);
}

// ── next_power_of_two ───────────────────────────────────────────

TEST(BitMath, NextPowerOfTwo)
{
    EXPECT_EQ(cb::next_power_of_two(uint32_t{0}), uint32_t{1});
    EXPECT_EQ(cb::next_power_of_two(uint32_t{1}), uint32_t{1});
    EXPECT_EQ(cb::next_power_of_two(uint32_t{3}), uint32_t{4});
    EXPECT_EQ(cb::next_power_of_two(uint32_t{5}), uint32_t{8});
    EXPECT_EQ(cb::next_power_of_two(uint32_t{16}), uint32_t{16});
    EXPECT_EQ(cb::next_power_of_two(uint32_t{17}), uint32_t{32});
    EXPECT_EQ(cb::next_power_of_two(uint8_t{100}), uint8_t{128});
    static_assert(cb::next_power_of_two(uint32_t{5}) == uint32_t{8}, "next pow2 5");
}

TEST(BitMath, NextPowerOfTwoTemplate)
{
    EXPECT_EQ(cb::next_power_of_two<5>(), static_cast<castle::size_type>(8));
    EXPECT_EQ(cb::next_power_of_two<0>(), static_cast<castle::size_type>(1));
    static_assert(cb::next_power_of_two<17>() == 32u, "next pow2 17");
    static_assert(cb::next_power_of_two_const<9>::value == 16u, "next const 9");
}

// ── previous_power_of_two ───────────────────────────────────────

TEST(BitMath, PreviousPowerOfTwo)
{
    EXPECT_EQ(cb::previous_power_of_two(uint32_t{0}), uint32_t{0});
    EXPECT_EQ(cb::previous_power_of_two(uint32_t{1}), uint32_t{1});
    EXPECT_EQ(cb::previous_power_of_two(uint32_t{5}), uint32_t{4});
    EXPECT_EQ(cb::previous_power_of_two(uint32_t{16}), uint32_t{16});
    EXPECT_EQ(cb::previous_power_of_two(uint32_t{17}), uint32_t{16});
    EXPECT_EQ(cb::previous_power_of_two(uint8_t{200}), uint8_t{128});
    static_assert(cb::previous_power_of_two(uint32_t{17}) == uint32_t{16}, "prev pow2 17");
}

TEST(BitMath, PreviousPowerOfTwoTemplate)
{
    EXPECT_EQ(cb::previous_power_of_two<17>(), static_cast<castle::size_type>(16));
    EXPECT_EQ(cb::previous_power_of_two<0>(), static_cast<castle::size_type>(0));
    static_assert(cb::previous_power_of_two<5>() == 4u, "prev pow2 5");
    static_assert(cb::previous_power_of_two_const<100>::value == 64u, "prev const 100");
}

// ── align_up / align_down / is_aligned ──────────────────────────

TEST(BitMath, AlignUp)
{
    EXPECT_EQ(cb::align_up(uint32_t{5}, uint32_t{4}), uint32_t{8});
    EXPECT_EQ(cb::align_up(uint32_t{8}, uint32_t{4}), uint32_t{8});
    EXPECT_EQ(cb::align_up(uint32_t{0}, uint32_t{16}), uint32_t{0});
    EXPECT_EQ(cb::align_up(uint32_t{1}, uint32_t{16}), uint32_t{16});
    EXPECT_EQ(cb::align_up(uint32_t{17}, uint32_t{8}), uint32_t{24});
    static_assert(cb::align_up(uint32_t{5}, uint32_t{4}) == uint32_t{8}, "align_up 5,4");
}

TEST(BitMath, AlignDown)
{
    EXPECT_EQ(cb::align_down(uint32_t{5}, uint32_t{4}), uint32_t{4});
    EXPECT_EQ(cb::align_down(uint32_t{8}, uint32_t{4}), uint32_t{8});
    EXPECT_EQ(cb::align_down(uint32_t{17}, uint32_t{8}), uint32_t{16});
    EXPECT_EQ(cb::align_down(uint32_t{15}, uint32_t{16}), uint32_t{0});
    static_assert(cb::align_down(uint32_t{5}, uint32_t{4}) == uint32_t{4}, "align_down 5,4");
}

TEST(BitMath, IsAligned)
{
    EXPECT_TRUE(cb::is_aligned(uint32_t{8}, uint32_t{4}));
    EXPECT_FALSE(cb::is_aligned(uint32_t{5}, uint32_t{4}));
    EXPECT_TRUE(cb::is_aligned(uint32_t{0}, uint32_t{16}));
    EXPECT_TRUE(cb::is_aligned(uint32_t{16}, uint32_t{16}));
    static_assert(cb::is_aligned(uint32_t{8}, uint32_t{4}), "8 aligned to 4");
    static_assert(!cb::is_aligned(uint32_t{5}, uint32_t{4}), "5 not aligned to 4");
}

// ── sign ────────────────────────────────────────────────────────

TEST(BitMath, Sign)
{
    EXPECT_EQ(cb::sign(int32_t{-5}), -1);
    EXPECT_EQ(cb::sign(int32_t{0}), 0);
    EXPECT_EQ(cb::sign(int32_t{5}), 1);
    EXPECT_EQ(cb::sign(int8_t{-1}), -1);
    EXPECT_EQ(cb::sign(uint32_t{7}), 1);
    EXPECT_EQ(cb::sign(uint32_t{0}), 0);
    static_assert(cb::sign(int32_t{-5}) == -1, "sign -5");
    static_assert(cb::sign(int32_t{5}) == 1, "sign 5");
}

TEST(BitMath, SignTemplate)
{
    EXPECT_EQ(cb::sign<5>(), 1);
    EXPECT_EQ(cb::sign<0>(), 0);
    static_assert(cb::sign<9>() == 1, "sign<9>");
    static_assert(cb::sign<0>() == 0, "sign<0>");
}

} // namespace

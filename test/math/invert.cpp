#include <gtest/gtest.h>

#include "castle/math/invert.h"

#include <stdint.h>

namespace
{

// ---------------------------------------------------------------------------
// invert<T> - default construction
//   signed   : offset 0, minuend 0        -> invert(v) == -v
//   unsigned : offset 0, minuend max()    -> invert(v) == max - v
// ---------------------------------------------------------------------------
TEST(MathInvert, DefaultSigned)
{
    constexpr castle::math::invert<int> inv;
    static_assert(inv.offset() == 0, "");
    static_assert(inv.minuend() == 0, "");
    static_assert(inv(5) == -5, "");
    static_assert(inv(-3) == 3, "");
    static_assert(inv(0) == 0, "");
    SUCCEED();
}

TEST(MathInvert, DefaultUnsigned)
{
    constexpr castle::math::invert<uint8_t> inv;
    static_assert(inv.offset() == 0, "");
    static_assert(inv.minuend() == 255, "");
    static_assert(inv(0) == 255, "");
    static_assert(inv(255) == 0, "");
    static_assert(inv(10) == 245, "");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// invert<T> - explicit offset / minuend
//   invert(v) == minuend - (v - offset)
// ---------------------------------------------------------------------------
TEST(MathInvert, ExplicitParameters)
{
    constexpr castle::math::invert<int> inv(1, 10); // 11 - v
    static_assert(inv.offset() == 1, "");
    static_assert(inv.minuend() == 10, "");
    static_assert(inv(1) == 10, "");
    static_assert(inv(3) == 8, "");
    static_assert(inv(10) == 1, "");
    SUCCEED();
}

TEST(MathInvert, RangeMirror)
{
    // Mirror the inclusive range [0, 100] about its centre.
    constexpr castle::math::invert<int> inv(0, 100);
    static_assert(inv(0) == 100, "");
    static_assert(inv(100) == 0, "");
    static_assert(inv(25) == 75, "");
    static_assert(inv(50) == 50, "");
    SUCCEED();
}

TEST(MathInvert, RuntimeUsage)
{
    castle::math::invert<uint16_t> inv;
    EXPECT_EQ(inv(0), 65535);
    EXPECT_EQ(inv(65535), 0);

    castle::math::invert<int> shifted(10, 20);
    EXPECT_EQ(shifted(10), 20);
    EXPECT_EQ(shifted(20), 10);
    EXPECT_EQ(shifted(15), 15);
}

TEST(MathInvert, Involution)
{
    // Applying the same inversion twice restores the original value.
    constexpr castle::math::invert<int> inv(0, 100);
    static_assert(inv(inv(37)) == 37, "");
    SUCCEED();
}

} // namespace

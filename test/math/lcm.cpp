#include <gtest/gtest.h>

#include "castle/math/lcm.h"

#include <stdint.h>

namespace
{

// ---------------------------------------------------------------------------
// lcm<A, B>::value - compile-time least common multiple (A > 0, B > 0)
// ---------------------------------------------------------------------------
TEST(MathLcm, CommonCases)
{
    static_assert(castle::math::lcm<4, 6>::value == 12, "");
    static_assert(castle::math::lcm<3, 5>::value == 15, "");
    static_assert(castle::math::lcm<6, 8>::value == 24, "");
    static_assert(castle::math::lcm<21, 6>::value == 42, "");
    SUCCEED();
}

TEST(MathLcm, Coprime)
{
    // For coprime values the lcm is the product.
    static_assert(castle::math::lcm<7, 5>::value == 35, "");
    static_assert(castle::math::lcm<9, 4>::value == 36, "");
    SUCCEED();
}

TEST(MathLcm, Divisible)
{
    // When one divides the other, the lcm is the larger value.
    static_assert(castle::math::lcm<3, 9>::value == 9, "");
    static_assert(castle::math::lcm<5, 20>::value == 20, "");
    SUCCEED();
}

TEST(MathLcm, Identity)
{
    static_assert(castle::math::lcm<7, 7>::value == 7, "");
    static_assert(castle::math::lcm<1, 1>::value == 1, "");
    static_assert(castle::math::lcm<1, 12>::value == 12, "");
    SUCCEED();
}

TEST(MathLcm, Symmetry)
{
    static_assert(castle::math::lcm<4, 6>::value == castle::math::lcm<6, 4>::value, "");
    SUCCEED();
}

TEST(MathLcm, RuntimeReadable)
{
    const intmax_t v = castle::math::lcm<21, 6>::value;
    EXPECT_EQ(v, 42);
}

} // namespace

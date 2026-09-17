
#include <gtest/gtest.h>

#include "castle/math/is_power_of_two.h"

#include <cstdint>

namespace
{

TEST(MathIsPowerOfTwo, ZeroAndPowers)
{
    EXPECT_FALSE(castle::math::is_power_of_two(uint32_t{0}));
    EXPECT_TRUE(castle::math::is_power_of_two(uint32_t{1}));
    EXPECT_TRUE(castle::math::is_power_of_two(uint32_t{2}));
    EXPECT_TRUE(castle::math::is_power_of_two(uint32_t{4}));
    EXPECT_TRUE(castle::math::is_power_of_two(uint32_t{1024}));
    static_assert(castle::math::is_power_of_two(uint32_t{1024}), "power of two");
}

TEST(MathIsPowerOfTwo, NonPowersAndSignedNegative)
{
    EXPECT_FALSE(castle::math::is_power_of_two(uint32_t{3}));
    EXPECT_FALSE(castle::math::is_power_of_two(uint32_t{6}));
    EXPECT_FALSE(castle::math::is_power_of_two(int32_t{-4}));
    EXPECT_FALSE(castle::math::is_power_of_two(int32_t{-1}));
    static_assert(!castle::math::is_power_of_two(uint32_t{1000}), "not power of two");
}

} // namespace

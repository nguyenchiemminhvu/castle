
#include <gtest/gtest.h>

#include "castle/math/mod.h"

namespace
{

TEST(MathMod, PositiveModulo)
{
    EXPECT_EQ(castle::math::positive_mod(7, 5), 2);
    EXPECT_EQ(castle::math::positive_mod(5, 5), 0);
    EXPECT_EQ(castle::math::positive_mod(0, 5), 0);
    EXPECT_EQ(castle::math::positive_mod(-1, 5), 4);
    EXPECT_EQ(castle::math::positive_mod(-11, 5), 4);
    static_assert(castle::math::positive_mod(-11, 5) == 4, "positive_mod");
}

TEST(MathMod, WrapsIntoHalfOpenInterval)
{
    EXPECT_EQ(castle::math::wrap(0, 0, 10), 0);
    EXPECT_EQ(castle::math::wrap(5, 0, 10), 5);
    EXPECT_EQ(castle::math::wrap(9, 0, 10), 9);
    EXPECT_EQ(castle::math::wrap(10, 0, 10), 0);
    EXPECT_EQ(castle::math::wrap(12, 0, 10), 2);
    EXPECT_EQ(castle::math::wrap(-1, 0, 10), 9);
    EXPECT_EQ(castle::math::wrap(-12, 0, 10), 8);
}

TEST(MathMod, NarrowUnsignedWideningPath)
{
    EXPECT_EQ(castle::math::wrap<uint8_t>(250U, 250U, static_cast<uint8_t>(255U)), 250U);
    EXPECT_EQ(castle::math::wrap<uint8_t>(252U, 250U, static_cast<uint8_t>(255U)), 252U);
    EXPECT_EQ(castle::math::wrap<uint8_t>(255U, 250U, static_cast<uint8_t>(255U)), 250U);
}

} // namespace

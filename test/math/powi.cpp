
#include <gtest/gtest.h>

#include "castle/math/powi.h"

namespace
{

TEST(MathPowi, ZeroAndOne)
{
    EXPECT_EQ(castle::math::powi(7, 0U), 1);
    EXPECT_EQ(castle::math::powi(7, 1U), 7);
    static_assert(castle::math::powi(7, 0U) == 1, "powi zero exponent");
}

TEST(MathPowi, EvenAndOddExponentBits)
{
    EXPECT_EQ(castle::math::powi(3, 2U), 9);
    EXPECT_EQ(castle::math::powi(3, 3U), 27);
    EXPECT_EQ(castle::math::powi(2, 10U), 1024);
    EXPECT_DOUBLE_EQ(castle::math::powi(1.5, 4U), 5.0625);
    static_assert(castle::math::powi(3, 5U) == 243, "powi");
}

} // namespace

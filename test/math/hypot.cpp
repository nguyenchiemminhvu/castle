
#include <gtest/gtest.h>

#include "castle/math/hypot.h"

namespace
{

TEST(MathHypot, TwoDimensionalZeroAndAxes)
{
    EXPECT_DOUBLE_EQ(castle::math::hypot(0.0, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::hypot(3.0, 0.0), 3.0);
    EXPECT_DOUBLE_EQ(castle::math::hypot(0.0, 4.0), 4.0);
    EXPECT_DOUBLE_EQ(castle::math::hypot(-3.0, 4.0), 5.0);
}

TEST(MathHypot, TwoDimensionalScaleSelection)
{
    EXPECT_NEAR(castle::math::hypot(3.0, 4.0), 5.0, 1e-12);
    EXPECT_NEAR(castle::math::hypot(4.0, 3.0), 5.0, 1e-12);
    EXPECT_NEAR(castle::math::hypot(1.0e200, 1.0e200), 1.4142135623730951e200, 1e187);
    static_assert(castle::math::hypot(3.0, 4.0) > 4.99, "hypot constexpr");
}

TEST(MathHypot, ThreeDimensionalZeroAndAxes)
{
    EXPECT_DOUBLE_EQ(castle::math::hypot(0.0, 0.0, 0.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::hypot(3.0, 0.0, 0.0), 3.0);
    EXPECT_DOUBLE_EQ(castle::math::hypot(0.0, 4.0, 0.0), 4.0);
    EXPECT_DOUBLE_EQ(castle::math::hypot(0.0, 0.0, 5.0), 5.0);
}

TEST(MathHypot, ThreeDimensionalScaleSelection)
{
    EXPECT_NEAR(castle::math::hypot(2.0, 3.0, 6.0), 7.0, 1e-12);
    EXPECT_NEAR(castle::math::hypot(6.0, 3.0, 2.0), 7.0, 1e-12);
    EXPECT_NEAR(castle::math::hypot(3.0, 6.0, 2.0), 7.0, 1e-12);
    static_assert(castle::math::hypot(2.0, 3.0, 6.0) > 6.99, "hypot constexpr");
}

} // namespace

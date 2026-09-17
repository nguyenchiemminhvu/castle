
#include <gtest/gtest.h>

#include "castle/math/sqrt_real.h"

namespace
{

TEST(MathSqrtReal, ZeroAndBasicValues)
{
    EXPECT_DOUBLE_EQ(castle::math::sqrt_real(0.0), 0.0);
    EXPECT_NEAR(castle::math::sqrt_real(1.0), 1.0, 1e-12);
    EXPECT_NEAR(castle::math::sqrt_real(2.0), 1.4142135623730951, 1e-12);
    static_assert(castle::math::sqrt_real(9.0) > 2.999, "sqrt_real constexpr");
}

TEST(MathSqrtReal, RangeReductionDown)
{
    EXPECT_NEAR(castle::math::sqrt_real(4096.0), 64.0, 1e-10);
    EXPECT_NEAR(castle::math::sqrt_real(1.0e20), 1.0e10, 1.0e-3);
}

TEST(MathSqrtReal, RangeReductionUp)
{
    EXPECT_NEAR(castle::math::sqrt_real(0.0625), 0.25, 1e-12);
    EXPECT_NEAR(castle::math::sqrt_real(1.0e-20), 1.0e-10, 1e-16);
}

TEST(MathSqrtReal, FloatOverload)
{
    EXPECT_NEAR(castle::math::sqrt_real(81.0f), 9.0f, 1e-5f);
}

} // namespace

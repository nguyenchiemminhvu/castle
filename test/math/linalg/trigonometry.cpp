#include <gtest/gtest.h>

#include "castle/math/linalg/trigonometry.h"

TEST(Trigonometry, BasicFloatDoubleAndLongDoubleWrappers)
{
    EXPECT_FLOAT_EQ(castle::math::sin(0.0F), 0.0F);
    EXPECT_FLOAT_EQ(castle::math::cos(0.0F), 1.0F);
    EXPECT_DOUBLE_EQ(castle::math::sin(0.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::cos(0.0), 1.0);
    EXPECT_EQ(castle::math::sin(static_cast<long double>(0.0)), static_cast<long double>(0.0));
    EXPECT_EQ(castle::math::cos(static_cast<long double>(0.0)), static_cast<long double>(1.0));
}

TEST(Trigonometry, TypedRadiansWrappers)
{
    constexpr float half_pi = 1.57079632679F;
    EXPECT_NEAR(castle::math::radians_sin(half_pi), 1.0F, 1.0e-5F);
    EXPECT_NEAR(castle::math::radians_cos(half_pi), 0.0F, 1.0e-5F);
}

TEST(Trigonometry, SinCosPair)
{
    constexpr double half_pi = 1.5707963267948966;
    const castle::math::sin_cos_result<double> result = castle::math::sin_cos(half_pi);

    EXPECT_NEAR(result.sine, 1.0, 1.0e-12);
    EXPECT_NEAR(result.cosine, 0.0, 1.0e-12);
}

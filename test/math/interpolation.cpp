
#include <gtest/gtest.h>

#include "castle/math/interpolation.hpp"

namespace
{

TEST(MathInterpolation, UmbrellaExposesScalarKernels)
{
    EXPECT_DOUBLE_EQ(castle::math::lerp(0.0, 10.0, 0.5), 5.0);
    EXPECT_NEAR(castle::math::catmull_rom(0.0, 1.0, 2.0, 3.0, 0.5), 1.5, 1e-12);
    EXPECT_NEAR(castle::math::bilinear(0.0, 1.0, 2.0, 3.0, 0.5, 0.5), 1.5, 1e-12);
    EXPECT_NEAR(castle::math::cubic_bezier(0.0, 0.0, 1.0, 1.0, 0.5), 0.5, 1e-12);
    EXPECT_NEAR(castle::math::cubic_hermite(0.0, 0.0, 1.0, 0.0, 0.5), 0.5, 1e-12);
    EXPECT_NEAR(castle::math::cubic_hermite_derivative(0.0, 1.0, 1.0, 1.0, 0.0), 1.0, 1e-12);
}

TEST(MathInterpolation, UmbrellaExposesLinearTable)
{
    const float xs[] = {0.0F, 1.0F, 2.0F};
    const float ys[] = {0.0F, 2.0F, 8.0F};
    const castle::math::linear_table<float> table(xs, ys, 3U);
    ASSERT_TRUE(table.valid());
    EXPECT_NEAR(table.evaluate(1.5F), 5.0F, 1e-5F);
    EXPECT_NEAR(table.evaluate(3.0F, castle::math::extrapolation::linear), 14.0F, 1e-5F);
    EXPECT_FLOAT_EQ(table(-1.0F), 0.0F);
}

TEST(MathInterpolation, UmbrellaExposesCubicSpline)
{
    const double xs[] = {0.0, 1.0, 2.0, 3.0};
    const double ys[] = {0.0, 1.0, 8.0, 27.0};
    castle::math::cubic_spline<double, 8U> spline;
    ASSERT_EQ(spline.build(xs, ys, 4U, castle::math::spline_kind::clamped, 0.0, 27.0), castle::status::ok);
    EXPECT_NEAR(spline.evaluate(1.5), 3.375, 1e-9);
    EXPECT_NEAR(spline.derivative(1.5), 6.75, 1e-9);
    EXPECT_EQ(spline.build(xs, ys, 1U), castle::status::invalid_argument);
    EXPECT_FALSE(spline.valid());
}

} // namespace

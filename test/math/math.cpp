#include <gtest/gtest.h>

#include "castle/math/math.hpp"

#include <stdint.h>

namespace
{

// ---------------------------------------------------------------------------
// math.h is an umbrella header: the aggregate API must be reachable.
// ---------------------------------------------------------------------------
TEST(MathUmbrella, AggregateApiExposed)
{
    // fib / abs / clamp / sqrt / gcd / lcm / logarithm / ratio all visible.
    EXPECT_EQ(castle::math::fib(10u), 55u);
    EXPECT_EQ(castle::math::abs<int>(-4), 4);
    EXPECT_EQ(castle::math::clamp(12, 0, 10), 10);
    EXPECT_EQ(castle::sqrt<144U>::value, 12u);

    static_assert(castle::math::gcd<48, 36>::value == 12, "");
    static_assert(castle::math::lcm<4, 6>::value == 12, "");
    static_assert(castle::math::logarithm<8, 2>::value == 3u, "");
    static_assert(castle::math::ratio<2, 4>::num == 1, "");
    SUCCEED();
}

TEST(MathUmbrella, ComplexInterpolationAndNumericExposed)
{
    // complex / exponential
    const castle::math::complex<double> i_unit(0.0, 1.0);
    const castle::math::complex<double> squared = i_unit * i_unit;
    EXPECT_NEAR(squared.real(), -1.0, 1.0e-12);
    EXPECT_NEAR(squared.imag(), 0.0, 1.0e-12);
    EXPECT_NEAR(castle::math::exp(0.0), 1.0, 1.0e-12);

    // interpolation
    const double xs[3] = {0.0, 1.0, 2.0};
    const double ys[3] = {0.0, 10.0, 20.0};
    const castle::math::linear_table<double> table(xs, ys, 3U);
    EXPECT_TRUE(table.valid());
    EXPECT_NEAR(table(0.5), 5.0, 1.0e-12);
    EXPECT_NEAR(castle::math::lerp(0.0, 10.0, 0.25), 2.5, 1.0e-12);

    // numeric
    const auto square = [](double x) { return x * x; };
    EXPECT_NEAR(castle::math::integrate_simpson(square, 0.0, 3.0, 100U), 9.0, 1.0e-9);

    const auto shifted = [](double x) { return (x * x) - 2.0; };
    const auto root = castle::math::find_root_bisection(shifted, 0.0, 2.0, 1.0e-12);
    EXPECT_TRUE(root.ok());
    EXPECT_NEAR(root.value, 1.4142135623730951, 1.0e-9);

    const auto decay = [](double, double y) { return -y; };
    EXPECT_NEAR(castle::math::rk4_step(decay, 0.0, 1.0, 0.1), 0.9048375, 1.0e-7);
}

// ---------------------------------------------------------------------------
// is_equal - floating point equality within epsilon
// ---------------------------------------------------------------------------
TEST(MathIsEqual, Equal)
{
    EXPECT_TRUE(castle::math::is_equal(1.0, 1.0));
    EXPECT_TRUE(castle::math::is_equal(0.0, 0.0));
    EXPECT_TRUE(castle::math::is_equal(-2.5, -2.5));
    EXPECT_TRUE(castle::math::is_equal(3.5f, 3.5f));

    // Differences far smaller than epsilon compare equal.
    EXPECT_TRUE(castle::math::is_equal(1.0, 1.0 + 1e-18));
    EXPECT_TRUE(castle::math::is_equal(1.0f, 1.0f + 1e-9f));
}

TEST(MathIsEqual, NotEqual)
{
    EXPECT_FALSE(castle::math::is_equal(1.0, 2.0));
    EXPECT_FALSE(castle::math::is_equal(0.0, 1.0));
    EXPECT_FALSE(castle::math::is_equal(1.0f, 1.5f));
}

// ---------------------------------------------------------------------------
// is_zero - magnitude within epsilon of zero
// ---------------------------------------------------------------------------
TEST(MathIsZero, ZeroAndTiny)
{
    EXPECT_TRUE(castle::math::is_zero(0.0));
    EXPECT_TRUE(castle::math::is_zero(-0.0));
    EXPECT_TRUE(castle::math::is_zero(1e-18));
    EXPECT_TRUE(castle::math::is_zero(-1e-18));
}

TEST(MathIsZero, NonZero)
{
    EXPECT_FALSE(castle::math::is_zero(1.0));
    EXPECT_FALSE(castle::math::is_zero(-0.5));
    EXPECT_FALSE(castle::math::is_zero(0.001f));
}

} // namespace

#include <gtest/gtest.h>

#include "castle/math/numeric.hpp"

namespace
{

// ---------------------------------------------------------------------------
// numeric.hpp is an umbrella header: the aggregate API must be reachable.
// ---------------------------------------------------------------------------
TEST(MathNumeric, IntegrationAndDifferentiationExposed)
{
    const auto square = [](double x) { return x * x; };
    EXPECT_NEAR(castle::math::integrate_simpson(square, 0.0, 3.0, 100U), 9.0, 1.0e-9);
    EXPECT_NEAR(castle::math::derivative_central(square, 3.0, 1.0e-4), 6.0, 1.0e-6);
}

TEST(MathNumeric, RootFindingExposed)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto df = [](double x) { return 2.0 * x; };

    const castle::math::numeric_result<double> bisection = castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-12);
    EXPECT_TRUE(bisection.ok());
    EXPECT_NEAR(bisection.value, 1.4142135623730951, 1.0e-11);

    const auto newton = castle::math::find_root_newton(f, df, 1.0, 1.0e-12);
    EXPECT_TRUE(newton.ok());
    EXPECT_NEAR(newton.value, 1.4142135623730951, 1.0e-12);

    const auto secant = castle::math::find_root_secant(f, 1.0, 2.0, 1.0e-12);
    EXPECT_TRUE(secant.ok());
    EXPECT_NEAR(secant.value, 1.4142135623730951, 1.0e-11);
}

TEST(MathNumeric, OdeExposed)
{
    const auto decay = [](double, double y) { return -y; };
    double y = 1.0;
    for (int i = 0; i < 10; ++i)
    {
        y = castle::math::rk4_step(decay, 0.1 * static_cast<double>(i), y, 0.1);
    }
    EXPECT_NEAR(y, 0.36787944117144233, 1.0e-6);
    EXPECT_NEAR(castle::math::euler_step(decay, 0.0, 1.0, 0.5), 0.5, 1.0e-15);
}

} // namespace

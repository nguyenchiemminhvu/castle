
#include <gtest/gtest.h>

#include "castle/math/exponential.hpp"
#include "castle/math/numeric/differentiation.hpp"

namespace
{

constexpr double kE = 2.718281828459045;

double cube_fn(double x)
{
    return x * x * x;
}

float square_fn_f(float x)
{
    return x * x;
}

double exp_fn(double x)
{
    return castle::math::exp(x);
}

struct quartic_functor
{
    double operator()(double x) const
    {
        return x * x * x * x;
    }
};

struct counting_functor
{
    unsigned* calls;

    double operator()(double x) const
    {
        ++(*calls);
        return x * x;
    }
};

// ---------------------------------------------------------------------------
// derivative_forward / derivative_backward
// ---------------------------------------------------------------------------

TEST(MathNumericDifferentiation, ForwardDifferenceOnCubic)
{
    // (f(x+h) - f(x)) / h = 3x^2 + 3xh + h^2 -> 12 + 6h + h^2 at x = 2.
    EXPECT_NEAR(castle::math::derivative_forward(cube_fn, 2.0, 0.5), 15.25, 1e-12);
    EXPECT_NEAR(castle::math::derivative_forward(cube_fn, 2.0, 0.1), 12.61, 1e-10);
    EXPECT_NEAR(castle::math::derivative_forward(cube_fn, 2.0, 1e-5), 12.0, 1e-3);
    // Negative step mirrors the formula.
    EXPECT_NEAR(castle::math::derivative_forward(cube_fn, 2.0, -0.5), 9.25, 1e-12);
}

TEST(MathNumericDifferentiation, BackwardDifferenceOnCubic)
{
    // 12 - 6h + h^2.
    EXPECT_NEAR(castle::math::derivative_backward(cube_fn, 2.0, 0.5), 9.25, 1e-12);
    EXPECT_NEAR(castle::math::derivative_backward(cube_fn, 2.0, 0.1), 11.41, 1e-10);
    EXPECT_NEAR(castle::math::derivative_backward(cube_fn, 2.0, 1e-5), 12.0, 1e-3);
    EXPECT_NEAR(castle::math::derivative_backward(cube_fn, 2.0, -0.5), 15.25, 1e-12);
}

TEST(MathNumericDifferentiation, OneSidedDifferencesOnExp)
{
    EXPECT_NEAR(castle::math::derivative_forward(exp_fn, 1.0, 1e-6), kE, 1e-5);
    EXPECT_NEAR(castle::math::derivative_backward(exp_fn, 1.0, 1e-6), kE, 1e-5);
    // Forward and backward bracket the true derivative of a convex function.
    EXPECT_GT(castle::math::derivative_forward(exp_fn, 1.0, 0.1), kE);
    EXPECT_LT(castle::math::derivative_backward(exp_fn, 1.0, 0.1), kE);
}

TEST(MathNumericDifferentiation, OneSidedUseTwoEvaluations)
{
    unsigned calls = 0U;
    const counting_functor functor = {&calls};
    EXPECT_NEAR(castle::math::derivative_forward(functor, 1.0, 0.5), 2.5, 1e-12);
    EXPECT_EQ(calls, 2U);
    calls = 0U;
    EXPECT_NEAR(castle::math::derivative_backward(functor, 1.0, 0.5), 1.5, 1e-12);
    EXPECT_EQ(calls, 2U);
}

TEST(MathNumericDifferentiation, OneSidedFloatAndConstexpr)
{
    EXPECT_NEAR(castle::math::derivative_forward(square_fn_f, 3.0F, 0.5F), 6.5F, 1e-5F);
    EXPECT_NEAR(castle::math::derivative_backward(square_fn_f, 3.0F, 0.5F), 5.5F, 1e-5F);
    constexpr double fwd = castle::math::derivative_forward([](double x) { return x * x; }, 3.0, 0.5);
    constexpr double bwd = castle::math::derivative_backward([](double x) { return x * x; }, 3.0, 0.5);
    static_assert(fwd == 6.5, "forward");
    static_assert(bwd == 5.5, "backward");
    EXPECT_DOUBLE_EQ(fwd, 6.5);
    EXPECT_DOUBLE_EQ(bwd, 5.5);
}

// ---------------------------------------------------------------------------
// derivative_central
// ---------------------------------------------------------------------------

TEST(MathNumericDifferentiation, CentralDifferenceExactForQuadratics)
{
    const auto quad = [](double x) { return 3.0 * x * x - 2.0 * x + 7.0; };
    EXPECT_NEAR(castle::math::derivative_central(quad, 2.0, 0.5), 10.0, 1e-12);
    EXPECT_NEAR(castle::math::derivative_central(quad, 2.0, 2.0), 10.0, 1e-12);
    EXPECT_NEAR(castle::math::derivative_central(quad, -1.0, 0.1), -8.0, 1e-12);
}

TEST(MathNumericDifferentiation, CentralDifferenceOnCubicQuarticExp)
{
    // x^3: f' + h^2.
    EXPECT_NEAR(castle::math::derivative_central(cube_fn, 2.0, 0.5), 12.25, 1e-12);
    EXPECT_NEAR(castle::math::derivative_central(cube_fn, 2.0, 1e-4), 12.0, 1e-6);
    // x^4 at 1.5: 4x^3 + 4xh^2 = 13.5 + 6h^2.
    EXPECT_NEAR(castle::math::derivative_central(quartic_functor{}, 1.5, 0.5), 13.5 + 1.5, 1e-12);
    EXPECT_NEAR(castle::math::derivative_central(exp_fn, 1.0, 1e-4), kE, 1e-6);
    // Negative step gives the same value.
    EXPECT_NEAR(castle::math::derivative_central(cube_fn, 2.0, -0.5), 12.25, 1e-12);
}

TEST(MathNumericDifferentiation, CentralDifferenceFloatAndConstexpr)
{
    EXPECT_NEAR(castle::math::derivative_central(square_fn_f, 3.0F, 0.5F), 6.0F, 1e-5F);
    constexpr double value = castle::math::derivative_central([](double x) { return x * x; }, 3.0, 0.5);
    static_assert(value == 6.0, "central");
    EXPECT_DOUBLE_EQ(value, 6.0);
}

// ---------------------------------------------------------------------------
// derivative_five_point
// ---------------------------------------------------------------------------

TEST(MathNumericDifferentiation, FivePointExactForPolynomialsUpToDegreeFour)
{
    EXPECT_NEAR(castle::math::derivative_five_point(cube_fn, 2.0, 0.5), 12.0, 1e-11);
    EXPECT_NEAR(castle::math::derivative_five_point(quartic_functor{}, 1.5, 0.25), 13.5, 1e-11);
    EXPECT_NEAR(castle::math::derivative_five_point(quartic_functor{}, 1.5, 1.0), 13.5, 1e-11);
    const auto quad = [](double x) { return x * x; };
    EXPECT_NEAR(castle::math::derivative_five_point(quad, 3.0, 2.0), 6.0, 1e-11);
}

TEST(MathNumericDifferentiation, FivePointOnExpIsFourthOrder)
{
    const double coarse = castle::math::derivative_five_point(exp_fn, 1.0, 0.1) - kE;
    const double fine = castle::math::derivative_five_point(exp_fn, 1.0, 0.05) - kE;
    EXPECT_NEAR(coarse, 0.0, 1e-5);
    EXPECT_NEAR(fine, 0.0, 1e-6);
    EXPECT_NEAR(coarse / fine, 16.0, 0.5);
    EXPECT_NEAR(castle::math::derivative_five_point(exp_fn, 1.0, 0.01), kE, 1e-8);
    EXPECT_NEAR(castle::math::derivative_five_point(exp_fn, 1.0, -0.01), kE, 1e-8);
}

TEST(MathNumericDifferentiation, FivePointEvaluationCountFloatConstexpr)
{
    unsigned calls = 0U;
    const counting_functor functor = {&calls};
    EXPECT_NEAR(castle::math::derivative_five_point(functor, 1.0, 0.25), 2.0, 1e-12);
    EXPECT_EQ(calls, 4U);

    EXPECT_NEAR(castle::math::derivative_five_point(square_fn_f, 3.0F, 0.5F), 6.0F, 1e-4F);
    constexpr double value = castle::math::derivative_five_point([](double x) { return x * x; }, 3.0, 0.5);
    static_assert(value > 5.999 && value < 6.001, "five point");
    EXPECT_NEAR(value, 6.0, 1e-12);
}

// ---------------------------------------------------------------------------
// derivative_richardson
// ---------------------------------------------------------------------------

TEST(MathNumericDifferentiation, RichardsonTwoLevelsEliminatesLeadingErrorTerm)
{
    // Central difference of x^3 / x^4 contains only an h^2 term, so one extrapolation is exact.
    EXPECT_NEAR(castle::math::derivative_richardson<2U>(cube_fn, 2.0, 0.5), 12.0, 1e-11);
    EXPECT_NEAR(castle::math::derivative_richardson<2U>(cube_fn, 2.0, 0.1), 12.0, 1e-11);
    EXPECT_NEAR(castle::math::derivative_richardson<2U>(quartic_functor{}, 1.5, 0.5), 13.5, 1e-11);
    // Smooth non-polynomial: error shrinks with each level.
    const double two = castle::math::derivative_richardson<2U>(exp_fn, 1.0, 0.2) - kE;
    const double three = castle::math::derivative_richardson<3U>(exp_fn, 1.0, 0.2) - kE;
    EXPECT_NEAR(two, 0.0, 1e-4);
    EXPECT_LT((three < 0.0 ? -three : three), (two < 0.0 ? -two : two));
}

TEST(MathNumericDifferentiation, RichardsonDefaultLevels)
{
    EXPECT_NEAR(castle::math::derivative_richardson(cube_fn, 2.0, 0.5), 12.0, 1e-9);
    EXPECT_NEAR(castle::math::derivative_richardson(quartic_functor{}, 1.5, 0.5), 13.5, 1e-9);
    EXPECT_NEAR(castle::math::derivative_richardson(exp_fn, 1.0, 0.5), kE, 1e-5);
    EXPECT_NEAR(castle::math::derivative_richardson(exp_fn, 1.0, -0.5), kE, 1e-5);
    // Default is equivalent to four explicit levels.
    EXPECT_DOUBLE_EQ(castle::math::derivative_richardson(exp_fn, 1.0, 0.5), castle::math::derivative_richardson<4U>(exp_fn, 1.0, 0.5));
}

TEST(MathNumericDifferentiation, RichardsonManyLevels)
{
    EXPECT_NEAR(castle::math::derivative_richardson<6U>(exp_fn, 1.0, 0.5), kE, 1e-9);
    EXPECT_NEAR(castle::math::derivative_richardson<8U>(exp_fn, 1.0, 1.0), kE, 1e-8);
    EXPECT_NEAR(castle::math::derivative_richardson<16U>(exp_fn, 1.0, 1.0), kE, 1e-6);
}

TEST(MathNumericDifferentiation, RichardsonEvaluationCountFloatConstexpr)
{
    unsigned calls = 0U;
    const counting_functor functor = {&calls};
    EXPECT_NEAR(castle::math::derivative_richardson<5U>(functor, 1.0, 0.5), 2.0, 1e-9);
    EXPECT_EQ(calls, 10U); // 2 * Levels

    EXPECT_NEAR(castle::math::derivative_richardson(square_fn_f, 3.0F, 0.5F), 6.0F, 1e-3F);
    EXPECT_NEAR(castle::math::derivative_richardson<2U>(square_fn_f, 3.0F, 0.5F), 6.0F, 1e-3F);
    constexpr double value = castle::math::derivative_richardson<3U>([](double x) { return x * x * x; }, 2.0, 0.5);
    static_assert(value > 11.999 && value < 12.001, "richardson");
    EXPECT_NEAR(value, 12.0, 1e-9);
}

// ---------------------------------------------------------------------------
// second_derivative
// ---------------------------------------------------------------------------

TEST(MathNumericDifferentiation, SecondDerivativeExactForCubics)
{
    EXPECT_NEAR(castle::math::second_derivative(cube_fn, 2.0, 0.5), 12.0, 1e-11);
    EXPECT_NEAR(castle::math::second_derivative(cube_fn, 2.0, 1e-2), 12.0, 1e-6);
    EXPECT_NEAR(castle::math::second_derivative(cube_fn, 2.0, -0.5), 12.0, 1e-11);
    const auto quad = [](double x) { return 4.0 * x * x - x; };
    EXPECT_NEAR(castle::math::second_derivative(quad, 1.0, 0.25), 8.0, 1e-11);
}

TEST(MathNumericDifferentiation, SecondDerivativeOnQuarticAndExp)
{
    // x^4 at 1.5: 12x^2 + 2h^2 = 27 + 2h^2.
    EXPECT_NEAR(castle::math::second_derivative(quartic_functor{}, 1.5, 0.5), 27.5, 1e-11);
    EXPECT_NEAR(castle::math::second_derivative(exp_fn, 1.0, 1e-3), kE, 1e-5);
}

TEST(MathNumericDifferentiation, SecondDerivativeFloatAndConstexpr)
{
    EXPECT_NEAR(castle::math::second_derivative(square_fn_f, 3.0F, 0.5F), 2.0F, 1e-4F);
    constexpr double value = castle::math::second_derivative([](double x) { return x * x; }, 3.0, 0.5);
    static_assert(value == 2.0, "second derivative");
    EXPECT_DOUBLE_EQ(value, 2.0);
}

// ---------------------------------------------------------------------------
// derivative_samples
// ---------------------------------------------------------------------------

TEST(MathNumericDifferentiation, SamplesStatusPaths)
{
    const double y[] = {0.0, 1.0, 4.0};
    double out[3] = {-1.0, -1.0, -1.0};
    EXPECT_EQ(castle::math::derivative_samples<double>(nullptr, 3U, 1.0, out), castle::status::invalid_argument);
    EXPECT_EQ(castle::math::derivative_samples<double>(y, 3U, 1.0, nullptr), castle::status::invalid_argument);
    EXPECT_EQ(castle::math::derivative_samples(y, 0U, 1.0, out), castle::status::invalid_argument);
    EXPECT_EQ(castle::math::derivative_samples(y, 1U, 1.0, out), castle::status::invalid_argument);
    EXPECT_EQ(castle::math::derivative_samples(y, 3U, 0.0, out), castle::status::invalid_argument);
    EXPECT_EQ(castle::math::derivative_samples(y, 3U, 1.0, out), castle::status::ok);
    EXPECT_NEAR(out[1], 2.0, 1e-12);
}

TEST(MathNumericDifferentiation, SamplesFailureLeavesOutputUntouched)
{
    const double y[] = {0.0, 1.0, 4.0};
    double out[3] = {-1.0, -1.0, -1.0};
    EXPECT_EQ(castle::math::derivative_samples(y, 3U, 0.0, out), castle::status::invalid_argument);
    EXPECT_EQ(castle::math::derivative_samples(y, 1U, 1.0, out), castle::status::invalid_argument);
    EXPECT_DOUBLE_EQ(out[0], -1.0);
    EXPECT_DOUBLE_EQ(out[1], -1.0);
    EXPECT_DOUBLE_EQ(out[2], -1.0);
}

TEST(MathNumericDifferentiation, SamplesTwoPointsUseForwardDifference)
{
    const double y[] = {1.0, 3.0};
    double out[2] = {};
    EXPECT_EQ(castle::math::derivative_samples(y, 2U, 0.5, out), castle::status::ok);
    EXPECT_NEAR(out[0], 4.0, 1e-12);
    EXPECT_NEAR(out[1], 4.0, 1e-12);

    EXPECT_EQ(castle::math::derivative_samples(y, 2U, -0.5, out), castle::status::ok);
    EXPECT_NEAR(out[0], -4.0, 1e-12);
    EXPECT_NEAR(out[1], -4.0, 1e-12);
}

TEST(MathNumericDifferentiation, SamplesThreePoints)
{
    // y = x^2 at x = 0, 0.5, 1 -> y' = 2x = 0, 1, 2 (end formulas are exact for quadratics).
    const double y[] = {0.0, 0.25, 1.0};
    double out[3] = {};
    EXPECT_EQ(castle::math::derivative_samples(y, 3U, 0.5, out), castle::status::ok);
    EXPECT_NEAR(out[0], 0.0, 1e-12);
    EXPECT_NEAR(out[1], 1.0, 1e-12);
    EXPECT_NEAR(out[2], 2.0, 1e-12);
}

TEST(MathNumericDifferentiation, SamplesLargerCountOnQuadratic)
{
    // y = x^2 + 3x + 1 sampled at x = 0, 0.25, ... 1.5 -> y' = 2x + 3.
    double y[7] = {};
    for (unsigned i = 0U; i < 7U; ++i)
    {
        const double x = 0.25 * static_cast<double>(i);
        y[i] = x * x + 3.0 * x + 1.0;
    }
    double out[7] = {};
    EXPECT_EQ(castle::math::derivative_samples(y, 7U, 0.25, out), castle::status::ok);
    for (unsigned i = 0U; i < 7U; ++i)
    {
        EXPECT_NEAR(out[i], 2.0 * 0.25 * static_cast<double>(i) + 3.0, 1e-12);
    }
}

TEST(MathNumericDifferentiation, SamplesOnExpIsSecondOrderAccurate)
{
    double y[11] = {};
    for (unsigned i = 0U; i < 11U; ++i)
    {
        y[i] = castle::math::exp(0.1 * static_cast<double>(i));
    }
    double out[11] = {};
    EXPECT_EQ(castle::math::derivative_samples(y, 11U, 0.1, out), castle::status::ok);
    for (unsigned i = 0U; i < 11U; ++i)
    {
        EXPECT_NEAR(out[i], castle::math::exp(0.1 * static_cast<double>(i)), 1.5e-2);
    }
}

TEST(MathNumericDifferentiation, SamplesNegativeSpacing)
{
    // Samples of x^2 listed from x = 2 down to x = 0 (dx = -0.5).
    const double y[] = {4.0, 2.25, 1.0, 0.25, 0.0};
    double out[5] = {};
    EXPECT_EQ(castle::math::derivative_samples(y, 5U, -0.5, out), castle::status::ok);
    EXPECT_NEAR(out[0], 4.0, 1e-12);
    EXPECT_NEAR(out[1], 3.0, 1e-12);
    EXPECT_NEAR(out[2], 2.0, 1e-12);
    EXPECT_NEAR(out[3], 1.0, 1e-12);
    EXPECT_NEAR(out[4], 0.0, 1e-12);
}

TEST(MathNumericDifferentiation, SamplesAliasingOutEqualsY)
{
    double buffer[5] = {0.0, 1.0, 4.0, 9.0, 16.0};
    EXPECT_EQ(castle::math::derivative_samples(buffer, 5U, 1.0, buffer), castle::status::ok);
    for (unsigned i = 0U; i < 5U; ++i)
    {
        EXPECT_NEAR(buffer[i], 2.0 * static_cast<double>(i), 1e-12);
    }

    double pair[2] = {1.0, 3.0};
    EXPECT_EQ(castle::math::derivative_samples(pair, 2U, 1.0, pair), castle::status::ok);
    EXPECT_NEAR(pair[0], 2.0, 1e-12);
    EXPECT_NEAR(pair[1], 2.0, 1e-12);
}

TEST(MathNumericDifferentiation, SamplesFloat)
{
    const float y[] = {0.0F, 1.0F, 4.0F, 9.0F, 16.0F};
    float out[5] = {};
    EXPECT_EQ(castle::math::derivative_samples(y, 5U, 1.0F, out), castle::status::ok);
    EXPECT_NEAR(out[0], 0.0F, 1e-5F);
    EXPECT_NEAR(out[2], 4.0F, 1e-5F);
    EXPECT_NEAR(out[4], 8.0F, 1e-5F);
    EXPECT_EQ(castle::math::derivative_samples<float>(nullptr, 5U, 1.0F, out), castle::status::invalid_argument);
}

} // namespace

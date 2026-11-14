#include <gtest/gtest.h>

#include "castle/core/status.hpp"
#include "castle/math/numeric/root_finding.hpp"

namespace
{

const double kRoot2 = 1.4142135623730951;
const float kRoot2f = 1.41421356F;

double square_minus_two(double x)
{
    return (x * x) - 2.0;
}

double square_minus_two_derivative(double x)
{
    return 2.0 * x;
}

struct CubicMinusEight
{
    double operator()(double x) const
    {
        return (x * x * x) - 8.0;
    }
};

struct CubicDerivative
{
    double operator()(double x) const
    {
        return 3.0 * x * x;
    }
};

// ---------------------------------------------------------------------------
// find_root_bisection
// ---------------------------------------------------------------------------
TEST(MathRootFinding, BisectionConvergesDouble)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto r = castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-12);

    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.code, castle::status::ok);
    EXPECT_NEAR(r.value, kRoot2, 1.0e-11);
    EXPECT_LE(r.error, 1.0e-12 * 2.0);
    EXPECT_GE(r.error, 0.0);
    EXPECT_GT(r.iterations, 10U);
    EXPECT_LT(r.iterations, 100U);
}

TEST(MathRootFinding, BisectionConvergesFloat)
{
    const auto f = [](float x) { return (x * x) - 2.0F; };
    const castle::math::numeric_result<float> r = castle::math::find_root_bisection(f, 0.0F, 2.0F, 1.0e-6F);

    EXPECT_TRUE(r.ok());
    EXPECT_NEAR(r.value, kRoot2f, 1.0e-5F);
    EXPECT_LE(r.error, 1.0e-6F * 2.0F);
}

TEST(MathRootFinding, BisectionBothBranchDirections)
{
    // Increasing function: the midpoint sign matches f(lo) and the lower end moves, then the upper end.
    const auto increasing = [](double x) { return (x * x * x) - 3.0; };
    const auto up = castle::math::find_root_bisection(increasing, 0.0, 3.0, 1.0e-12);
    EXPECT_TRUE(up.ok());
    EXPECT_NEAR(up.value, 1.4422495703074083, 1.0e-11);

    // Decreasing function: the opposite sign pattern is exercised.
    const auto decreasing = [](double x) { return 3.0 - (x * x * x); };
    const auto down = castle::math::find_root_bisection(decreasing, 0.0, 3.0, 1.0e-12);
    EXPECT_TRUE(down.ok());
    EXPECT_NEAR(down.value, 1.4422495703074083, 1.0e-11);
}

TEST(MathRootFinding, BisectionReversedBracket)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto forward = castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-12);
    const auto reversed = castle::math::find_root_bisection(f, 2.0, 0.0, 1.0e-12);

    EXPECT_TRUE(reversed.ok());
    EXPECT_NEAR(reversed.value, kRoot2, 1.0e-11);
    EXPECT_NEAR(reversed.value, forward.value, 1.0e-11);
    EXPECT_LE(reversed.error, 1.0e-12 * 2.0);
}

TEST(MathRootFinding, BisectionNegativeBracket)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto r = castle::math::find_root_bisection(f, -2.0, 0.0, 1.0e-12);

    EXPECT_TRUE(r.ok());
    EXPECT_NEAR(r.value, -kRoot2, 1.0e-11);
}

TEST(MathRootFinding, BisectionExactZeroAtLo)
{
    const auto f = [](double x) { return x - 1.0; };
    const auto r = castle::math::find_root_bisection(f, 1.0, 3.0, 1.0e-12);

    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.value, 1.0);
    EXPECT_EQ(r.error, 0.0);
    EXPECT_EQ(r.iterations, 0U);

    // Reversed bracket: the zero sits at the first argument.
    const auto reversed = castle::math::find_root_bisection(f, 1.0, -3.0, 1.0e-12);
    EXPECT_TRUE(reversed.ok());
    EXPECT_EQ(reversed.value, 1.0);
    EXPECT_EQ(reversed.iterations, 0U);
}

TEST(MathRootFinding, BisectionExactZeroAtHi)
{
    const auto f = [](double x) { return x - 2.0; };
    const auto r = castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-12);

    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.value, 2.0);
    EXPECT_EQ(r.error, 0.0);
    EXPECT_EQ(r.iterations, 0U);
}

TEST(MathRootFinding, BisectionExactZeroAtMidpoint)
{
    const auto f = [](double x) { return x - 1.0; };
    const auto r = castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-12);

    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.value, 1.0);
    EXPECT_EQ(r.error, 0.0);
    EXPECT_EQ(r.iterations, 1U);
}

TEST(MathRootFinding, BisectionSameSignBracketIsInvalid)
{
    const auto f = [](double x) { return (x * x) - 2.0; };

    // Both positive.
    const auto positive = castle::math::find_root_bisection(f, 2.0, 3.0, 1.0e-12);
    EXPECT_FALSE(positive.ok());
    EXPECT_EQ(positive.code, castle::status::invalid_argument);
    EXPECT_EQ(positive.value, 2.0);
    EXPECT_EQ(positive.error, 1.0);
    EXPECT_EQ(positive.iterations, 0U);

    // Both negative.
    const auto negative = castle::math::find_root_bisection(f, -1.0, 1.0, 1.0e-12);
    EXPECT_EQ(negative.code, castle::status::invalid_argument);
    EXPECT_EQ(negative.value, -1.0);
    EXPECT_EQ(negative.error, 2.0);
    EXPECT_EQ(negative.iterations, 0U);

    // Reversed order reports the distance as a magnitude.
    const auto reversed = castle::math::find_root_bisection(f, 3.0, 2.0, 1.0e-12);
    EXPECT_EQ(reversed.code, castle::status::invalid_argument);
    EXPECT_EQ(reversed.value, 3.0);
    EXPECT_EQ(reversed.error, 1.0);
}

TEST(MathRootFinding, BisectionIterationBudgetExhausted)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto r = castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-12, 3U);

    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.code, castle::status::out_of_range);
    EXPECT_EQ(r.iterations, 3U);
    // After three halvings the bracket is [1.25, 1.5] with half width 0.125.
    EXPECT_NEAR(r.value, 1.375, 1.0e-12);
    EXPECT_NEAR(r.error, 0.125, 1.0e-12);
}

TEST(MathRootFinding, BisectionZeroIterationBudget)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto r = castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-12, 0U);

    EXPECT_EQ(r.code, castle::status::out_of_range);
    EXPECT_EQ(r.iterations, 0U);
    EXPECT_EQ(r.value, 0.0);
    EXPECT_EQ(r.error, 2.0);
}

TEST(MathRootFinding, BisectionCallableForms)
{
    // Function pointer.
    const auto from_pointer = castle::math::find_root_bisection(&square_minus_two, 0.0, 2.0, 1.0e-12);
    EXPECT_TRUE(from_pointer.ok());
    EXPECT_NEAR(from_pointer.value, kRoot2, 1.0e-11);

    // Function reference.
    const auto from_function = castle::math::find_root_bisection(square_minus_two, 0.0, 2.0, 1.0e-12);
    EXPECT_TRUE(from_function.ok());
    EXPECT_NEAR(from_function.value, kRoot2, 1.0e-11);

    // Functor object (rvalue and lvalue).
    const auto from_rvalue = castle::math::find_root_bisection(CubicMinusEight(), 0.0, 4.0, 1.0e-12);
    EXPECT_TRUE(from_rvalue.ok());
    EXPECT_NEAR(from_rvalue.value, 2.0, 1.0e-11);

    CubicMinusEight functor;
    const auto from_lvalue = castle::math::find_root_bisection(functor, 0.0, 4.0, 1.0e-12);
    EXPECT_TRUE(from_lvalue.ok());
    EXPECT_NEAR(from_lvalue.value, 2.0, 1.0e-11);

    // Capturing lambda.
    const double target = 9.0;
    const auto capture = [target](double x) { return (x * x) - target; };
    const auto from_capture = castle::math::find_root_bisection(capture, 0.0, 10.0, 1.0e-12);
    EXPECT_TRUE(from_capture.ok());
    EXPECT_NEAR(from_capture.value, 3.0, 1.0e-11);
}

TEST(MathRootFinding, BisectionEvaluationCount)
{
    unsigned calls = 0U;
    const auto f = [&calls](double x)
    {
        ++calls;
        return (x * x) - 2.0;
    };
    const auto r = castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-6);

    EXPECT_TRUE(r.ok());
    EXPECT_EQ(calls, 2U + r.iterations);
}

// ---------------------------------------------------------------------------
// find_root_newton
// ---------------------------------------------------------------------------
TEST(MathRootFinding, NewtonConvergesDouble)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto df = [](double x) { return 2.0 * x; };
    const auto r = castle::math::find_root_newton(f, df, 1.0, 1.0e-12);

    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.code, castle::status::ok);
    EXPECT_NEAR(r.value, kRoot2, 1.0e-12);
    EXPECT_LE(r.error, 1.0e-12 * 2.0);
    EXPECT_GT(r.iterations, 0U);
    EXPECT_LT(r.iterations, 10U);
}

TEST(MathRootFinding, NewtonConvergesFloat)
{
    const auto f = [](float x) { return (x * x) - 2.0F; };
    const auto df = [](float x) { return 2.0F * x; };
    const castle::math::numeric_result<float> r = castle::math::find_root_newton(f, df, 1.0F, 1.0e-6F);

    EXPECT_TRUE(r.ok());
    EXPECT_NEAR(r.value, kRoot2f, 1.0e-6F);
    EXPECT_LT(r.iterations, 10U);
}

TEST(MathRootFinding, NewtonNegativeRootAndCallableForms)
{
    const auto from_pointers = castle::math::find_root_newton(&square_minus_two, &square_minus_two_derivative, -1.0, 1.0e-12);
    EXPECT_TRUE(from_pointers.ok());
    EXPECT_NEAR(from_pointers.value, -kRoot2, 1.0e-12);

    const auto from_functors = castle::math::find_root_newton(CubicMinusEight(), CubicDerivative(), 5.0, 1.0e-12);
    EXPECT_TRUE(from_functors.ok());
    EXPECT_NEAR(from_functors.value, 2.0, 1.0e-12);

    CubicMinusEight f;
    CubicDerivative df;
    const auto from_lvalues = castle::math::find_root_newton(f, df, 5.0, 1.0e-12);
    EXPECT_TRUE(from_lvalues.ok());
    EXPECT_NEAR(from_lvalues.value, 2.0, 1.0e-12);
}

TEST(MathRootFinding, NewtonIsFasterThanBisection)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto df = [](double x) { return 2.0 * x; };
    const auto newton = castle::math::find_root_newton(f, df, 1.0, 1.0e-12);
    const auto bisection = castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-12);

    EXPECT_TRUE(newton.ok());
    EXPECT_TRUE(bisection.ok());
    EXPECT_LT(newton.iterations, bisection.iterations);
}

TEST(MathRootFinding, NewtonZeroDerivativeAtStart)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto df = [](double x) { return 2.0 * x; };
    const auto r = castle::math::find_root_newton(f, df, 0.0, 1.0e-12);

    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.code, castle::status::invalid_argument);
    EXPECT_EQ(r.value, 0.0);
    EXPECT_EQ(r.iterations, 0U);
}

TEST(MathRootFinding, NewtonZeroDerivativeAtLaterIterate)
{
    // f(x) = x^2 + 1 has no real root; from x0 = 1 the first step lands on x = 0 where f' vanishes.
    const auto f = [](double x) { return (x * x) + 1.0; };
    const auto df = [](double x) { return 2.0 * x; };
    const auto r = castle::math::find_root_newton(f, df, 1.0, 1.0e-12);

    EXPECT_EQ(r.code, castle::status::invalid_argument);
    EXPECT_EQ(r.value, 0.0);
    EXPECT_EQ(r.iterations, 1U);
}

TEST(MathRootFinding, NewtonIterationBudgetExhausted)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto df = [](double x) { return 2.0 * x; };
    const auto r = castle::math::find_root_newton(f, df, 100.0, 1.0e-12, 2U);

    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.code, castle::status::out_of_range);
    EXPECT_EQ(r.iterations, 2U);
    EXPECT_GT(r.error, 1.0e-12);

    // Zero budget returns the initial guess untouched.
    const auto none = castle::math::find_root_newton(f, df, 100.0, 1.0e-12, 0U);
    EXPECT_EQ(none.code, castle::status::out_of_range);
    EXPECT_EQ(none.iterations, 0U);
    EXPECT_EQ(none.value, 100.0);
    EXPECT_EQ(none.error, 0.0);
}

TEST(MathRootFinding, NewtonEvaluationCount)
{
    unsigned f_calls = 0U;
    unsigned df_calls = 0U;
    const auto f = [&f_calls](double x)
    {
        ++f_calls;
        return (x * x) - 2.0;
    };
    const auto df = [&df_calls](double x)
    {
        ++df_calls;
        return 2.0 * x;
    };
    const auto r = castle::math::find_root_newton(f, df, 1.0, 1.0e-12);

    EXPECT_TRUE(r.ok());
    EXPECT_EQ(f_calls, r.iterations);
    EXPECT_EQ(df_calls, r.iterations);
}

// ---------------------------------------------------------------------------
// find_root_secant
// ---------------------------------------------------------------------------
TEST(MathRootFinding, SecantConvergesDouble)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto r = castle::math::find_root_secant(f, 1.0, 2.0, 1.0e-12);

    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.code, castle::status::ok);
    EXPECT_NEAR(r.value, kRoot2, 1.0e-11);
    EXPECT_LE(r.error, 1.0e-12 * 2.0);
    EXPECT_GT(r.iterations, 0U);
    EXPECT_LT(r.iterations, 15U);
}

TEST(MathRootFinding, SecantConvergesFloat)
{
    const auto f = [](float x) { return (x * x) - 2.0F; };
    const castle::math::numeric_result<float> r = castle::math::find_root_secant(f, 1.0F, 2.0F, 1.0e-6F);

    EXPECT_TRUE(r.ok());
    EXPECT_NEAR(r.value, kRoot2f, 1.0e-5F);
}

TEST(MathRootFinding, SecantCallableForms)
{
    const auto from_pointer = castle::math::find_root_secant(&square_minus_two, 2.0, 1.0, 1.0e-12);
    EXPECT_TRUE(from_pointer.ok());
    EXPECT_NEAR(from_pointer.value, kRoot2, 1.0e-11);

    const auto from_functor = castle::math::find_root_secant(CubicMinusEight(), 1.0, 3.0, 1.0e-12);
    EXPECT_TRUE(from_functor.ok());
    EXPECT_NEAR(from_functor.value, 2.0, 1.0e-11);

    CubicMinusEight functor;
    const auto from_lvalue = castle::math::find_root_secant(functor, 1.0, 3.0, 1.0e-12);
    EXPECT_TRUE(from_lvalue.ok());
    EXPECT_NEAR(from_lvalue.value, 2.0, 1.0e-11);
}

TEST(MathRootFinding, SecantExactZeroAtSecondStartingPoint)
{
    const auto f = [](double x) { return (x * x) - 4.0; };
    const auto r = castle::math::find_root_secant(f, 1.0, 2.0, 1.0e-12);

    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.value, 2.0);
    EXPECT_EQ(r.error, 0.0);
    EXPECT_EQ(r.iterations, 0U);
}

TEST(MathRootFinding, SecantExactZeroDetectedAfterStep)
{
    // A linear function is solved exactly in one secant step; with a zero tolerance the
    // convergence is reported by the exact-zero check on the next pass.
    const auto f = [](double x) { return (2.0 * x) - 1.0; };
    const auto r = castle::math::find_root_secant(f, 0.0, 1.0, 0.0);

    EXPECT_TRUE(r.ok());
    EXPECT_EQ(r.value, 0.5);
    EXPECT_EQ(r.error, 0.0);
    EXPECT_EQ(r.iterations, 1U);
}

TEST(MathRootFinding, SecantFlatAtStart)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto r = castle::math::find_root_secant(f, -1.0, 1.0, 1.0e-12);

    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.code, castle::status::invalid_argument);
    EXPECT_EQ(r.value, 1.0);
    EXPECT_EQ(r.error, 2.0);
    EXPECT_EQ(r.iterations, 0U);
}

TEST(MathRootFinding, SecantFlatAtLaterIterate)
{
    // f(x) = x^2 + 1 has no real root: 0 -> 1 -> -1 gives f(-1) == f(1) on the second pass.
    const auto f = [](double x) { return (x * x) + 1.0; };
    const auto r = castle::math::find_root_secant(f, 0.0, 1.0, 1.0e-12);

    EXPECT_EQ(r.code, castle::status::invalid_argument);
    EXPECT_EQ(r.value, -1.0);
    EXPECT_EQ(r.iterations, 1U);
}

TEST(MathRootFinding, SecantIterationBudgetExhausted)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto r = castle::math::find_root_secant(f, 100.0, 200.0, 1.0e-12, 2U);

    EXPECT_FALSE(r.ok());
    EXPECT_EQ(r.code, castle::status::out_of_range);
    EXPECT_EQ(r.iterations, 2U);
    EXPECT_GT(r.error, 1.0e-12);

    // Zero budget returns the second starting point.
    const auto none = castle::math::find_root_secant(f, 100.0, 200.0, 1.0e-12, 0U);
    EXPECT_EQ(none.code, castle::status::out_of_range);
    EXPECT_EQ(none.iterations, 0U);
    EXPECT_EQ(none.value, 200.0);
    EXPECT_EQ(none.error, 100.0);
}

TEST(MathRootFinding, SecantEvaluationCount)
{
    unsigned calls = 0U;
    const auto f = [&calls](double x)
    {
        ++calls;
        return (x * x) - 2.0;
    };
    const auto r = castle::math::find_root_secant(f, 1.0, 2.0, 1.0e-12);

    EXPECT_TRUE(r.ok());
    EXPECT_EQ(calls, 2U + r.iterations);
}

TEST(MathRootFinding, SecantIsFasterThanBisection)
{
    const auto f = [](double x) { return (x * x) - 2.0; };
    const auto secant = castle::math::find_root_secant(f, 1.0, 2.0, 1.0e-12);
    const auto bisection = castle::math::find_root_bisection(f, 1.0, 2.0, 1.0e-12);

    EXPECT_TRUE(secant.ok());
    EXPECT_TRUE(bisection.ok());
    EXPECT_LT(secant.iterations, bisection.iterations);
}

// ---------------------------------------------------------------------------
// Tolerance scaling: error <= tolerance * max(1, |x|)
// ---------------------------------------------------------------------------
TEST(MathRootFinding, RelativeToleranceForLargeRoots)
{
    const auto f = [](double x) { return x - 1.0e6; };
    const auto df = [](double) { return 1.0; };

    const auto newton = castle::math::find_root_newton(f, df, 0.0, 1.0e-9);
    EXPECT_TRUE(newton.ok());
    EXPECT_NEAR(newton.value, 1.0e6, 1.0e-3);

    const auto secant = castle::math::find_root_secant(f, 0.0, 10.0, 1.0e-9);
    EXPECT_TRUE(secant.ok());
    EXPECT_NEAR(secant.value, 1.0e6, 1.0e-3);

    const auto bisection = castle::math::find_root_bisection(f, 0.0, 2.0e6, 1.0e-9);
    EXPECT_TRUE(bisection.ok());
    EXPECT_NEAR(bisection.value, 1.0e6, 1.0e-3);
}

} // namespace

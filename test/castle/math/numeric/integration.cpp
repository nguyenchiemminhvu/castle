
#include <gtest/gtest.h>

#include "castle/math/exponential.hpp"
#include "castle/math/numeric/integration.hpp"

namespace
{

constexpr double kAtanOne = 0.7853981633974483;
constexpr double kEMinusOne = 1.718281828459045;

double square_fn(double x)
{
    return x * x;
}

float square_fn_f(float x)
{
    return x * x;
}

struct cubic_functor
{
    double operator()(double x) const
    {
        return x * x * x;
    }
};

struct counting_functor
{
    unsigned* calls;

    double operator()(double x) const
    {
        ++(*calls);
        return x;
    }
};

double lorentz(double x)
{
    return 1.0 / (1.0 + x * x);
}

double exp_fn(double x)
{
    return castle::math::exp(x);
}

// ---------------------------------------------------------------------------
// integrate_trapezoid
// ---------------------------------------------------------------------------

TEST(MathNumericIntegration, TrapezoidExactForLinear)
{
    const auto linear = [](double x) { return 2.0 * x + 1.0; };
    EXPECT_NEAR(castle::math::integrate_trapezoid(linear, 0.0, 3.0, 1U), 12.0, 1e-12);
    EXPECT_NEAR(castle::math::integrate_trapezoid(linear, 0.0, 3.0, 2U), 12.0, 1e-12);
    EXPECT_NEAR(castle::math::integrate_trapezoid(linear, 0.0, 3.0, 17U), 12.0, 1e-11);
    EXPECT_NEAR(castle::math::integrate_trapezoid(linear, -1.0, 1.0, 5U), 2.0, 1e-12);
}

TEST(MathNumericIntegration, TrapezoidConvergesQuadratically)
{
    const auto cube = [](double x) { return x * x * x; };
    const double coarse = castle::math::integrate_trapezoid(cube, 0.0, 2.0, 10U);
    const double fine = castle::math::integrate_trapezoid(cube, 0.0, 2.0, 20U);
    // Trapezoid on x^3 overestimates with error ~ h^2 * (b-a) * f'' / 12 -> quarter every doubling.
    EXPECT_GT(coarse, 4.0);
    EXPECT_GT(fine, 4.0);
    EXPECT_NEAR((coarse - 4.0) / (fine - 4.0), 4.0, 0.05);
    EXPECT_NEAR(castle::math::integrate_trapezoid(cube, 0.0, 2.0, 4000U), 4.0, 1e-5);
}

TEST(MathNumericIntegration, TrapezoidCallableKinds)
{
    EXPECT_NEAR(castle::math::integrate_trapezoid(square_fn, 0.0, 3.0, 3000U), 9.0, 1e-5);
    double (*pointer)(double) = &square_fn;
    EXPECT_NEAR(castle::math::integrate_trapezoid(pointer, 0.0, 3.0, 3000U), 9.0, 1e-5);
    EXPECT_NEAR(castle::math::integrate_trapezoid(cubic_functor{}, 0.0, 2.0, 4000U), 4.0, 1e-5);
    const cubic_functor named;
    EXPECT_NEAR(castle::math::integrate_trapezoid(named, 0.0, 2.0, 4000U), 4.0, 1e-5);
    EXPECT_NEAR(castle::math::integrate_trapezoid(lorentz, 0.0, 1.0, 2000U), kAtanOne, 1e-6);
    EXPECT_NEAR(castle::math::integrate_trapezoid(exp_fn, 0.0, 1.0, 2000U), kEMinusOne, 1e-6);
}

TEST(MathNumericIntegration, TrapezoidEvaluationCount)
{
    unsigned calls = 0U;
    const counting_functor functor = {&calls};
    EXPECT_NEAR(castle::math::integrate_trapezoid(functor, 0.0, 4.0, 8U), 8.0, 1e-12);
    EXPECT_EQ(calls, 9U); // intervals + 1
    calls = 0U;
    EXPECT_NEAR(castle::math::integrate_trapezoid(functor, 0.0, 4.0, 1U), 8.0, 1e-12);
    EXPECT_EQ(calls, 2U);
}

TEST(MathNumericIntegration, TrapezoidReversedAndEmptyLimits)
{
    EXPECT_NEAR(castle::math::integrate_trapezoid(square_fn, 3.0, 0.0, 3000U), -9.0, 1e-5);
    EXPECT_NEAR(castle::math::integrate_trapezoid(square_fn, 2.0, 2.0, 10U), 0.0, 1e-15);
}

TEST(MathNumericIntegration, TrapezoidFloat)
{
    const auto linear = [](float x) { return 2.0F * x + 1.0F; };
    EXPECT_NEAR(castle::math::integrate_trapezoid(linear, 0.0F, 3.0F, 4U), 12.0F, 1e-5F);
    EXPECT_NEAR(castle::math::integrate_trapezoid(square_fn_f, 0.0F, 3.0F, 300U), 9.0F, 1e-3F);
}

TEST(MathNumericIntegration, TrapezoidConstexpr)
{
    constexpr double value = castle::math::integrate_trapezoid([](double x) { return 2.0 * x; }, 0.0, 2.0, 4U);
    static_assert(value > 3.99 && value < 4.01, "trapezoid");
    EXPECT_NEAR(value, 4.0, 1e-12);
}

// ---------------------------------------------------------------------------
// integrate_simpson
// ---------------------------------------------------------------------------

TEST(MathNumericIntegration, SimpsonExactForCubics)
{
    const auto cubic = [](double x) { return x * x * x - 2.0 * x * x + x - 5.0; };
    // Antiderivative x^4/4 - 2x^3/3 + x^2/2 - 5x on [0, 3] = 81/4 - 18 + 4.5 - 15 = -8.25.
    EXPECT_NEAR(castle::math::integrate_simpson(cubic, 0.0, 3.0, 2U), -8.25, 1e-12);
    EXPECT_NEAR(castle::math::integrate_simpson(cubic, 0.0, 3.0, 4U), -8.25, 1e-12);
    EXPECT_NEAR(castle::math::integrate_simpson(cubic, 0.0, 3.0, 10U), -8.25, 1e-11);
    EXPECT_NEAR(castle::math::integrate_simpson(cubic_functor{}, 0.0, 2.0, 2U), 4.0, 1e-12);
    EXPECT_NEAR(castle::math::integrate_simpson(square_fn, 0.0, 3.0, 2U), 9.0, 1e-12);
}

TEST(MathNumericIntegration, SimpsonOddIntervalsRoundUp)
{
    unsigned calls = 0U;
    const counting_functor functor = {&calls};
    EXPECT_NEAR(castle::math::integrate_simpson(functor, 0.0, 2.0, 1U), 2.0, 1e-12);
    EXPECT_EQ(calls, 3U); // rounded up to 2 intervals -> 3 samples
    calls = 0U;
    EXPECT_NEAR(castle::math::integrate_simpson(functor, 0.0, 2.0, 3U), 2.0, 1e-12);
    EXPECT_EQ(calls, 5U); // rounded up to 4 intervals
    calls = 0U;
    EXPECT_NEAR(castle::math::integrate_simpson(functor, 0.0, 2.0, 4U), 2.0, 1e-12);
    EXPECT_EQ(calls, 5U);

    const auto cube = [](double x) { return x * x * x; };
    EXPECT_NEAR(castle::math::integrate_simpson(cube, 0.0, 2.0, 3U), 4.0, 1e-12);
    EXPECT_NEAR(castle::math::integrate_simpson(cube, 0.0, 2.0, 7U), 4.0, 1e-12);
}

TEST(MathNumericIntegration, SimpsonSmoothFunctions)
{
    EXPECT_NEAR(castle::math::integrate_simpson(lorentz, 0.0, 1.0, 100U), kAtanOne, 1e-9);
    EXPECT_NEAR(castle::math::integrate_simpson(exp_fn, 0.0, 1.0, 64U), kEMinusOne, 1e-8);
    double (*pointer)(double) = &exp_fn;
    EXPECT_NEAR(castle::math::integrate_simpson(pointer, 0.0, 1.0, 64U), kEMinusOne, 1e-8);
}

TEST(MathNumericIntegration, SimpsonReversedAndEmptyLimits)
{
    EXPECT_NEAR(castle::math::integrate_simpson(cubic_functor{}, 2.0, 0.0, 4U), -4.0, 1e-12);
    EXPECT_NEAR(castle::math::integrate_simpson(cubic_functor{}, 1.0, 1.0, 4U), 0.0, 1e-15);
}

TEST(MathNumericIntegration, SimpsonFloat)
{
    EXPECT_NEAR(castle::math::integrate_simpson(square_fn_f, 0.0F, 3.0F, 10U), 9.0F, 1e-5F);
    EXPECT_NEAR(castle::math::integrate_simpson([](float x) { return x * x * x; }, 0.0F, 2.0F, 3U), 4.0F, 1e-5F);
}

TEST(MathNumericIntegration, SimpsonConstexpr)
{
    constexpr double value = castle::math::integrate_simpson([](double x) { return x * x; }, 0.0, 3.0, 2U);
    static_assert(value > 8.99 && value < 9.01, "simpson");
    EXPECT_NEAR(value, 9.0, 1e-12);
}

// ---------------------------------------------------------------------------
// integrate_gauss_legendre
// ---------------------------------------------------------------------------

TEST(MathNumericIntegration, GaussLegendreExactUpToDegreeNine)
{
    const auto degree9 = [](double x) { return x * x * x * x * x * x * x * x * x; };
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(degree9, 0.0, 1.0), 0.1, 1e-14);
    const auto degree4 = [](double x) { return x * x * x * x; };
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(degree4, 0.0, 2.0), 32.0 / 5.0, 1e-13);
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(cubic_functor{}, 0.0, 2.0), 4.0, 1e-13);
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(square_fn, -1.0, 2.0), 3.0, 1e-13);
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(square_fn, -1.0, 2.0, 1U), 3.0, 1e-13);
}

TEST(MathNumericIntegration, GaussLegendreCompositePanels)
{
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(lorentz, 0.0, 1.0), kAtanOne, 1e-6);
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(lorentz, 0.0, 1.0, 4U), kAtanOne, 1e-12);
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(exp_fn, 0.0, 1.0, 2U), kEMinusOne, 1e-12);
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(exp_fn, 1.0, 0.0, 2U), -kEMinusOne, 1e-12);
    double (*pointer)(double) = &lorentz;
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(pointer, 0.0, 1.0, 8U), kAtanOne, 1e-13);
}

TEST(MathNumericIntegration, GaussLegendreEvaluationCountAndEmptyLimits)
{
    unsigned calls = 0U;
    const counting_functor functor = {&calls};
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(functor, 0.0, 4.0, 3U), 8.0, 1e-12);
    EXPECT_EQ(calls, 15U); // 5 per panel
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(square_fn, 1.0, 1.0, 2U), 0.0, 1e-15);
}

TEST(MathNumericIntegration, GaussLegendreFloat)
{
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(square_fn_f, 0.0F, 3.0F, 2U), 9.0F, 1e-4F);
    EXPECT_NEAR(castle::math::integrate_gauss_legendre(square_fn_f, 0.0F, 3.0F), 9.0F, 1e-4F);
}

TEST(MathNumericIntegration, GaussLegendreConstexpr)
{
    constexpr double value = castle::math::integrate_gauss_legendre([](double x) { return x * x; }, 0.0, 3.0);
    static_assert(value > 8.99 && value < 9.01, "gauss");
    EXPECT_NEAR(value, 9.0, 1e-13);
}

// ---------------------------------------------------------------------------
// integrate_romberg
// ---------------------------------------------------------------------------

TEST(MathNumericIntegration, RombergConvergesOnPolynomial)
{
    const auto cube = [](double x) { return x * x * x; };
    const auto result = castle::math::integrate_romberg(cube, 0.0, 2.0, 1e-12);
    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.code, castle::status::ok);
    EXPECT_NEAR(result.value, 4.0, 1e-10);
    EXPECT_LE(result.error, 1e-12 * 4.0);
    EXPECT_GE(result.iterations, 2U);
    EXPECT_LE(result.iterations, 6U);
}

TEST(MathNumericIntegration, RombergLinearConvergesAtFirstRefinement)
{
    const auto linear = [](double x) { return 3.0 * x + 1.0; };
    const auto result = castle::math::integrate_romberg(linear, 0.0, 2.0, 1e-12);
    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.iterations, 2U);
    EXPECT_NEAR(result.value, 8.0, 1e-12);
    EXPECT_NEAR(result.error, 0.0, 1e-12);

    const auto minimal = castle::math::integrate_romberg<2U>(linear, 0.0, 2.0, 1e-12);
    EXPECT_TRUE(minimal.ok());
    EXPECT_EQ(minimal.iterations, 2U);
}

TEST(MathNumericIntegration, RombergSmoothFunctions)
{
    const auto expo = castle::math::integrate_romberg(exp_fn, 0.0, 1.0, 1e-10);
    EXPECT_TRUE(expo.ok());
    EXPECT_NEAR(expo.value, kEMinusOne, 1e-9);

    const auto lor = castle::math::integrate_romberg(lorentz, 0.0, 1.0, 1e-10);
    EXPECT_TRUE(lor.ok());
    EXPECT_NEAR(lor.value, kAtanOne, 1e-9);
    EXPECT_GT(lor.iterations, 3U);
    EXPECT_LE(lor.iterations, 12U);
}

TEST(MathNumericIntegration, RombergHandlesFunctorsPointersAndReversedLimits)
{
    const auto forward = castle::math::integrate_romberg(cubic_functor{}, 0.0, 2.0, 1e-12);
    const auto reversed = castle::math::integrate_romberg(cubic_functor{}, 2.0, 0.0, 1e-12);
    EXPECT_TRUE(forward.ok());
    EXPECT_TRUE(reversed.ok());
    EXPECT_NEAR(reversed.value, -forward.value, 1e-10);

    double (*pointer)(double) = &exp_fn;
    const auto viaPointer = castle::math::integrate_romberg(pointer, 0.0, 1.0, 1e-10);
    EXPECT_TRUE(viaPointer.ok());
    EXPECT_NEAR(viaPointer.value, kEMinusOne, 1e-9);
}

TEST(MathNumericIntegration, RombergLargeMagnitudeUsesRelativeTolerance)
{
    const auto big = [](double x) { return 1000.0 * x * x; };
    const auto result = castle::math::integrate_romberg(big, 0.0, 30.0, 1e-12);
    EXPECT_TRUE(result.ok());
    EXPECT_NEAR(result.value, 9.0e6, 1e-3);
}

TEST(MathNumericIntegration, RombergNotConvergedReportsOutOfRangeWithBestEstimate)
{
    const auto minimal = castle::math::integrate_romberg<2U>(lorentz, 0.0, 1.0, 1e-14);
    EXPECT_FALSE(minimal.ok());
    EXPECT_EQ(minimal.code, castle::status::out_of_range);
    EXPECT_EQ(minimal.iterations, 2U);
    EXPECT_GT(minimal.error, 1e-14);
    EXPECT_NEAR(minimal.value, kAtanOne, 5e-3);

    const auto small = castle::math::integrate_romberg<3U>(lorentz, 0.0, 1.0, 1e-14);
    EXPECT_FALSE(small.ok());
    EXPECT_EQ(small.code, castle::status::out_of_range);
    EXPECT_EQ(small.iterations, 3U);
    EXPECT_NEAR(small.value, kAtanOne, 1e-3);

    const auto exhausted = castle::math::integrate_romberg<5U>(exp_fn, 0.0, 3.0, 1e-15);
    EXPECT_EQ(exhausted.code, castle::status::out_of_range);
    EXPECT_EQ(exhausted.iterations, 5U);
    EXPECT_NEAR(exhausted.value, castle::math::exp(3.0) - 1.0, 1e-3);
}

TEST(MathNumericIntegration, RombergMaxLevelsBoundaryInstantiation)
{
    const auto cube = [](double x) { return x * x * x; };
    const auto widest = castle::math::integrate_romberg<30U>(cube, 0.0, 2.0, 1e-12);
    EXPECT_TRUE(widest.ok());
    EXPECT_NEAR(widest.value, 4.0, 1e-10);
}

TEST(MathNumericIntegration, RombergFloat)
{
    const auto ok = castle::math::integrate_romberg(square_fn_f, 0.0F, 3.0F, 1e-5F);
    EXPECT_TRUE(ok.ok());
    EXPECT_NEAR(ok.value, 9.0F, 1e-4F);

    const auto limited = castle::math::integrate_romberg<2U>([](float x) { return 1.0F / (1.0F + x * x); }, 0.0F, 1.0F, 1e-9F);
    EXPECT_EQ(limited.code, castle::status::out_of_range);
    EXPECT_NEAR(limited.value, 0.785398F, 5e-3F);
}

// ---------------------------------------------------------------------------
// integrate_trapezoid_samples
// ---------------------------------------------------------------------------

TEST(MathNumericIntegration, TrapezoidSamplesDegenerateCounts)
{
    const double y[] = {1.0, 2.0, 3.0};
    EXPECT_DOUBLE_EQ(castle::math::integrate_trapezoid_samples(y, 0U, 1.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::integrate_trapezoid_samples(y, 1U, 1.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::integrate_trapezoid_samples<double>(nullptr, 0U, 1.0), 0.0);
}

TEST(MathNumericIntegration, TrapezoidSamplesRegularCounts)
{
    const double y[] = {0.0, 1.0, 4.0, 9.0, 16.0};
    EXPECT_NEAR(castle::math::integrate_trapezoid_samples(y, 2U, 0.5), 0.25, 1e-15);
    EXPECT_NEAR(castle::math::integrate_trapezoid_samples(y, 3U, 1.0), 3.0, 1e-15);
    EXPECT_NEAR(castle::math::integrate_trapezoid_samples(y, 5U, 1.0), 22.0, 1e-14);
    EXPECT_NEAR(castle::math::integrate_trapezoid_samples(y, 5U, 0.5), 11.0, 1e-14);
    EXPECT_NEAR(castle::math::integrate_trapezoid_samples(y, 5U, -1.0), -22.0, 1e-14);
}

TEST(MathNumericIntegration, TrapezoidSamplesFloatAndConstexpr)
{
    const float y[] = {0.0F, 1.0F, 4.0F, 9.0F, 16.0F};
    EXPECT_NEAR(castle::math::integrate_trapezoid_samples(y, 5U, 1.0F), 22.0F, 1e-5F);
    static constexpr double table[] = {1.0, 1.0, 1.0};
    constexpr double value = castle::math::integrate_trapezoid_samples(table, 3U, 2.0);
    static_assert(value == 4.0, "samples");
    EXPECT_DOUBLE_EQ(value, 4.0);
}

// ---------------------------------------------------------------------------
// integrate_simpson_samples
// ---------------------------------------------------------------------------

TEST(MathNumericIntegration, SimpsonSamplesDegenerateCounts)
{
    const double y[] = {1.0, 2.0};
    EXPECT_DOUBLE_EQ(castle::math::integrate_simpson_samples(y, 0U, 1.0), 0.0);
    EXPECT_DOUBLE_EQ(castle::math::integrate_simpson_samples(y, 1U, 1.0), 0.0);
}

TEST(MathNumericIntegration, SimpsonSamplesSingleIntervalTrapezoidFallback)
{
    const double y[] = {1.0, 3.0};
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 2U, 0.5), 1.0, 1e-15);
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 2U, 2.0), 4.0, 1e-15);
}

TEST(MathNumericIntegration, SimpsonSamplesEvenIntervals)
{
    // y = x^2 at x = 0..6 (dx = 1): even interval counts use plain Simpson, exact for quadratics.
    const double y[] = {0.0, 1.0, 4.0, 9.0, 16.0, 25.0, 36.0};
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 3U, 1.0), 8.0 / 3.0, 1e-13);   // 2 intervals
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 5U, 1.0), 64.0 / 3.0, 1e-13);  // 4 intervals
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 7U, 1.0), 72.0, 1e-12);        // 6 intervals
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 5U, 0.5), 32.0 / 3.0, 1e-13);
}

TEST(MathNumericIntegration, SimpsonSamplesOddIntervalsUseThreeEighths)
{
    // Cubic samples y = x^3 (dx = 1); both Simpson and Simpson 3/8 are exact for cubics.
    const double y[] = {0.0, 1.0, 8.0, 27.0, 64.0, 125.0, 216.0, 343.0, 512.0};
    // 3 intervals: prefix == 0, only the 3/8 panel. Integral of x^3 on [0, 3] = 81 / 4.
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 4U, 1.0), 81.0 / 4.0, 1e-12);
    // 5 intervals: Simpson over 2 + 3/8 over 3. Integral on [0, 5] = 625 / 4.
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 6U, 1.0), 625.0 / 4.0, 1e-11);
    // 7 intervals: Simpson over 4 + 3/8 over 3. Integral on [0, 7] = 2401 / 4.
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 8U, 1.0), 2401.0 / 4.0, 1e-10);
    // 8 intervals: even.
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 9U, 1.0), 1024.0, 1e-9);
    // Spacing scales the result by dx^4 for x^3 in index space.
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 4U, 0.5), 81.0 / 8.0, 1e-12);
}

TEST(MathNumericIntegration, SimpsonSamplesOddIntervalsQuadratic)
{
    const double y[] = {0.0, 1.0, 4.0, 9.0, 16.0, 25.0};
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 4U, 1.0), 9.0, 1e-13);           // [0,3] -> 9
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 6U, 1.0), 125.0 / 3.0, 1e-12);   // [0,5]
}

TEST(MathNumericIntegration, SimpsonSamplesNegativeSpacing)
{
    const double y[] = {0.0, 1.0, 4.0, 9.0, 16.0};
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 5U, -1.0), -64.0 / 3.0, 1e-13);
}

TEST(MathNumericIntegration, SimpsonSamplesFloatAndConstexpr)
{
    const float y[] = {0.0F, 1.0F, 4.0F, 9.0F, 16.0F, 25.0F};
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 5U, 1.0F), 64.0F / 3.0F, 1e-4F);
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 6U, 1.0F), 125.0F / 3.0F, 1e-3F);
    EXPECT_NEAR(castle::math::integrate_simpson_samples(y, 4U, 1.0F), 9.0F, 1e-4F);
    static constexpr double table[] = {1.0, 1.0, 1.0, 1.0, 1.0};
    constexpr double value = castle::math::integrate_simpson_samples(table, 5U, 0.5);
    static_assert(value > 1.99 && value < 2.01, "simpson samples");
    EXPECT_NEAR(value, 2.0, 1e-12);
}

// ---------------------------------------------------------------------------
// integrate_cumulative_trapezoid
// ---------------------------------------------------------------------------

TEST(MathNumericIntegration, CumulativeTrapezoidValues)
{
    const double y[] = {0.0, 1.0, 4.0, 9.0, 16.0};
    double out[5] = {-1.0, -1.0, -1.0, -1.0, -1.0};
    EXPECT_EQ(castle::math::integrate_cumulative_trapezoid(y, 5U, 1.0, out), castle::status::ok);
    EXPECT_DOUBLE_EQ(out[0], 0.0);
    EXPECT_NEAR(out[1], 0.5, 1e-15);
    EXPECT_NEAR(out[2], 3.0, 1e-15);
    EXPECT_NEAR(out[3], 9.5, 1e-15);
    EXPECT_NEAR(out[4], 22.0, 1e-14);
    EXPECT_NEAR(out[4], castle::math::integrate_trapezoid_samples(y, 5U, 1.0), 1e-14);
}

TEST(MathNumericIntegration, CumulativeTrapezoidSingleSample)
{
    const double y[] = {7.0};
    double out[1] = {-1.0};
    EXPECT_EQ(castle::math::integrate_cumulative_trapezoid(y, 1U, 0.5, out), castle::status::ok);
    EXPECT_DOUBLE_EQ(out[0], 0.0);
}

TEST(MathNumericIntegration, CumulativeTrapezoidAliasingOutEqualsY)
{
    double buffer[5] = {0.0, 1.0, 4.0, 9.0, 16.0};
    EXPECT_EQ(castle::math::integrate_cumulative_trapezoid(buffer, 5U, 1.0, buffer), castle::status::ok);
    EXPECT_DOUBLE_EQ(buffer[0], 0.0);
    EXPECT_NEAR(buffer[1], 0.5, 1e-15);
    EXPECT_NEAR(buffer[2], 3.0, 1e-15);
    EXPECT_NEAR(buffer[3], 9.5, 1e-15);
    EXPECT_NEAR(buffer[4], 22.0, 1e-14);
}

TEST(MathNumericIntegration, CumulativeTrapezoidStatusPaths)
{
    const double y[] = {1.0, 2.0, 3.0};
    double out[3] = {-1.0, -1.0, -1.0};
    EXPECT_EQ(castle::math::integrate_cumulative_trapezoid<double>(nullptr, 3U, 1.0, out), castle::status::invalid_argument);
    EXPECT_EQ(castle::math::integrate_cumulative_trapezoid<double>(y, 3U, 1.0, nullptr), castle::status::invalid_argument);
    EXPECT_EQ(castle::math::integrate_cumulative_trapezoid<double>(nullptr, 3U, 1.0, nullptr), castle::status::invalid_argument);
    EXPECT_EQ(castle::math::integrate_cumulative_trapezoid(y, 0U, 1.0, out), castle::status::empty);
    // Output untouched on every failure path.
    EXPECT_DOUBLE_EQ(out[0], -1.0);
    EXPECT_DOUBLE_EQ(out[2], -1.0);
}

TEST(MathNumericIntegration, CumulativeTrapezoidFloat)
{
    const float y[] = {1.0F, 1.0F, 1.0F};
    float out[3] = {};
    EXPECT_EQ(castle::math::integrate_cumulative_trapezoid(y, 3U, 0.5F, out), castle::status::ok);
    EXPECT_FLOAT_EQ(out[0], 0.0F);
    EXPECT_FLOAT_EQ(out[1], 0.5F);
    EXPECT_FLOAT_EQ(out[2], 1.0F);
}

} // namespace

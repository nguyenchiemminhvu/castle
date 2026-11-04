#include <gtest/gtest.h>

#include "castle/math/abs.hpp"
#include "castle/math/complex.hpp"
#include "castle/math/linalg/vector.hpp"
#include "castle/math/numeric/ode.hpp"

namespace
{

const double kInvE = 0.36787944117144233;

typedef castle::math::vector<double, 2U> Vec2;
typedef castle::math::complex<double> Cplx;

double decay(double, double y)
{
    return -y;
}

struct Growth
{
    double operator()(double, double y) const
    {
        return 2.0 * y;
    }
};

Vec2 oscillator(double, const Vec2& s)
{
    return Vec2(s[1U], -s[0U]);
}

Cplx rotation(double, const Cplx& z)
{
    return Cplx(0.0, 1.0) * z;
}

// Integrates y' = -y on [0, 1] with `steps` equal steps and returns the final error.
double euler_decay_error(unsigned steps)
{
    const double h = 1.0 / static_cast<double>(steps);
    double y = 1.0;
    for (unsigned i = 0U; i < steps; ++i)
    {
        y = castle::math::euler_step(decay, h * static_cast<double>(i), y, h);
    }
    return castle::math::abs(y - kInvE);
}

double rk4_decay_error(unsigned steps)
{
    const double h = 1.0 / static_cast<double>(steps);
    double y = 1.0;
    for (unsigned i = 0U; i < steps; ++i)
    {
        y = castle::math::rk4_step(decay, h * static_cast<double>(i), y, h);
    }
    return castle::math::abs(y - kInvE);
}

// ---------------------------------------------------------------------------
// euler_step
// ---------------------------------------------------------------------------
TEST(MathOde, EulerSingleStepFormula)
{
    EXPECT_DOUBLE_EQ(castle::math::euler_step(decay, 0.0, 1.0, 0.5), 0.5);
    EXPECT_DOUBLE_EQ(castle::math::euler_step(decay, 0.0, 2.0, 0.25), 1.5);
    EXPECT_DOUBLE_EQ(castle::math::euler_step(decay, 0.0, 1.0, 0.0), 1.0);

    // Time-dependent right-hand side: y' = t gives y + t * h.
    const auto ramp = [](double t, double) { return t; };
    EXPECT_DOUBLE_EQ(castle::math::euler_step(ramp, 3.0, 10.0, 0.5), 11.5);
}

TEST(MathOde, EulerScalarDecayTenSteps)
{
    double y = 1.0;
    for (int i = 0; i < 10; ++i)
    {
        y = castle::math::euler_step(decay, 0.1 * static_cast<double>(i), y, 0.1);
    }

    // Explicit Euler multiplies by (1 - h) every step.
    EXPECT_NEAR(y, 0.34867844010000015, 1.0e-12);
    const double error = castle::math::abs(y - kInvE);
    EXPECT_NEAR(error, 0.01920100107144, 1.0e-9);
}

TEST(MathOde, EulerFloat)
{
    const auto f = [](float, float y) { return -y; };
    float y = 1.0F;
    for (int i = 0; i < 10; ++i)
    {
        y = castle::math::euler_step(f, 0.1F * static_cast<float>(i), y, 0.1F);
    }
    EXPECT_NEAR(y, 0.34867844F, 1.0e-6F);
}

TEST(MathOde, EulerNegativeStep)
{
    // y + f * h with h < 0: y' = -y  ->  y * (1 + |h|).
    EXPECT_DOUBLE_EQ(castle::math::euler_step(decay, 1.0, 1.0, -0.5), 1.5);

    // Integrating forward and then backward by the same step does not return to the start for Euler,
    // but remains within O(h^2).
    const double forward = castle::math::euler_step(decay, 0.0, 1.0, 0.01);
    const double back = castle::math::euler_step(decay, 0.01, forward, -0.01);
    EXPECT_NEAR(back, 1.0, 2.0e-4);
}

TEST(MathOde, EulerVectorState)
{
    // (x, v)' = (v, -x) from (1, 0): one step of size h gives (1, -h).
    const Vec2 s0(1.0, 0.0);
    const Vec2 s1 = castle::math::euler_step(oscillator, 0.0, s0, 0.1);
    EXPECT_NEAR(s1[0U], 1.0, 1.0e-15);
    EXPECT_NEAR(s1[1U], -0.1, 1.0e-15);

    // Euler gains energy: |s|^2 grows by (1 + h^2) per step.
    Vec2 s = s0;
    for (int i = 0; i < 10; ++i)
    {
        s = castle::math::euler_step(oscillator, 0.1 * static_cast<double>(i), s, 0.1);
    }
    EXPECT_NEAR(s.dot(s), 1.1046221254112045, 1.0e-12);
}

TEST(MathOde, EulerComplexState)
{
    // z' = i z from z = 1: one step gives 1 + i h.
    const Cplx z1 = castle::math::euler_step(rotation, 0.0, Cplx(1.0, 0.0), 0.1);
    EXPECT_NEAR(z1.real(), 1.0, 1.0e-15);
    EXPECT_NEAR(z1.imag(), 0.1, 1.0e-15);

    Cplx z(1.0, 0.0);
    for (int i = 0; i < 10; ++i)
    {
        z = castle::math::euler_step(rotation, 0.1 * static_cast<double>(i), z, 0.1);
    }
    EXPECT_NEAR((z.real() * z.real()) + (z.imag() * z.imag()), 1.1046221254112045, 1.0e-12);
}

TEST(MathOde, EulerCallableForms)
{
    // Function pointer, functor and capturing lambda.
    EXPECT_DOUBLE_EQ(castle::math::euler_step(&decay, 0.0, 1.0, 0.5), 0.5);
    EXPECT_DOUBLE_EQ(castle::math::euler_step(Growth(), 0.0, 1.0, 0.5), 2.0);

    const double rate = 3.0;
    const auto capture = [rate](double, double y) { return rate * y; };
    EXPECT_DOUBLE_EQ(castle::math::euler_step(capture, 0.0, 1.0, 0.5), 2.5);

    Growth growth;
    EXPECT_DOUBLE_EQ(castle::math::euler_step(growth, 0.0, 2.0, 0.25), 3.0);
}

TEST(MathOde, EulerEvaluatesOnce)
{
    unsigned calls = 0U;
    const auto f = [&calls](double, double y)
    {
        ++calls;
        return -y;
    };
    EXPECT_DOUBLE_EQ(castle::math::euler_step(f, 0.0, 1.0, 0.1), 0.9);
    EXPECT_EQ(calls, 1U);
}

TEST(MathOde, EulerConstexpr)
{
    constexpr auto f = [](double, double y) { return -y; };
    static_assert(castle::math::euler_step(f, 0.0, 1.0, 0.5) == 0.5, "euler_step constexpr");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// rk4_step
// ---------------------------------------------------------------------------
TEST(MathOde, Rk4SingleStepFormula)
{
    // y' = -y, y = 1, h = 0.5: (1 - h + h^2/2 - h^3/6 + h^4/24) = 0.6067708333.
    EXPECT_NEAR(castle::math::rk4_step(decay, 0.0, 1.0, 0.5), 0.6067708333333334, 1.0e-14);
    EXPECT_DOUBLE_EQ(castle::math::rk4_step(decay, 0.0, 1.0, 0.0), 1.0);

    // Exact for polynomial right-hand sides up to degree 3 in t: y' = t^3 -> y = t^4 / 4.
    const auto cubic = [](double t, double) { return t * t * t; };
    EXPECT_NEAR(castle::math::rk4_step(cubic, 0.0, 0.0, 1.0), 0.25, 1.0e-15);
    EXPECT_NEAR(castle::math::rk4_step(cubic, 1.0, 0.25, 1.0), 4.0, 1.0e-14);

    // Constant right-hand side is integrated exactly.
    const auto constant = [](double, double) { return 3.0; };
    EXPECT_DOUBLE_EQ(castle::math::rk4_step(constant, 0.0, 1.0, 0.5), 2.5);
}

TEST(MathOde, Rk4ScalarDecayTenSteps)
{
    double y = 1.0;
    for (int i = 0; i < 10; ++i)
    {
        y = castle::math::rk4_step(decay, 0.1 * static_cast<double>(i), y, 0.1);
    }

    EXPECT_NEAR(y, kInvE, 1.0e-6);
    const double rk4_error = castle::math::abs(y - kInvE);
    EXPECT_LT(rk4_error, 1.0e-6);

    double y_euler = 1.0;
    for (int i = 0; i < 10; ++i)
    {
        y_euler = castle::math::euler_step(decay, 0.1 * static_cast<double>(i), y_euler, 0.1);
    }
    const double euler_error = castle::math::abs(y_euler - kInvE);
    EXPECT_LT(rk4_error * 1.0e4, euler_error);
}

TEST(MathOde, Rk4Float)
{
    const auto f = [](float, float y) { return -y; };
    float y = 1.0F;
    for (int i = 0; i < 10; ++i)
    {
        y = castle::math::rk4_step(f, 0.1F * static_cast<float>(i), y, 0.1F);
    }
    EXPECT_NEAR(y, 0.36787945F, 1.0e-5F);

    const auto ramp = [](float t, float) { return t; };
    EXPECT_NEAR(castle::math::rk4_step(ramp, 0.0F, 0.0F, 1.0F), 0.5F, 1.0e-6F);
}

TEST(MathOde, Rk4NegativeStep)
{
    // Stepping forward and then backward returns to the start with RK4 accuracy.
    const double forward = castle::math::rk4_step(decay, 0.0, 1.0, 0.1);
    const double back = castle::math::rk4_step(decay, 0.1, forward, -0.1);
    EXPECT_NEAR(back, 1.0, 1.0e-6);

    // Integrating backwards from e^-1 at t = 1 to t = 0 recovers y(0) = 1.
    double y = kInvE;
    for (int i = 0; i < 10; ++i)
    {
        y = castle::math::rk4_step(decay, 1.0 - (0.1 * static_cast<double>(i)), y, -0.1);
    }
    EXPECT_NEAR(y, 1.0, 1.0e-5);
}

TEST(MathOde, Rk4VectorHarmonicOscillator)
{
    // x'' = -x, x(0) = 1, v(0) = 0  ->  x(t) = cos t, v(t) = -sin t.
    Vec2 s(1.0, 0.0);
    const double h = 0.01;
    for (int i = 0; i < 100; ++i)
    {
        s = castle::math::rk4_step(oscillator, h * static_cast<double>(i), s, h);
    }
    EXPECT_NEAR(s[0U], 0.5403023058681398, 1.0e-9);
    EXPECT_NEAR(s[1U], -0.8414709848078965, 1.0e-9);

    // Energy is conserved to RK4 accuracy.
    EXPECT_NEAR(s.dot(s), 1.0, 1.0e-9);
}

TEST(MathOde, Rk4VectorFloat)
{
    typedef castle::math::vector<float, 2U> Vec2f;
    const auto f = [](float, const Vec2f& s) { return Vec2f(s[1U], -s[0U]); };
    Vec2f s(1.0F, 0.0F);
    for (int i = 0; i < 100; ++i)
    {
        s = castle::math::rk4_step(f, 0.01F * static_cast<float>(i), s, 0.01F);
    }
    EXPECT_NEAR(s[0U], 0.54030231F, 1.0e-5F);
    EXPECT_NEAR(s[1U], -0.84147098F, 1.0e-5F);
}

TEST(MathOde, Rk4ComplexRotation)
{
    // z' = i z, z(0) = 1  ->  z(t) = e^{i t}.
    Cplx z(1.0, 0.0);
    const double h = 0.01;
    for (int i = 0; i < 100; ++i)
    {
        z = castle::math::rk4_step(rotation, h * static_cast<double>(i), z, h);
    }
    EXPECT_NEAR(z.real(), 0.5403023058681398, 1.0e-9);
    EXPECT_NEAR(z.imag(), 0.8414709848078965, 1.0e-9);
    EXPECT_NEAR((z.real() * z.real()) + (z.imag() * z.imag()), 1.0, 1.0e-9);

    // A quarter turn: t = pi/2 in 50 steps ends close to i.
    const double quarter = 1.5707963267948966;
    const unsigned steps = 50U;
    const double hq = quarter / static_cast<double>(steps);
    Cplx w(1.0, 0.0);
    for (unsigned i = 0U; i < steps; ++i)
    {
        w = castle::math::rk4_step(rotation, hq * static_cast<double>(i), w, hq);
    }
    EXPECT_NEAR(w.real(), 0.0, 1.0e-7);
    EXPECT_NEAR(w.imag(), 1.0, 1.0e-7);
}

TEST(MathOde, Rk4TimeDependentSource)
{
    // y' = y + t (t starts at 0.5): exercises the t + h/2 and t + h stage times.
    // Closed form: y(t) = 2 e^{t - 0.5} - t - 1 with y(0.5) = 0.5.
    const auto f = [](double t, double y) { return y + t; };
    const double t0 = 0.5;
    const double h = 0.01;
    double y = 0.5;
    for (int i = 0; i < 100; ++i)
    {
        y = castle::math::rk4_step(f, t0 + (h * static_cast<double>(i)), y, h);
    }
    // y(1.5) = 2 e - 2.5.
    EXPECT_NEAR(y, (2.0 * 2.718281828459045) - 2.5, 1.0e-9);
}

TEST(MathOde, Rk4CallableForms)
{
    EXPECT_NEAR(castle::math::rk4_step(&decay, 0.0, 1.0, 0.5), 0.6067708333333334, 1.0e-14);

    // y' = 2y: (1 + 2h + 2h^2 + (4/3)h^3 + (2/3)h^4) = 2.708333... for h = 0.5.
    EXPECT_NEAR(castle::math::rk4_step(Growth(), 0.0, 1.0, 0.5), 2.7083333333333335, 1.0e-14);

    Growth growth;
    EXPECT_NEAR(castle::math::rk4_step(growth, 0.0, 1.0, 0.5), 2.7083333333333335, 1.0e-14);

    const double rate = -1.0;
    const auto capture = [rate](double, double y) { return rate * y; };
    EXPECT_NEAR(castle::math::rk4_step(capture, 0.0, 1.0, 0.5), 0.6067708333333334, 1.0e-14);
}

TEST(MathOde, Rk4EvaluatesFourTimes)
{
    unsigned calls = 0U;
    const auto f = [&calls](double, double y)
    {
        ++calls;
        return -y;
    };
    EXPECT_NEAR(castle::math::rk4_step(f, 0.0, 1.0, 0.1), 0.9048375, 1.0e-7);
    EXPECT_EQ(calls, 4U);
}

TEST(MathOde, Rk4Constexpr)
{
    constexpr auto f = [](double, double y) { return -y; };
    constexpr double y = castle::math::rk4_step(f, 0.0, 1.0, 0.5);
    static_assert((y > 0.6067708333) && (y < 0.6067708334), "rk4_step constexpr");
    EXPECT_NEAR(y, 0.6067708333333334, 1.0e-12);
}

// ---------------------------------------------------------------------------
// Convergence order
// ---------------------------------------------------------------------------
TEST(MathOde, RkMuchMoreAccurateThanEuler)
{
    for (unsigned steps = 10U; steps <= 80U; steps *= 2U)
    {
        EXPECT_LT(rk4_decay_error(steps) * 1000.0, euler_decay_error(steps)) << "steps=" << steps;
    }
}

TEST(MathOde, EulerIsFirstOrder)
{
    // Halving h halves the global error.
    const double ratio = euler_decay_error(100U) / euler_decay_error(200U);
    EXPECT_NEAR(ratio, 2.0, 0.05);

    const double ratio2 = euler_decay_error(200U) / euler_decay_error(400U);
    EXPECT_NEAR(ratio2, 2.0, 0.05);
}

TEST(MathOde, Rk4IsFourthOrder)
{
    // Halving h reduces the global error by 2^4 = 16.
    const double ratio = rk4_decay_error(10U) / rk4_decay_error(20U);
    EXPECT_NEAR(ratio, 16.0, 1.0);

    const double ratio2 = rk4_decay_error(20U) / rk4_decay_error(40U);
    EXPECT_NEAR(ratio2, 16.0, 1.0);
}

} // namespace

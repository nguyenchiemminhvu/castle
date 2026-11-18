
#include <gtest/gtest.h>

#include <limits.h>

#include "castle/core/constants.hpp"
#include "castle/math/complex.hpp"
#include "castle/math/near_equal.hpp"

namespace
{

using castle::math::complexd;
using castle::math::complexf;

template <typename T>
void expect_complex_near(const castle::math::complex<T>& actual,
                         const castle::math::complex<T>& expected,
                         T tolerance)
{
    EXPECT_NEAR(static_cast<double>(actual.real()), static_cast<double>(expected.real()), static_cast<double>(tolerance));
    EXPECT_NEAR(static_cast<double>(actual.imag()), static_cast<double>(expected.imag()), static_cast<double>(tolerance));
}

TEST(MathComplex, ValueTypeAndAliases)
{
    static_assert(sizeof(complexf) == 2U * sizeof(float), "complexf layout");
    static_assert(sizeof(complexd) == 2U * sizeof(double), "complexd layout");
    static_assert(castle::meta::is_same<complexf::value_type, float>::value, "complexf value_type");
    static_assert(castle::meta::is_same<complexd::value_type, double>::value, "complexd value_type");
    static_assert(castle::meta::is_same<complexf, castle::math::complex<float>>::value, "complexf alias");
    static_assert(castle::meta::is_same<complexd, castle::math::complex<double>>::value, "complexd alias");
    SUCCEED();
}

TEST(MathComplex, DefaultAndValueConstruction)
{
    const complexf zero;
    EXPECT_EQ(zero.real(), 0.0F);
    EXPECT_EQ(zero.imag(), 0.0F);

    const complexd from_real(2.5);
    EXPECT_EQ(from_real.real(), 2.5);
    EXPECT_EQ(from_real.imag(), 0.0);

    const complexd full(1.5, -2.5);
    EXPECT_EQ(full.real(), 1.5);
    EXPECT_EQ(full.imag(), -2.5);

    static_assert(complexd().real() == 0.0, "default real");
    static_assert(complexd().imag() == 0.0, "default imag");
    static_assert(complexd(1.0, 2.0).real() == 1.0, "constexpr real");
    static_assert(complexd(1.0, 2.0).imag() == 2.0, "constexpr imag");
}

TEST(MathComplex, ConvertingConstructor)
{
    const complexf narrow(1.5F, -0.25F);
    const complexd wide(narrow);
    EXPECT_EQ(wide.real(), 1.5);
    EXPECT_EQ(wide.imag(), -0.25);

    const complexd source(0.5, 0.125);
    const complexf back(source);
    EXPECT_EQ(back.real(), 0.5F);
    EXPECT_EQ(back.imag(), 0.125F);

    // Precision is dropped when narrowing.
    const complexf rounded(complexd(0.1, 0.2));
    EXPECT_EQ(rounded.real(), 0.1F);
    EXPECT_EQ(rounded.imag(), 0.2F);

    static_assert(complexd(complexf(1.0F, 2.0F)).imag() == 2.0, "constexpr conversion");
}

TEST(MathComplex, SettersReplaceComponents)
{
    complexd z(1.0, 2.0);
    z.real(-3.0);
    EXPECT_EQ(z.real(), -3.0);
    EXPECT_EQ(z.imag(), 2.0);
    z.imag(7.0);
    EXPECT_EQ(z.real(), -3.0);
    EXPECT_EQ(z.imag(), 7.0);

    complexf f;
    f.real(4.0F);
    f.imag(-4.0F);
    EXPECT_EQ(f.real(), 4.0F);
    EXPECT_EQ(f.imag(), -4.0F);
}

TEST(MathComplex, FreeRealAndImagAccessors)
{
    const complexd z(1.25, -8.5);
    EXPECT_EQ(castle::math::real(z), 1.25);
    EXPECT_EQ(castle::math::imag(z), -8.5);
    static_assert(castle::math::real(complexd(3.0, 4.0)) == 3.0, "free real");
    static_assert(castle::math::imag(complexd(3.0, 4.0)) == 4.0, "free imag");

    const complexf f(0.5F, 0.75F);
    EXPECT_EQ(castle::math::real(f), 0.5F);
    EXPECT_EQ(castle::math::imag(f), 0.75F);
}

TEST(MathComplex, UnaryOperators)
{
    const complexd z(1.0, -2.0);
    EXPECT_EQ((+z).real(), 1.0);
    EXPECT_EQ((+z).imag(), -2.0);
    EXPECT_EQ((-z).real(), -1.0);
    EXPECT_EQ((-z).imag(), 2.0);
    static_assert((-complexd(1.0, -2.0)).imag() == 2.0, "constexpr negation");
    static_assert((+complexd(1.0, -2.0)).real() == 1.0, "constexpr identity");
}

TEST(MathComplex, RealAssignmentClearsImaginary)
{
    complexd z(1.0, 9.0);
    complexd& result = (z = 4.5);
    EXPECT_EQ(&result, &z);
    EXPECT_EQ(z.real(), 4.5);
    EXPECT_EQ(z.imag(), 0.0);

    complexf f(2.0F, 3.0F);
    f = -1.0F;
    EXPECT_EQ(f.real(), -1.0F);
    EXPECT_EQ(f.imag(), 0.0F);
}

TEST(MathComplex, RealCompoundAssignment)
{
    complexd z(2.0, 3.0);
    complexd* self = &z;

    EXPECT_EQ(&(z += 1.0), self);
    EXPECT_EQ(z.real(), 3.0);
    EXPECT_EQ(z.imag(), 3.0);

    EXPECT_EQ(&(z -= 0.5), self);
    EXPECT_EQ(z.real(), 2.5);
    EXPECT_EQ(z.imag(), 3.0);

    EXPECT_EQ(&(z *= 2.0), self);
    EXPECT_EQ(z.real(), 5.0);
    EXPECT_EQ(z.imag(), 6.0);

    EXPECT_EQ(&(z /= 4.0), self);
    EXPECT_EQ(z.real(), 1.25);
    EXPECT_EQ(z.imag(), 1.5);
}

TEST(MathComplex, ComplexCompoundAssignment)
{
    complexd z(1.0, 2.0);
    complexd* self = &z;

    EXPECT_EQ(&(z += complexd(3.0, -1.0)), self);
    EXPECT_EQ(z.real(), 4.0);
    EXPECT_EQ(z.imag(), 1.0);

    EXPECT_EQ(&(z -= complexd(1.0, 4.0)), self);
    EXPECT_EQ(z.real(), 3.0);
    EXPECT_EQ(z.imag(), -3.0);

    // (3 - 3i)(2 + i) = 6 + 3i - 6i + 3 = 9 - 3i
    EXPECT_EQ(&(z *= complexd(2.0, 1.0)), self);
    EXPECT_EQ(z.real(), 9.0);
    EXPECT_EQ(z.imag(), -3.0);

    // (9 - 3i) / (2 + i) = (9 - 3i)(2 - i) / 5 = (15 - 15i) / 5 = 3 - 3i
    EXPECT_EQ(&(z /= complexd(2.0, 1.0)), self);
    EXPECT_NEAR(z.real(), 3.0, 1.0e-12);
    EXPECT_NEAR(z.imag(), -3.0, 1.0e-12);

    // Multiplying by itself must read both operands before writing.
    complexd sq(1.0, 1.0);
    sq *= sq;
    EXPECT_EQ(sq.real(), 0.0);
    EXPECT_EQ(sq.imag(), 2.0);
}

TEST(MathComplex, DivisionRealDominantDenominator)
{
    // |c| >= |d|: first Smith branch.
    const complexd q = complexd(1.0, 2.0) / complexd(3.0, -1.0);
    EXPECT_NEAR(q.real(), 0.1, 1.0e-15);
    EXPECT_NEAR(q.imag(), 0.7, 1.0e-15);

    // |c| == |d| takes the same (>=) branch.
    const complexd eq = complexd(2.0, 0.0) / complexd(1.0, 1.0);
    EXPECT_NEAR(eq.real(), 1.0, 1.0e-15);
    EXPECT_NEAR(eq.imag(), -1.0, 1.0e-15);

    const complexf qf = complexf(1.0F, 2.0F) / complexf(3.0F, -1.0F);
    EXPECT_NEAR(qf.real(), 0.1F, 1.0e-6F);
    EXPECT_NEAR(qf.imag(), 0.7F, 1.0e-6F);
}

TEST(MathComplex, DivisionImaginaryDominantDenominator)
{
    // |c| < |d|: second Smith branch. (1 + 2i)/(1 + 3i) = (7 - i)/10
    const complexd q = complexd(1.0, 2.0) / complexd(1.0, 3.0);
    EXPECT_NEAR(q.real(), 0.7, 1.0e-15);
    EXPECT_NEAR(q.imag(), -0.1, 1.0e-15);

    // Pure imaginary divisor: 4 / (2i) = -2i
    const complexd pure = complexd(4.0, 0.0) / complexd(0.0, 2.0);
    EXPECT_NEAR(pure.real(), 0.0, 1.0e-15);
    EXPECT_NEAR(pure.imag(), -2.0, 1.0e-15);

    const complexf qf = complexf(1.0F, 2.0F) / complexf(1.0F, 3.0F);
    EXPECT_NEAR(qf.real(), 0.7F, 1.0e-6F);
    EXPECT_NEAR(qf.imag(), -0.1F, 1.0e-6F);
}

TEST(MathComplex, DivisionAvoidsSpuriousOverflow)
{
    // Naive c^2 + d^2 would overflow float; Smith's algorithm does not.
    const complexf big(1.0e30F, 1.0e30F);
    const complexf q = big / big;
    EXPECT_NEAR(q.real(), 1.0F, 1.0e-6F);
    EXPECT_NEAR(q.imag(), 0.0F, 1.0e-6F);
}

TEST(MathComplex, BinaryAddition)
{
    const complexd a(1.0, 2.0);
    const complexd b(3.0, -1.0);
    EXPECT_EQ(a + b, complexd(4.0, 1.0));
    EXPECT_EQ(a + 1.5, complexd(2.5, 2.0));
    EXPECT_EQ(1.5 + a, complexd(2.5, 2.0));

    const complexf af(1.0F, 2.0F);
    EXPECT_EQ(af + complexf(0.5F, 0.5F), complexf(1.5F, 2.5F));
    EXPECT_EQ(af + 1.0F, complexf(2.0F, 2.0F));
    EXPECT_EQ(1.0F + af, complexf(2.0F, 2.0F));

    static_assert(complexd(1.0, 2.0) + complexd(3.0, 4.0) == complexd(4.0, 6.0), "constexpr add");
    static_assert(complexd(1.0, 2.0) + 1.0 == complexd(2.0, 2.0), "constexpr add real rhs");
    static_assert(1.0 + complexd(1.0, 2.0) == complexd(2.0, 2.0), "constexpr add real lhs");
}

TEST(MathComplex, BinarySubtraction)
{
    const complexd a(1.0, 2.0);
    const complexd b(3.0, -1.0);
    EXPECT_EQ(a - b, complexd(-2.0, 3.0));
    EXPECT_EQ(a - 1.5, complexd(-0.5, 2.0));
    EXPECT_EQ(1.5 - a, complexd(0.5, -2.0));

    const complexf af(1.0F, 2.0F);
    EXPECT_EQ(af - complexf(0.5F, 0.5F), complexf(0.5F, 1.5F));
    EXPECT_EQ(af - 1.0F, complexf(0.0F, 2.0F));
    EXPECT_EQ(1.0F - af, complexf(0.0F, -2.0F));

    static_assert(complexd(1.0, 2.0) - complexd(3.0, 4.0) == complexd(-2.0, -2.0), "constexpr sub");
    static_assert(complexd(1.0, 2.0) - 1.0 == complexd(0.0, 2.0), "constexpr sub real rhs");
    static_assert(1.0 - complexd(1.0, 2.0) == complexd(0.0, -2.0), "constexpr sub real lhs");
}

TEST(MathComplex, BinaryMultiplication)
{
    const complexd a(1.0, 2.0);
    const complexd b(3.0, -1.0);
    EXPECT_EQ(a * b, complexd(5.0, 5.0));
    EXPECT_EQ(a * 2.0, complexd(2.0, 4.0));
    EXPECT_EQ(2.0 * a, complexd(2.0, 4.0));

    // i * i = -1
    EXPECT_EQ(complexd(0.0, 1.0) * complexd(0.0, 1.0), complexd(-1.0, 0.0));

    const complexf af(1.0F, 2.0F);
    EXPECT_EQ(af * complexf(3.0F, -1.0F), complexf(5.0F, 5.0F));
    EXPECT_EQ(af * 3.0F, complexf(3.0F, 6.0F));
    EXPECT_EQ(3.0F * af, complexf(3.0F, 6.0F));

    static_assert(complexd(1.0, 2.0) * complexd(3.0, -1.0) == complexd(5.0, 5.0), "constexpr mul");
    static_assert(complexd(1.0, 2.0) * 2.0 == complexd(2.0, 4.0), "constexpr mul real rhs");
    static_assert(2.0 * complexd(1.0, 2.0) == complexd(2.0, 4.0), "constexpr mul real lhs");
}

TEST(MathComplex, BinaryDivision)
{
    const complexd a(1.0, 2.0);
    const complexd b(3.0, -1.0);
    expect_complex_near(a / b, complexd(0.1, 0.7), 1.0e-15);
    EXPECT_EQ(a / 2.0, complexd(0.5, 1.0));
    // 2 / (1 + 2i) = 2(1 - 2i)/5 = 0.4 - 0.8i
    expect_complex_near(2.0 / a, complexd(0.4, -0.8), 1.0e-15);

    const complexf af(1.0F, 2.0F);
    expect_complex_near(af / complexf(3.0F, -1.0F), complexf(0.1F, 0.7F), 1.0e-6F);
    EXPECT_EQ(af / 2.0F, complexf(0.5F, 1.0F));
    expect_complex_near(2.0F / af, complexf(0.4F, -0.8F), 1.0e-6F);

    static_assert(complexd(1.0, 2.0) / 2.0 == complexd(0.5, 1.0), "constexpr div real rhs");
    static_assert(complexd(4.0, 2.0) / complexd(2.0, 0.0) == complexd(2.0, 1.0), "constexpr div");
    static_assert(4.0 / complexd(2.0, 0.0) == complexd(2.0, 0.0), "constexpr div real lhs");
}

TEST(MathComplex, EqualityAndInequality)
{
    const complexd a(1.0, 2.0);
    EXPECT_TRUE(a == complexd(1.0, 2.0));
    EXPECT_FALSE(a == complexd(1.0, 2.5));
    EXPECT_FALSE(a == complexd(1.5, 2.0));
    EXPECT_TRUE(a != complexd(1.0, 2.5));
    EXPECT_TRUE(a != complexd(1.5, 2.0));
    EXPECT_FALSE(a != complexd(1.0, 2.0));

    // Real right-hand operand.
    EXPECT_TRUE(complexd(2.0, 0.0) == 2.0);
    EXPECT_FALSE(complexd(2.0, 1.0) == 2.0);
    EXPECT_FALSE(complexd(3.0, 0.0) == 2.0);
    EXPECT_FALSE(complexd(2.0, 0.0) != 2.0);
    EXPECT_TRUE(complexd(2.0, 1.0) != 2.0);
    EXPECT_TRUE(complexd(3.0, 0.0) != 2.0);

    EXPECT_TRUE(complexf(1.0F, 2.0F) == complexf(1.0F, 2.0F));
    EXPECT_TRUE(complexf(1.0F, 2.0F) != complexf(1.0F, 3.0F));
    EXPECT_TRUE(complexf(2.0F) == 2.0F);
    EXPECT_TRUE(complexf(2.0F) != 3.0F);
    EXPECT_FALSE(complexf(1.0F, 2.0F) == complexf(2.0F, 2.0F));
    EXPECT_FALSE(complexf(2.0F, 1.0F) == 2.0F);
    EXPECT_FALSE(complexf(3.0F, 0.0F) == 2.0F);
    EXPECT_TRUE(complexf(2.0F, 1.0F) != 2.0F);
    EXPECT_TRUE(complexf(1.0F, 2.0F) != complexf(2.0F, 2.0F));

    static_assert(complexd(1.0, 2.0) == complexd(1.0, 2.0), "constexpr ==");
    static_assert(complexd(1.0, 2.0) != complexd(2.0, 1.0), "constexpr !=");
    static_assert(complexd(1.0, 0.0) == 1.0, "constexpr == real");
    static_assert(complexd(1.0, 1.0) != 1.0, "constexpr != real");
}

TEST(MathComplex, Conjugate)
{
    EXPECT_EQ(castle::math::conj(complexd(3.0, 4.0)), complexd(3.0, -4.0));
    EXPECT_EQ(castle::math::conj(complexd(3.0, -4.0)), complexd(3.0, 4.0));
    EXPECT_EQ(castle::math::conj(complexf(1.0F, 0.5F)), complexf(1.0F, -0.5F));
    static_assert(castle::math::conj(complexd(1.0, 2.0)) == complexd(1.0, -2.0), "constexpr conj");

    // z * conj(z) = |z|^2
    const complexd z(3.0, 4.0);
    EXPECT_EQ(z * castle::math::conj(z), complexd(25.0, 0.0));
}

TEST(MathComplex, NormIsSquaredMagnitude)
{
    EXPECT_EQ(castle::math::norm(complexd(3.0, 4.0)), 25.0);
    EXPECT_EQ(castle::math::norm(complexd(-3.0, -4.0)), 25.0);
    EXPECT_EQ(castle::math::norm(complexd()), 0.0);
    EXPECT_EQ(castle::math::norm(complexf(1.0F, 2.0F)), 5.0F);
    static_assert(castle::math::norm(complexd(3.0, 4.0)) == 25.0, "constexpr norm");
}

TEST(MathComplex, AbsUsesHypot)
{
    EXPECT_NEAR(castle::math::abs(complexd(3.0, 4.0)), 5.0, 1.0e-12);
    EXPECT_NEAR(castle::math::abs(complexd(-3.0, 4.0)), 5.0, 1.0e-12);
    EXPECT_NEAR(castle::math::abs(complexd(0.0, -2.0)), 2.0, 1.0e-12);
    EXPECT_EQ(castle::math::abs(complexd()), 0.0);
    EXPECT_NEAR(castle::math::abs(complexf(3.0F, 4.0F)), 5.0F, 1.0e-6F);

    // norm() would overflow here, hypot-based abs() must not.
    EXPECT_NEAR(castle::math::abs(complexf(3.0e30F, 4.0e30F)) / 1.0e30F, 5.0F, 1.0e-5F);
}

TEST(MathComplex, ArgCoversAllQuadrants)
{
    const double pi = castle::math::pi<double>();
    EXPECT_NEAR(castle::math::arg(complexd(1.0, 0.0)), 0.0, 1.0e-12);
    EXPECT_NEAR(castle::math::arg(complexd(0.0, 1.0)), pi * 0.5, 1.0e-12);
    EXPECT_NEAR(castle::math::arg(complexd(-1.0, 0.0)), pi, 1.0e-12);
    EXPECT_NEAR(castle::math::arg(complexd(0.0, -1.0)), -pi * 0.5, 1.0e-12);
    EXPECT_NEAR(castle::math::arg(complexd(1.0, 1.0)), pi * 0.25, 1.0e-12);
    EXPECT_NEAR(castle::math::arg(complexd(-1.0, 1.0)), pi * 0.75, 1.0e-12);
    EXPECT_NEAR(castle::math::arg(complexd(-1.0, -1.0)), -pi * 0.75, 1.0e-12);
    EXPECT_NEAR(castle::math::arg(complexd(1.0, -1.0)), -pi * 0.25, 1.0e-12);

    EXPECT_NEAR(castle::math::arg(complexf(0.0F, 2.0F)), castle::math::pi<float>() * 0.5F, 1.0e-6F);
}

TEST(MathComplex, PolarConstruction)
{
    const double pi = castle::math::pi<double>();
    expect_complex_near(castle::math::polar(2.0, 0.0), complexd(2.0, 0.0), 1.0e-12);
    expect_complex_near(castle::math::polar(2.0, pi * 0.5), complexd(0.0, 2.0), 1.0e-12);
    expect_complex_near(castle::math::polar(2.0, pi), complexd(-2.0, 0.0), 1.0e-12);
    expect_complex_near(castle::math::polar(1.0, -pi * 0.5), complexd(0.0, -1.0), 1.0e-12);

    // Default phase is zero.
    expect_complex_near(castle::math::polar(3.0), complexd(3.0, 0.0), 1.0e-12);
    expect_complex_near(castle::math::polar(3.0F), complexf(3.0F, 0.0F), 1.0e-6F);
    expect_complex_near(castle::math::polar(2.0F, castle::math::pi<float>() * 0.5F), complexf(0.0F, 2.0F), 1.0e-6F);

    // Round trip through abs()/arg().
    const complexd p = castle::math::polar(2.5, 0.75);
    EXPECT_NEAR(castle::math::abs(p), 2.5, 1.0e-12);
    EXPECT_NEAR(castle::math::arg(p), 0.75, 1.0e-12);
}

TEST(MathComplex, CisFunction)
{
    const double pi = castle::math::pi<double>();
    expect_complex_near(castle::math::cis(0.0), complexd(1.0, 0.0), 1.0e-12);
    expect_complex_near(castle::math::cis(pi * 0.5), complexd(0.0, 1.0), 1.0e-12);
    expect_complex_near(castle::math::cis(pi), complexd(-1.0, 0.0), 1.0e-12);
    expect_complex_near(castle::math::cis(-pi * 0.5), complexd(0.0, -1.0), 1.0e-12);

    // cis(phase) == polar(1, phase)
    expect_complex_near(castle::math::cis(0.75), castle::math::polar(1.0, 0.75), 1.0e-12);
    EXPECT_NEAR(castle::math::abs(castle::math::cis(1.3)), 1.0, 1.0e-12);

    expect_complex_near(castle::math::cis(castle::math::pi<float>() * 0.5F), complexf(0.0F, 1.0F), 1.0e-6F);
}

TEST(MathComplex, ExponentialFunction)
{
    const double pi = castle::math::pi<double>();
    // Euler's identity.
    expect_complex_near(castle::math::exp(complexd(0.0, pi)), complexd(-1.0, 0.0), 1.0e-12);
    expect_complex_near(castle::math::exp(complexd(0.0, 0.0)), complexd(1.0, 0.0), 1.0e-12);
    expect_complex_near(castle::math::exp(complexd(1.0, 0.0)), complexd(2.718281828459045, 0.0), 1.0e-12);
    expect_complex_near(castle::math::exp(complexd(0.0, pi * 0.5)), complexd(0.0, 1.0), 1.0e-12);

    // exp(a + bi) = e^a (cos b + i sin b)
    expect_complex_near(castle::math::exp(complexd(1.0, 1.0)),
                        complexd(1.4686939399158851, 2.2873552871788423),
                        1.0e-12);
    expect_complex_near(castle::math::exp(complexf(0.0F, castle::math::pi<float>())),
                        complexf(-1.0F, 0.0F),
                        1.0e-5F);
}

TEST(MathComplex, LogarithmFunction)
{
    const double pi = castle::math::pi<double>();
    expect_complex_near(castle::math::log(complexd(1.0, 0.0)), complexd(0.0, 0.0), 1.0e-12);
    expect_complex_near(castle::math::log(complexd(-1.0, 0.0)), complexd(0.0, pi), 1.0e-12);
    expect_complex_near(castle::math::log(complexd(0.0, 1.0)), complexd(0.0, pi * 0.5), 1.0e-12);
    expect_complex_near(castle::math::log(complexd(0.0, -1.0)), complexd(0.0, -pi * 0.5), 1.0e-12);
    expect_complex_near(castle::math::log(complexd(1.0, 1.0)),
                        complexd(0.34657359027997264, pi * 0.25),
                        1.0e-12);
    expect_complex_near(castle::math::log(complexf(2.7182818F, 0.0F)), complexf(1.0F, 0.0F), 1.0e-6F);

    // exp(log(z)) == z
    const complexd z(-2.0, 3.0);
    EXPECT_TRUE(castle::math::near_equal(castle::math::exp(castle::math::log(z)), z, 1.0e-12));
}

TEST(MathComplex, SqrtOfZero)
{
    EXPECT_EQ(castle::math::sqrt(complexd()), complexd());
    EXPECT_EQ(castle::math::sqrt(complexf()), complexf());
    EXPECT_EQ(castle::math::sqrt(complexd(-0.0, 0.0)), complexd());
}

TEST(MathComplex, SqrtRightHalfPlane)
{
    // real >= 0 branch, positive imaginary: sqrt(3 + 4i) = 2 + i
    expect_complex_near(castle::math::sqrt(complexd(3.0, 4.0)), complexd(2.0, 1.0), 1.0e-12);
    // Negative imaginary keeps a non-negative real part: sqrt(3 - 4i) = 2 - i
    expect_complex_near(castle::math::sqrt(complexd(3.0, -4.0)), complexd(2.0, -1.0), 1.0e-12);
    // Positive real axis.
    expect_complex_near(castle::math::sqrt(complexd(9.0, 0.0)), complexd(3.0, 0.0), 1.0e-12);
    // Imaginary axis (real == 0): sqrt(2i) = 1 + i
    expect_complex_near(castle::math::sqrt(complexd(0.0, 2.0)), complexd(1.0, 1.0), 1.0e-12);
    expect_complex_near(castle::math::sqrt(complexd(0.0, -2.0)), complexd(1.0, -1.0), 1.0e-12);

    expect_complex_near(castle::math::sqrt(complexf(3.0F, 4.0F)), complexf(2.0F, 1.0F), 1.0e-5F);
    expect_complex_near(castle::math::sqrt(complexf(3.0F, -4.0F)), complexf(2.0F, -1.0F), 1.0e-5F);
}

TEST(MathComplex, SqrtLeftHalfPlane)
{
    // real < 0 branch, positive imaginary: sqrt(-3 + 4i) = 1 + 2i
    expect_complex_near(castle::math::sqrt(complexd(-3.0, 4.0)), complexd(1.0, 2.0), 1.0e-12);
    // Negative imaginary: sqrt(-3 - 4i) = 1 - 2i
    expect_complex_near(castle::math::sqrt(complexd(-3.0, -4.0)), complexd(1.0, -2.0), 1.0e-12);
    // Negative real axis (imag == 0 is treated as non-negative): sqrt(-4) = 2i
    expect_complex_near(castle::math::sqrt(complexd(-4.0, 0.0)), complexd(0.0, 2.0), 1.0e-12);
    // Negative imaginary zero is not < 0 either.
    expect_complex_near(castle::math::sqrt(complexd(-4.0, -0.0)), complexd(0.0, 2.0), 1.0e-12);

    expect_complex_near(castle::math::sqrt(complexf(-3.0F, 4.0F)), complexf(1.0F, 2.0F), 1.0e-5F);
    expect_complex_near(castle::math::sqrt(complexf(-3.0F, -4.0F)), complexf(1.0F, -2.0F), 1.0e-5F);
}

TEST(MathComplex, SqrtSquaresBack)
{
    const complexd samples[] = {complexd(2.0, 3.0), complexd(2.0, -3.0), complexd(-2.0, 3.0), complexd(-2.0, -3.0)};
    for (const complexd& z : samples)
    {
        const complexd root = castle::math::sqrt(z);
        EXPECT_GE(root.real(), 0.0);
        EXPECT_TRUE(castle::math::near_equal(root * root, z, 1.0e-12));
    }
}

TEST(MathComplex, PowPositiveAndZeroExponent)
{
    const complexd i_unit(0.0, 1.0);
    EXPECT_EQ(castle::math::pow(i_unit, 0), complexd(1.0, 0.0));
    EXPECT_EQ(castle::math::pow(complexd(0.0, 0.0), 0), complexd(1.0, 0.0));
    EXPECT_EQ(castle::math::pow(i_unit, 1), i_unit);
    EXPECT_EQ(castle::math::pow(i_unit, 2), complexd(-1.0, 0.0));
    EXPECT_EQ(castle::math::pow(i_unit, 3), complexd(0.0, -1.0));
    EXPECT_EQ(castle::math::pow(i_unit, 4), complexd(1.0, 0.0));

    // (1 + i)^4 = -4
    EXPECT_EQ(castle::math::pow(complexd(1.0, 1.0), 4), complexd(-4.0, 0.0));
    // (1 + 2i)^3 = 1 + 6i - 12 - 8i = -11 - 2i
    EXPECT_EQ(castle::math::pow(complexd(1.0, 2.0), 3), complexd(-11.0, -2.0));
    EXPECT_EQ(castle::math::pow(complexf(1.0F, 2.0F), 3), complexf(-11.0F, -2.0F));
    EXPECT_EQ(castle::math::pow(complexd(0.0, 0.0), 5), complexd(0.0, 0.0));
}

TEST(MathComplex, PowNegativeExponent)
{
    // i^-1 = -i
    expect_complex_near(castle::math::pow(complexd(0.0, 1.0), -1), complexd(0.0, -1.0), 1.0e-12);
    // (2i)^-2 = 1 / -4
    expect_complex_near(castle::math::pow(complexd(0.0, 2.0), -2), complexd(-0.25, 0.0), 1.0e-12);
    // (1 + i)^-2 = 1 / (2i) = -0.5i
    expect_complex_near(castle::math::pow(complexd(1.0, 1.0), -2), complexd(0.0, -0.5), 1.0e-12);
    expect_complex_near(castle::math::pow(complexf(1.0F, 1.0F), -2), complexf(0.0F, -0.5F), 1.0e-6F);
    // z^-n * z^n = 1
    const complexd z(1.5, -0.5);
    EXPECT_TRUE(castle::math::near_equal(castle::math::pow(z, -3) * castle::math::pow(z, 3), complexd(1.0, 0.0), 1.0e-12));
}

TEST(MathComplex, PowMinimumIntExponent)
{
    // INT_MIN must not overflow when negated; unit-modulus bases keep the result finite.
    expect_complex_near(castle::math::pow(complexd(1.0, 0.0), INT_MIN), complexd(1.0, 0.0), 1.0e-12);
    expect_complex_near(castle::math::pow(complexd(-1.0, 0.0), INT_MIN), complexd(1.0, 0.0), 1.0e-12);
    expect_complex_near(castle::math::pow(complexd(0.0, 1.0), INT_MIN), complexd(1.0, 0.0), 1.0e-12);
    expect_complex_near(castle::math::pow(complexd(1.0, 0.0), INT_MAX), complexd(1.0, 0.0), 1.0e-12);
}

TEST(MathComplex, NearEqualComplex)
{
    const complexd base(1.0, 1.0);
    // Identical values and tiny differences below the tolerance.
    EXPECT_TRUE(castle::math::near_equal(base, base, 0.0));
    EXPECT_TRUE(castle::math::near_equal(base, complexd(1.0, 1.0 + 1.0e-10), 1.0e-9));
    EXPECT_TRUE(castle::math::near_equal(base, complexd(1.0 + 1.0e-10, 1.0), 1.0e-9));
    // Differences above the tolerance.
    EXPECT_FALSE(castle::math::near_equal(base, complexd(1.0, 1.1), 1.0e-9));
    EXPECT_FALSE(castle::math::near_equal(base, complexd(1.1, 1.0), 1.0e-9));
    // Values near zero use an absolute tolerance (scale clamps to 1).
    EXPECT_TRUE(castle::math::near_equal(complexd(0.0, 0.0), complexd(1.0e-10, -1.0e-10), 1.0e-9));
    EXPECT_FALSE(castle::math::near_equal(complexd(0.0, 0.0), complexd(1.0e-3, 0.0), 1.0e-9));
    // Large magnitudes use a relative tolerance, either operand may dominate.
    EXPECT_TRUE(castle::math::near_equal(complexd(1.0e6, 0.0), complexd(1.0e6 + 0.5, 0.0), 1.0e-6));
    EXPECT_TRUE(castle::math::near_equal(complexd(1.0e6 + 0.5, 0.0), complexd(1.0e6, 0.0), 1.0e-6));
    EXPECT_FALSE(castle::math::near_equal(complexd(1.0e6, 0.0), complexd(1.0e6 + 5.0, 0.0), 1.0e-6));
    EXPECT_TRUE(castle::math::near_equal(complexd(1.0e6, 0.0), complexd(0.0, 0.0), 2.0));

    EXPECT_TRUE(castle::math::near_equal(complexf(1.0F, 2.0F), complexf(1.0F, 2.000001F), 1.0e-5F));
    EXPECT_FALSE(castle::math::near_equal(complexf(1.0F, 2.0F), complexf(1.0F, 2.5F), 1.0e-5F));

    // Negative tolerance never matches.
    EXPECT_FALSE(castle::math::near_equal(base, base, -1.0));

    static_assert(castle::math::near_equal(complexd(1.0, 1.0), complexd(1.0, 1.0), 0.0), "constexpr near_equal");
}

} // namespace

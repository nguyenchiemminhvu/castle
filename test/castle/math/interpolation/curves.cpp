
#include <gtest/gtest.h>

#include "castle/math/interpolation/curves.hpp"

namespace
{

template <typename T>
void check_bilinear(T tol)
{
    EXPECT_NEAR(castle::math::bilinear<T>(1, 2, 3, 4, 0, 0), 1, tol);
    EXPECT_NEAR(castle::math::bilinear<T>(1, 2, 3, 4, 1, 0), 2, tol);
    EXPECT_NEAR(castle::math::bilinear<T>(1, 2, 3, 4, 0, 1), 3, tol);
    EXPECT_NEAR(castle::math::bilinear<T>(1, 2, 3, 4, 1, 1), 4, tol);
    EXPECT_NEAR(castle::math::bilinear<T>(1, 2, 3, 4, static_cast<T>(0.5), static_cast<T>(0.5)), static_cast<T>(2.5), tol);
    EXPECT_NEAR(castle::math::bilinear<T>(1, 2, 3, 4, static_cast<T>(0.25), static_cast<T>(0.75)), static_cast<T>(2.75), tol);
    // Extrapolation outside the unit cell.
    EXPECT_NEAR(castle::math::bilinear<T>(0, 1, 0, 1, 2, static_cast<T>(0.5)), 2, tol);
    EXPECT_NEAR(castle::math::bilinear<T>(0, 0, 1, 1, static_cast<T>(0.5), -1), -1, tol);
}

template <typename T>
void check_hermite(T tol)
{
    // End values and end tangents.
    EXPECT_NEAR(castle::math::cubic_hermite<T>(1, 5, 4, -3, 0), 1, tol);
    EXPECT_NEAR(castle::math::cubic_hermite<T>(1, 5, 4, -3, 1), 4, tol);
    EXPECT_NEAR(castle::math::cubic_hermite_derivative<T>(1, 5, 4, -3, 0), 5, tol);
    EXPECT_NEAR(castle::math::cubic_hermite_derivative<T>(1, 5, 4, -3, 1), -3, tol);

    // Straight line: tangent equals the secant.
    EXPECT_NEAR(castle::math::cubic_hermite<T>(0, 3, 3, 3, static_cast<T>(0.5)), static_cast<T>(1.5), tol);
    EXPECT_NEAR(castle::math::cubic_hermite_derivative<T>(0, 3, 3, 3, static_cast<T>(0.3)), 3, tol);

    // Reproduces y = t^3: p0=0, m0=0, p1=1, m1=3.
    for (int i = -2; i <= 6; ++i)
    {
        const T t = static_cast<T>(i) * static_cast<T>(0.25);
        EXPECT_NEAR(castle::math::cubic_hermite<T>(0, 0, 1, 3, t), t * t * t, tol);
        EXPECT_NEAR(castle::math::cubic_hermite_derivative<T>(0, 0, 1, 3, t), 3 * t * t, tol);
    }
}

template <typename T>
void check_derivative_matches_difference(T h, T tol)
{
    for (int i = 1; i < 10; ++i)
    {
        const T t = static_cast<T>(i) * static_cast<T>(0.1);
        const T fd = (castle::math::cubic_hermite<T>(1, 5, 4, -3, t + h)
                      - castle::math::cubic_hermite<T>(1, 5, 4, -3, t - h)) / (2 * h);
        EXPECT_NEAR(castle::math::cubic_hermite_derivative<T>(1, 5, 4, -3, t), fd, tol);
    }
}

template <typename T>
void check_catmull_rom(T tol)
{
    EXPECT_NEAR(castle::math::catmull_rom<T>(0, 1, 2, 3, 0), 1, tol);
    EXPECT_NEAR(castle::math::catmull_rom<T>(0, 1, 2, 3, 1), 2, tol);
    EXPECT_NEAR(castle::math::catmull_rom<T>(0, 1, 2, 3, static_cast<T>(0.5)), static_cast<T>(1.5), tol);
    EXPECT_NEAR(castle::math::catmull_rom<T>(0, 1, 2, 3, static_cast<T>(0.25)), static_cast<T>(1.25), tol);
    // Matches the equivalent Hermite segment.
    EXPECT_NEAR(castle::math::catmull_rom<T>(4, 1, 2, 7, static_cast<T>(0.4)),
                castle::math::cubic_hermite<T>(1, static_cast<T>(-1), 2, static_cast<T>(3), static_cast<T>(0.4)), tol);
    // Overshoot on a peak.
    EXPECT_GT(castle::math::catmull_rom<T>(0, 1, 1, 0, static_cast<T>(0.5)), 1);
    // Extrapolation.
    EXPECT_NEAR(castle::math::catmull_rom<T>(0, 1, 2, 3, 2), 3, tol);
}

template <typename T>
void check_bezier(T tol)
{
    EXPECT_NEAR(castle::math::cubic_bezier<T>(2, 5, 7, 9, 0), 2, tol);
    EXPECT_NEAR(castle::math::cubic_bezier<T>(2, 5, 7, 9, 1), 9, tol);
    EXPECT_NEAR(castle::math::cubic_bezier<T>(0, 0, 1, 1, static_cast<T>(0.5)), static_cast<T>(0.5), tol);
    // Collinear equally spaced handles give the identity curve.
    for (int i = -1; i <= 5; ++i)
    {
        const T t = static_cast<T>(i) * static_cast<T>(0.25);
        EXPECT_NEAR(castle::math::cubic_bezier<T>(0, 1, 2, 3, t), 3 * t, tol);
    }
    // Hand-computed: u=0.5 -> (p0 + 3 p1 + 3 p2 + p3) / 8.
    EXPECT_NEAR(castle::math::cubic_bezier<T>(0, 4, 8, 4, static_cast<T>(0.5)), static_cast<T>(5), tol);
}

TEST(MathCurves, BilinearFloat)
{
    check_bilinear<float>(1e-5F);
}

TEST(MathCurves, BilinearDouble)
{
    check_bilinear<double>(1e-12);
    static_assert(castle::math::bilinear(0.0, 1.0, 2.0, 3.0, 0.5, 0.5) == 1.5, "bilinear");
}

TEST(MathCurves, CubicHermiteFloat)
{
    check_hermite<float>(1e-5F);
}

TEST(MathCurves, CubicHermiteDouble)
{
    check_hermite<double>(1e-12);
    static_assert(castle::math::cubic_hermite(0.0, 0.0, 1.0, 0.0, 0.5) == 0.5, "cubic_hermite");
}

TEST(MathCurves, CubicHermiteDerivative)
{
    check_derivative_matches_difference<double>(1e-6, 1e-6);
    check_derivative_matches_difference<float>(1e-2F, 2e-2F);
    static_assert(castle::math::cubic_hermite_derivative(0.0, 1.0, 1.0, 1.0, 0.0) == 1.0, "cubic_hermite_derivative");
}

TEST(MathCurves, CatmullRomFloat)
{
    check_catmull_rom<float>(1e-5F);
}

TEST(MathCurves, CatmullRomDouble)
{
    check_catmull_rom<double>(1e-12);
    static_assert(castle::math::catmull_rom(0.0, 1.0, 2.0, 3.0, 0.5) == 1.5, "catmull_rom");
}

TEST(MathCurves, CubicBezierFloat)
{
    check_bezier<float>(1e-5F);
}

TEST(MathCurves, CubicBezierDouble)
{
    check_bezier<double>(1e-12);
    static_assert(castle::math::cubic_bezier(0.0, 0.0, 1.0, 1.0, 0.5) == 0.5, "cubic_bezier");
}

} // namespace

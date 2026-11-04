
#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <limits>

#include "castle/math/interpolation/cubic_spline.hpp"

namespace
{

using castle::size_type;
using castle::status;
using castle::math::cubic_spline;
using castle::math::extrapolation;
using castle::math::spline_kind;

template <typename T>
T tolerance()
{
    return std::numeric_limits<T>::epsilon() * static_cast<T>(2000);
}

const spline_kind all_kinds[] = {spline_kind::natural, spline_kind::clamped, spline_kind::monotone};

// ---------------------------------------------------------------------------
// Construction, capacity and build() status codes
// ---------------------------------------------------------------------------

template <typename T>
void check_empty_spline()
{
    cubic_spline<T, 8U> spline;
    EXPECT_FALSE(spline.valid());
    EXPECT_EQ(spline.size(), 0U);
    EXPECT_EQ((cubic_spline<T, 8U>::capacity()), 8U);
    EXPECT_EQ((cubic_spline<T, 2U>::capacity()), 2U);
    EXPECT_EQ(spline.capacity(), 8U);
}

template <typename T>
void check_build_failures()
{
    const T xs[] = {0, 1, 2, 3, 4};
    const T ys[] = {0, 1, 4, 9, 16};
    const T flat_x[] = {0, 1, 1, 3, 4};
    const T decreasing_x[] = {0, 2, 1, 3, 4};
    const T nan_x[] = {0, 1, std::numeric_limits<T>::quiet_NaN(), 3, 4};

    for (std::size_t k = 0U; k < 3U; ++k)
    {
        const spline_kind kind = all_kinds[k];
        cubic_spline<T, 4U> spline;

        // Null pointers.
        EXPECT_EQ(spline.build(nullptr, ys, 4U, kind), status::invalid_argument);
        EXPECT_FALSE(spline.valid());
        EXPECT_EQ(spline.build(xs, nullptr, 4U, kind), status::invalid_argument);
        EXPECT_FALSE(spline.valid());
        EXPECT_EQ(spline.build(nullptr, nullptr, 4U, kind), status::invalid_argument);
        EXPECT_FALSE(spline.valid());

        // Too few knots.
        EXPECT_EQ(spline.build(xs, ys, 0U, kind), status::invalid_argument);
        EXPECT_EQ(spline.build(xs, ys, 1U, kind), status::invalid_argument);
        EXPECT_FALSE(spline.valid());
        EXPECT_EQ(spline.size(), 0U);

        // Non-increasing and NaN abscissas.
        EXPECT_EQ(spline.build(flat_x, ys, 4U, kind), status::invalid_argument);
        EXPECT_EQ(spline.build(decreasing_x, ys, 4U, kind), status::invalid_argument);
        EXPECT_EQ(spline.build(nan_x, ys, 4U, kind), status::invalid_argument);
        EXPECT_FALSE(spline.valid());
        // The offending knots lie beyond `count`, so the prefix is fine.
        EXPECT_EQ(spline.build(flat_x, ys, 2U, kind), status::ok);

        // Capacity exceeded.
        EXPECT_EQ(spline.build(xs, ys, 5U, kind), status::full);
        EXPECT_FALSE(spline.valid());
        EXPECT_EQ(spline.size(), 0U);
        // Validation precedes the capacity check.
        EXPECT_EQ(spline.build(flat_x, ys, 5U, kind), status::invalid_argument);
        EXPECT_EQ(spline.build(nullptr, ys, 5U, kind), status::invalid_argument);

        // Exactly Capacity knots fit.
        EXPECT_EQ(spline.build(xs, ys, 4U, kind), status::ok);
        EXPECT_TRUE(spline.valid());
        EXPECT_EQ(spline.size(), 4U);
    }
}

template <typename T>
void check_failed_build_clears_previous_fit()
{
    const T xs[] = {0, 1, 2, 3};
    const T ys[] = {0, 1, 4, 9};
    const T bad_x[] = {0, 0, 1, 2};
    const T pad[] = {0, 1, 2, 3, 4, 5};

    for (std::size_t k = 0U; k < 3U; ++k)
    {
        const spline_kind kind = all_kinds[k];
        cubic_spline<T, 4U> spline;
        ASSERT_EQ(spline.build(xs, ys, 4U, kind), status::ok);
        ASSERT_TRUE(spline.valid());

        EXPECT_EQ(spline.build(bad_x, ys, 4U, kind), status::invalid_argument);
        EXPECT_FALSE(spline.valid());
        EXPECT_EQ(spline.size(), 0U);

        // Rebuild after failure.
        EXPECT_EQ(spline.build(xs, ys, 4U, kind), status::ok);
        EXPECT_TRUE(spline.valid());
        EXPECT_EQ(spline.size(), 4U);

        EXPECT_EQ(spline.build(pad, pad, 6U, kind), status::full);
        EXPECT_FALSE(spline.valid());

        // Rebuild with a different, shorter table replaces the old one.
        EXPECT_EQ(spline.build(xs, ys, 2U, kind), status::ok);
        EXPECT_EQ(spline.size(), 2U);
        EXPECT_NEAR(spline.x_min(), 0, tolerance<T>());
        EXPECT_NEAR(spline.x_max(), 1, tolerance<T>());
    }
}

// ---------------------------------------------------------------------------
// Knot reproduction, ranges and operator()
// ---------------------------------------------------------------------------

template <typename T>
void check_knots_reproduced()
{
    const T xs[] = {-2, -1, -0.25F, 0.5F, 2, 3.5F, 7};
    const T ys[] = {4, 1, -1, 0.5F, 3, -2, -1.5F};

    for (size_type count = 2U; count <= 7U; ++count)
    {
        for (std::size_t k = 0U; k < 3U; ++k)
        {
            cubic_spline<T, 8U> spline;
            ASSERT_EQ(spline.build(xs, ys, count, all_kinds[k], static_cast<T>(1), static_cast<T>(-1)), status::ok);
            ASSERT_TRUE(spline.valid());
            EXPECT_EQ(spline.size(), count);
            EXPECT_NEAR(spline.x_min(), xs[0], tolerance<T>());
            EXPECT_NEAR(spline.x_max(), xs[count - 1U], tolerance<T>());
            for (size_type i = 0U; i < count; ++i)
            {
                EXPECT_NEAR(spline.evaluate(xs[i]), ys[i], tolerance<T>());
                EXPECT_NEAR(spline.evaluate(xs[i], extrapolation::linear), ys[i], tolerance<T>());
                EXPECT_NEAR(spline(xs[i]), ys[i], tolerance<T>());
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Clamped spline reproduces a cubic polynomial
// ---------------------------------------------------------------------------

template <typename T>
T poly(T x)
{
    return x * x * x - static_cast<T>(2) * x * x + x + static_cast<T>(1);
}

template <typename T>
T poly_slope(T x)
{
    return static_cast<T>(3) * x * x - static_cast<T>(4) * x + static_cast<T>(1);
}

template <typename T>
void check_clamped_reproduces_cubic(T tol)
{
    const T xs[] = {-1, -0.5F, 0.25F, 1, 1.75F, 3};
    T ys[6];
    for (size_type i = 0U; i < 6U; ++i)
    {
        ys[i] = poly(xs[i]);
    }

    cubic_spline<T, 6U> spline;
    ASSERT_EQ(spline.build(xs, ys, 6U, spline_kind::clamped, poly_slope<T>(-1), poly_slope<T>(3)), status::ok);

    for (int i = 0; i <= 80; ++i)
    {
        const T x = static_cast<T>(-1) + static_cast<T>(i) * static_cast<T>(0.05);
        EXPECT_NEAR(spline.evaluate(x), poly(x), tol) << "x = " << x;
        EXPECT_NEAR(spline.derivative(x), poly_slope(x), tol) << "x = " << x;
    }

    // Linear extrapolation follows the clamped end tangents.
    EXPECT_NEAR(spline.evaluate(-2, extrapolation::linear), poly(static_cast<T>(-1)) - poly_slope(static_cast<T>(-1)), tol);
    EXPECT_NEAR(spline.evaluate(4, extrapolation::linear), poly(static_cast<T>(3)) + poly_slope(static_cast<T>(3)), tol);
}

template <typename T>
void check_clamped_end_slopes_honoured()
{
    const T xs[] = {0, 1, 2, 3};
    const T ys[] = {0, 1, 0, 1};

    for (size_type count = 2U; count <= 4U; ++count)
    {
        cubic_spline<T, 4U> spline;
        ASSERT_EQ(spline.build(xs, ys, count, spline_kind::clamped, static_cast<T>(2.5), static_cast<T>(-1.5)), status::ok);
        EXPECT_NEAR(spline.derivative(xs[0]), static_cast<T>(2.5), tolerance<T>());
        EXPECT_NEAR(spline.derivative(xs[count - 1U]), static_cast<T>(-1.5), tolerance<T>());
        EXPECT_NEAR(spline.derivative(xs[0], extrapolation::linear), static_cast<T>(2.5), tolerance<T>());
        EXPECT_NEAR(spline.derivative(xs[count - 1U] + 1, extrapolation::linear), static_cast<T>(-1.5), tolerance<T>());
    }
}

// ---------------------------------------------------------------------------
// Natural spline
// ---------------------------------------------------------------------------

// Second derivative at the left end via a forward difference of derivative().
template <typename T, size_type N>
T left_curvature(const cubic_spline<T, N>& spline, T h)
{
    return (spline.derivative(spline.x_min() + h) - spline.derivative(spline.x_min())) / h;
}

template <typename T, size_type N>
T right_curvature(const cubic_spline<T, N>& spline, T h)
{
    return (spline.derivative(spline.x_max()) - spline.derivative(spline.x_max() - h)) / h;
}

template <typename T>
void check_natural_end_curvature(T h, T tol)
{
    const T xs[] = {0, 1, 2.5F, 3, 5};
    const T ys[] = {0, 1, 6.25F, 9, 25}; // y = x^2, which has curvature 2 at both ends

    for (size_type count = 2U; count <= 5U; ++count)
    {
        cubic_spline<T, 5U> spline;
        ASSERT_EQ(spline.build(xs, ys, count, spline_kind::natural), status::ok);
        EXPECT_NEAR(left_curvature(spline, h), 0, tol) << "count = " << count;
        EXPECT_NEAR(right_curvature(spline, h), 0, tol) << "count = " << count;
    }

    // The slope arguments are ignored by the natural kind.
    cubic_spline<T, 5U> plain;
    cubic_spline<T, 5U> with_slopes;
    ASSERT_EQ(plain.build(xs, ys, 5U, spline_kind::natural), status::ok);
    ASSERT_EQ(with_slopes.build(xs, ys, 5U, spline_kind::natural, static_cast<T>(100), static_cast<T>(-100)), status::ok);
    for (int i = 0; i <= 20; ++i)
    {
        const T x = static_cast<T>(i) * static_cast<T>(0.25);
        EXPECT_EQ(plain.evaluate(x), with_slopes.evaluate(x));
        EXPECT_EQ(plain.derivative(x), with_slopes.derivative(x));
    }
}

template <typename T>
void check_natural_linear_data_is_linear(T tol)
{
    const T xs[] = {0, 0.5F, 2, 3, 6};
    T ys[5];
    for (size_type i = 0U; i < 5U; ++i)
    {
        ys[i] = static_cast<T>(3) * xs[i] - static_cast<T>(2);
    }
    for (std::size_t k = 0U; k < 3U; ++k)
    {
        if (all_kinds[k] == spline_kind::clamped)
        {
            continue;
        }
        cubic_spline<T, 5U> spline;
        ASSERT_EQ(spline.build(xs, ys, 5U, all_kinds[k]), status::ok);
        for (int i = 0; i <= 30; ++i)
        {
            const T x = static_cast<T>(i) * static_cast<T>(0.2);
            EXPECT_NEAR(spline.evaluate(x), static_cast<T>(3) * x - static_cast<T>(2), tol);
            EXPECT_NEAR(spline.derivative(x), 3, tol);
        }
    }
}

// C1 (all kinds) and C2 (natural/clamped) continuity across interior knots.
void check_continuity()
{
    const double xs[] = {0.0, 0.7, 1.5, 2.0, 3.5, 4.0};
    const double ys[] = {1.0, -0.5, 2.0, 2.5, -1.0, 0.0};
    const double h = 1e-6;

    for (std::size_t k = 0U; k < 3U; ++k)
    {
        cubic_spline<double, 6U> spline;
        ASSERT_EQ(spline.build(xs, ys, 6U, all_kinds[k], 0.5, -0.5), status::ok);
        for (size_type i = 1U; i + 1U < 6U; ++i)
        {
            const double left = spline.derivative(xs[i] - h);
            const double right = spline.derivative(xs[i] + h);
            EXPECT_NEAR(left, right, 1e-4) << "kind " << k << " knot " << i;
            if (all_kinds[k] != spline_kind::monotone)
            {
                const double curvature_left = (spline.derivative(xs[i]) - spline.derivative(xs[i] - h)) / h;
                const double curvature_right = (spline.derivative(xs[i] + h) - spline.derivative(xs[i])) / h;
                EXPECT_NEAR(curvature_left, curvature_right, 1e-3) << "kind " << k << " knot " << i;
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Monotone (PCHIP) spline
// ---------------------------------------------------------------------------

template <typename T, size_type N>
void check_monotone_nondecreasing(const cubic_spline<T, N>& spline, T lo, T hi, T tol)
{
    T prev = spline.evaluate(spline.x_min());
    for (int i = 1; i <= 2000; ++i)
    {
        const T x = spline.x_min() + (spline.x_max() - spline.x_min()) * static_cast<T>(i) / static_cast<T>(2000);
        const T y = spline.evaluate(x);
        EXPECT_GE(y, lo - tol) << "x = " << x;
        EXPECT_LE(y, hi + tol) << "x = " << x;
        EXPECT_GE(y, prev - tol) << "x = " << x;
        EXPECT_GE(spline.derivative(x), -tol) << "x = " << x;
        prev = y;
    }
}

template <typename T>
void check_monotone_data(T tol)
{
    const T xs[] = {0, 1, 2, 4, 5, 8, 9, 10};
    const T ys[] = {0, 0.1F, 0.1F, 0.2F, 3, 3.05F, 9, 9};

    cubic_spline<T, 8U> spline;
    ASSERT_EQ(spline.build(xs, ys, 8U, spline_kind::monotone), status::ok);
    check_monotone_nondecreasing(spline, static_cast<T>(0), static_cast<T>(9), tol);

    // Flat runs stay exactly flat (zero tangents at both ends).
    for (int i = 0; i <= 20; ++i)
    {
        const T flat1 = static_cast<T>(1) + static_cast<T>(i) * static_cast<T>(0.05);
        EXPECT_NEAR(spline.evaluate(flat1), 0.1F, tol);
        EXPECT_NEAR(spline.derivative(flat1), 0, tol);
        const T flat2 = static_cast<T>(9) + static_cast<T>(i) * static_cast<T>(0.05);
        EXPECT_NEAR(spline.evaluate(flat2), 9, tol);
        EXPECT_NEAR(spline.derivative(flat2), 0, tol);
    }

    // Decreasing data (mirror) is shape preserving as well.
    T neg[8];
    for (size_type i = 0U; i < 8U; ++i)
    {
        neg[i] = -ys[i];
    }
    cubic_spline<T, 8U> mirrored;
    ASSERT_EQ(mirrored.build(xs, neg, 8U, spline_kind::monotone), status::ok);
    for (int i = 0; i <= 1000; ++i)
    {
        const T x = static_cast<T>(i) * static_cast<T>(0.01);
        EXPECT_NEAR(mirrored.evaluate(x), -spline.evaluate(x), tol);
        EXPECT_NEAR(mirrored.derivative(x), -spline.derivative(x), tol);
        EXPECT_LE(mirrored.derivative(x), tol);
    }
}

template <typename T>
void check_monotone_local_extrema(T tol)
{
    // Local maximum then minimum: tangents there must be zero.
    const T xs[] = {0, 1, 2, 3, 4};
    const T ys[] = {0, 2, 1, 3, 3.5F};
    cubic_spline<T, 5U> spline;
    ASSERT_EQ(spline.build(xs, ys, 5U, spline_kind::monotone), status::ok);
    EXPECT_NEAR(spline.derivative(1), 0, tol);
    EXPECT_NEAR(spline.derivative(2), 0, tol);
    for (int i = 0; i <= 100; ++i)
    {
        const T x = static_cast<T>(i) * static_cast<T>(0.01);
        EXPECT_GE(spline.evaluate(x), -tol);
        EXPECT_LE(spline.evaluate(x), 2 + tol);
        EXPECT_GE(spline.evaluate(1 + x), 1 - tol);
        EXPECT_LE(spline.evaluate(1 + x), 2 + tol);
        EXPECT_GE(spline.evaluate(2 + x), 1 - tol);
        EXPECT_LE(spline.evaluate(2 + x), 3 + tol);
    }
}

// Three-point end slope rules (monotone_end_slope branches), uniform spacing h = 1.
// slope = (3 * d0 - d1) / 2, forced to 0 when its sign differs from d0 and capped at 3 * d0 when the
// secants have opposite signs and |slope| > 3 * |d0|.
template <typename T>
void check_monotone_end_slopes(T tol)
{
    const T xs[] = {0, 1, 2};
    cubic_spline<T, 3U> spline;

    // d0 = 1, d1 = 4: raw slope -0.5 has the wrong sign -> 0. Interior harmonic mean 1.6.
    {
        const T ys[] = {0, 1, 5};
        ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::monotone), status::ok);
        EXPECT_NEAR(spline.derivative(0), 0, tol);
        EXPECT_NEAR(spline.derivative(0, extrapolation::linear), 0, tol);
        EXPECT_NEAR(spline.derivative(-1, extrapolation::linear), 0, tol);
        EXPECT_NEAR(spline.derivative(static_cast<T>(1) - static_cast<T>(1e-3)), static_cast<T>(1.6), static_cast<T>(2e-2));
        // Last end: d0 = 4, d1 = 1 (same sign) -> raw slope 5.5 kept.
        EXPECT_NEAR(spline.derivative(3, extrapolation::linear), static_cast<T>(5.5), tol);
        EXPECT_NEAR(spline.evaluate(3, extrapolation::linear), 5 + static_cast<T>(5.5), tol);
        EXPECT_NEAR(spline.derivative(2), static_cast<T>(5.5), tol);
    }

    // d0 = 1, d1 = -4 (opposite signs): raw slope 3.5 > 3 * d0 -> capped to 3.
    {
        const T ys[] = {0, 1, -3};
        ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::monotone), status::ok);
        EXPECT_NEAR(spline.derivative(0), 3, tol);
        EXPECT_NEAR(spline.derivative(-1, extrapolation::linear), 3, tol);
        EXPECT_NEAR(spline.derivative(1), 0, tol); // sign change in secants -> flat tangent
        // Last end: d0 = -4, d1 = 1 (opposite signs) -> raw -6.5, |-6.5| <= 12 so kept.
        EXPECT_NEAR(spline.derivative(2), static_cast<T>(-6.5), tol);
        EXPECT_NEAR(spline.derivative(3, extrapolation::linear), static_cast<T>(-6.5), tol);
    }

    // Mirror of the capped case at the last end: d0 = 1 (last), d1 = -4.
    {
        const T ys[] = {0, -4, -3};
        ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::monotone), status::ok);
        EXPECT_NEAR(spline.derivative(2), 3, tol);
        EXPECT_NEAR(spline.derivative(3, extrapolation::linear), 3, tol);
        EXPECT_NEAR(spline.derivative(1), 0, tol);
        EXPECT_NEAR(spline.derivative(0), static_cast<T>(-6.5), tol);
    }

    // Mirror of the sign-flip case at the last end: d0 = 1 (last), d1 = 4 -> 0.
    {
        const T ys[] = {0, 4, 5};
        ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::monotone), status::ok);
        EXPECT_NEAR(spline.derivative(2), 0, tol);
        EXPECT_NEAR(spline.derivative(3, extrapolation::linear), 0, tol);
        EXPECT_NEAR(spline.derivative(0), static_cast<T>(5.5), tol);
    }

    // Opposite signs but within the limit: d0 = 1, d1 = -1 -> raw slope 2 kept.
    {
        const T ys[] = {0, 1, 0};
        ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::monotone), status::ok);
        EXPECT_NEAR(spline.derivative(0), 2, tol);
        EXPECT_NEAR(spline.derivative(2), -2, tol);
        EXPECT_NEAR(spline.derivative(1), 0, tol);
    }

    // Flat first secant: d0 = 0, d1 = 1 -> raw slope -0.5 differs from sign(0) -> 0.
    {
        const T ys[] = {1, 1, 2};
        ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::monotone), status::ok);
        EXPECT_NEAR(spline.derivative(0), 0, tol);
        EXPECT_NEAR(spline.derivative(1), 0, tol);
        EXPECT_NEAR(spline.derivative(2), static_cast<T>(1.5), tol);
        for (int i = 0; i <= 10; ++i)
        {
            EXPECT_NEAR(spline.evaluate(static_cast<T>(i) * static_cast<T>(0.1)), 1, tol);
        }
    }

    // Completely flat data: both secants zero -> raw slope 0, equal signs -> returned as-is.
    {
        const T ys[] = {2, 2, 2};
        ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::monotone), status::ok);
        for (int i = 0; i <= 20; ++i)
        {
            EXPECT_NEAR(spline.evaluate(static_cast<T>(i) * static_cast<T>(0.1)), 2, tol);
            EXPECT_NEAR(spline.derivative(static_cast<T>(i) * static_cast<T>(0.1)), 0, tol);
        }
    }

    // Uniform ramp: raw slope equals the secant and is kept; interior is the harmonic mean of equal secants.
    {
        const T ys[] = {0, 2, 4};
        ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::monotone), status::ok);
        for (int i = 0; i <= 20; ++i)
        {
            const T x = static_cast<T>(i) * static_cast<T>(0.1);
            EXPECT_NEAR(spline.evaluate(x), 2 * x, tol);
            EXPECT_NEAR(spline.derivative(x), 2, tol);
        }
    }
}

// Uneven widths exercise the width-weighted end formula and the weighted harmonic mean.
template <typename T>
void check_monotone_uneven_widths(T tol)
{
    // widths 1 and 3, secants 2 and 1.
    const T xs[] = {0, 1, 4};
    const T ys[] = {0, 2, 5};
    cubic_spline<T, 3U> spline;
    ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::monotone), status::ok);

    // Interior: w1 = 2 * h_next + h_prev = 7, w2 = h_next + 2 * h_prev = 5, m = 12 / (7/2 + 5/1).
    const T interior = static_cast<T>(12.0 / (7.0 / 2.0 + 5.0 / 1.0));
    EXPECT_NEAR(spline.derivative(1), interior, tol);
    // First end: ((2 * 1 + 3) * 2 - 1 * 1) / 4 = 9/4.
    EXPECT_NEAR(spline.derivative(0), static_cast<T>(2.25), tol);
    // Last end: h0 = 3, h1 = 1, d0 = 1, d1 = 2: ((6 + 1) * 1 - 3 * 2) / 4 = 1/4.
    EXPECT_NEAR(spline.derivative(4), static_cast<T>(0.25), tol);
    check_monotone_nondecreasing(spline, static_cast<T>(0), static_cast<T>(5), tol);
}

// ---------------------------------------------------------------------------
// Two- and three-knot splines for every kind
// ---------------------------------------------------------------------------

template <typename T>
void check_two_knots(T tol)
{
    const T xs[] = {1, 3};
    const T ys[] = {2, 6};

    cubic_spline<T, 2U> spline;
    for (std::size_t k = 0U; k < 3U; ++k)
    {
        ASSERT_EQ(spline.build(xs, ys, 2U, all_kinds[k], 5, 7), status::ok);
        EXPECT_EQ(spline.size(), 2U);
        EXPECT_NEAR(spline.evaluate(1), 2, tol);
        EXPECT_NEAR(spline.evaluate(3), 6, tol);
        if (all_kinds[k] == spline_kind::clamped)
        {
            EXPECT_NEAR(spline.derivative(1), 5, tol);
            EXPECT_NEAR(spline.derivative(3), 7, tol);
            // Midpoint of the Hermite cubic through (1,2),(3,6): tangents are width (2) * slope.
            EXPECT_NEAR(spline.evaluate(2), castle::math::cubic_hermite<T>(2, 10, 6, 14, static_cast<T>(0.5)), tol);
            EXPECT_NEAR(spline.derivative(2), castle::math::cubic_hermite_derivative<T>(2, 10, 6, 14, static_cast<T>(0.5)) / 2, tol);
        }
        else
        {
            // Natural and monotone with two knots are the straight line y = 2x.
            for (int i = 0; i <= 20; ++i)
            {
                const T x = static_cast<T>(1) + static_cast<T>(i) * static_cast<T>(0.1);
                EXPECT_NEAR(spline.evaluate(x), 2 * x, tol);
                EXPECT_NEAR(spline.derivative(x), 2, tol);
            }
        }
    }
}

template <typename T>
void check_three_knots(T tol)
{
    const T xs[] = {0, 1, 3};
    const T ys[] = {1, 3, 2};

    cubic_spline<T, 3U> spline;

    // Natural: knots, a continuous slope across the middle knot and zero end curvature.
    ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::natural), status::ok);
    for (size_type i = 0U; i < 3U; ++i)
    {
        EXPECT_NEAR(spline.evaluate(xs[i]), ys[i], tol);
    }
    EXPECT_NEAR(spline.derivative(static_cast<T>(1) - static_cast<T>(1e-3)),
                spline.derivative(static_cast<T>(1) + static_cast<T>(1e-3)), static_cast<T>(2e-2));

    // Clamped: the supplied end slopes hold and the middle knot is reproduced.
    ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::clamped, -1, 2), status::ok);
    EXPECT_NEAR(spline.derivative(0), -1, tol);
    EXPECT_NEAR(spline.derivative(3), 2, tol);
    EXPECT_NEAR(spline.evaluate(1), 3, tol);

    // Monotone with a local maximum in the middle: flat tangent there.
    ASSERT_EQ(spline.build(xs, ys, 3U, spline_kind::monotone), status::ok);
    EXPECT_NEAR(spline.derivative(1), 0, tol);
    EXPECT_NEAR(spline.evaluate(1), 3, tol);
    for (int i = 0; i <= 30; ++i)
    {
        const T x = static_cast<T>(i) * static_cast<T>(0.1);
        EXPECT_LE(spline.evaluate(x), 3 + tol);
        EXPECT_GE(spline.evaluate(x), 1 - tol);
    }
}

// ---------------------------------------------------------------------------
// Extrapolation modes and bounds
// ---------------------------------------------------------------------------

template <typename T>
void check_extrapolation(T tol)
{
    const T xs[] = {1, 2, 4, 5};
    const T ys[] = {3, 1, 2, 6};

    for (std::size_t k = 0U; k < 3U; ++k)
    {
        cubic_spline<T, 4U> spline;
        ASSERT_EQ(spline.build(xs, ys, 4U, all_kinds[k], static_cast<T>(-1.5), static_cast<T>(2.5)), status::ok);

        const T first_slope = spline.derivative(xs[0]);
        const T last_slope = spline.derivative(xs[3]);

        // Below: clamp holds ys[0] with zero slope, linear follows the tangent.
        EXPECT_NEAR(spline.evaluate(-3), 3, tol);
        EXPECT_NEAR(spline.evaluate(-3, extrapolation::clamp), 3, tol);
        EXPECT_NEAR(spline.evaluate(-3, extrapolation::linear), 3 + first_slope * (-3 - 1), tol * 10);
        EXPECT_EQ(spline.derivative(-3), static_cast<T>(0));
        EXPECT_EQ(spline.derivative(-3, extrapolation::clamp), static_cast<T>(0));
        EXPECT_NEAR(spline.derivative(-3, extrapolation::linear), first_slope, tol);

        // At the knots.
        EXPECT_NEAR(spline.evaluate(1, extrapolation::clamp), 3, tol);
        EXPECT_NEAR(spline.evaluate(1, extrapolation::linear), 3, tol);
        EXPECT_NEAR(spline.evaluate(5, extrapolation::clamp), 6, tol);
        EXPECT_NEAR(spline.evaluate(5, extrapolation::linear), 6, tol);

        // Inside: both modes agree.
        for (int i = 0; i <= 40; ++i)
        {
            const T x = static_cast<T>(1) + static_cast<T>(i) * static_cast<T>(0.1);
            EXPECT_EQ(spline.evaluate(x, extrapolation::clamp), spline.evaluate(x, extrapolation::linear));
            EXPECT_EQ(spline.derivative(x, extrapolation::clamp), spline.derivative(x, extrapolation::linear));
        }

        // Above.
        EXPECT_NEAR(spline.evaluate(9), 6, tol);
        EXPECT_NEAR(spline.evaluate(9, extrapolation::clamp), 6, tol);
        EXPECT_NEAR(spline.evaluate(9, extrapolation::linear), 6 + last_slope * 4, tol * 10);
        EXPECT_EQ(spline.derivative(9), static_cast<T>(0));
        EXPECT_EQ(spline.derivative(9, extrapolation::clamp), static_cast<T>(0));
        EXPECT_NEAR(spline.derivative(9, extrapolation::linear), last_slope, tol);

        // The linear extension is C1 across both boundaries.
        const T eps = static_cast<T>(1e-3);
        EXPECT_NEAR(spline.evaluate(1 - eps, extrapolation::linear), spline.evaluate(1 + eps), 3 * eps * (std::abs(first_slope) + 1));
        EXPECT_NEAR(spline.evaluate(5 + eps, extrapolation::linear), spline.evaluate(5 - eps), 3 * eps * (std::abs(last_slope) + 1));
    }
}

TEST(MathCubicSpline, EmptyFloat)
{
    check_empty_spline<float>();
}

TEST(MathCubicSpline, EmptyDouble)
{
    check_empty_spline<double>();
}

TEST(MathCubicSpline, ConstexprDefaultConstruction)
{
    static constexpr cubic_spline<double, 4U> spline;
    static_assert(!spline.valid(), "cubic_spline");
    static_assert(spline.size() == 0U, "cubic_spline");
    static_assert(cubic_spline<float, 4U>::capacity() == 4U, "cubic_spline");
    EXPECT_FALSE(spline.valid());
}

TEST(MathCubicSpline, BuildFailureStatusFloat)
{
    check_build_failures<float>();
}

TEST(MathCubicSpline, BuildFailureStatusDouble)
{
    check_build_failures<double>();
}

TEST(MathCubicSpline, FailedBuildClearsAndRebuildsFloat)
{
    check_failed_build_clears_previous_fit<float>();
}

TEST(MathCubicSpline, FailedBuildClearsAndRebuildsDouble)
{
    check_failed_build_clears_previous_fit<double>();
}

TEST(MathCubicSpline, ReproducesKnotsFloat)
{
    check_knots_reproduced<float>();
}

TEST(MathCubicSpline, ReproducesKnotsDouble)
{
    check_knots_reproduced<double>();
}

TEST(MathCubicSpline, ClampedReproducesCubicFloat)
{
    check_clamped_reproduces_cubic<float>(2e-3F);
}

TEST(MathCubicSpline, ClampedReproducesCubicDouble)
{
    check_clamped_reproduces_cubic<double>(1e-9);
}

TEST(MathCubicSpline, ClampedEndSlopesFloat)
{
    check_clamped_end_slopes_honoured<float>();
}

TEST(MathCubicSpline, ClampedEndSlopesDouble)
{
    check_clamped_end_slopes_honoured<double>();
}

TEST(MathCubicSpline, NaturalEndCurvatureDouble)
{
    check_natural_end_curvature<double>(1e-6, 1e-4);
}

TEST(MathCubicSpline, NaturalEndCurvatureFloat)
{
    check_natural_end_curvature<float>(1e-2F, 0.1F);
}

TEST(MathCubicSpline, NaturalDiffersFromQuadraticInterior)
{
    // A natural spline cannot reproduce y = x^2 because its end curvature is forced to zero.
    const double xs[] = {0.0, 1.0, 2.0, 3.0, 4.0};
    const double ys[] = {0.0, 1.0, 4.0, 9.0, 16.0};
    cubic_spline<double, 5U> spline;
    ASSERT_EQ(spline.build(xs, ys, 5U, spline_kind::natural), status::ok);
    EXPECT_GT(std::abs(spline.evaluate(0.5) - 0.25), 1e-3);
    // A clamped spline with the exact slopes does reproduce it.
    ASSERT_EQ(spline.build(xs, ys, 5U, spline_kind::clamped, 0.0, 8.0), status::ok);
    EXPECT_NEAR(spline.evaluate(0.5), 0.25, 1e-12);
    EXPECT_NEAR(spline.evaluate(3.7), 3.7 * 3.7, 1e-12);
}

TEST(MathCubicSpline, LinearDataStaysLinearFloat)
{
    check_natural_linear_data_is_linear<float>(1e-4F);
}

TEST(MathCubicSpline, LinearDataStaysLinearDouble)
{
    check_natural_linear_data_is_linear<double>(1e-12);
}

TEST(MathCubicSpline, ContinuityAcrossKnots)
{
    check_continuity();
}

TEST(MathCubicSpline, MonotoneShapePreservingFloat)
{
    check_monotone_data<float>(1e-5F);
}

TEST(MathCubicSpline, MonotoneShapePreservingDouble)
{
    check_monotone_data<double>(1e-12);
}

TEST(MathCubicSpline, MonotoneLocalExtremaFloat)
{
    check_monotone_local_extrema<float>(1e-5F);
}

TEST(MathCubicSpline, MonotoneLocalExtremaDouble)
{
    check_monotone_local_extrema<double>(1e-12);
}

TEST(MathCubicSpline, MonotoneEndSlopeRulesFloat)
{
    check_monotone_end_slopes<float>(1e-5F);
}

TEST(MathCubicSpline, MonotoneEndSlopeRulesDouble)
{
    check_monotone_end_slopes<double>(1e-12);
}

TEST(MathCubicSpline, MonotoneUnevenWidthsFloat)
{
    check_monotone_uneven_widths<float>(1e-5F);
}

TEST(MathCubicSpline, MonotoneUnevenWidthsDouble)
{
    check_monotone_uneven_widths<double>(1e-12);
}

TEST(MathCubicSpline, TwoKnotsFloat)
{
    check_two_knots<float>(1e-5F);
}

TEST(MathCubicSpline, TwoKnotsDouble)
{
    check_two_knots<double>(1e-12);
}

TEST(MathCubicSpline, ThreeKnotsFloat)
{
    check_three_knots<float>(1e-5F);
}

TEST(MathCubicSpline, ThreeKnotsDouble)
{
    check_three_knots<double>(1e-12);
}

TEST(MathCubicSpline, ExtrapolationModesFloat)
{
    check_extrapolation<float>(1e-4F);
}

TEST(MathCubicSpline, ExtrapolationModesDouble)
{
    check_extrapolation<double>(1e-10);
}

TEST(MathCubicSpline, SplineIsCopyable)
{
    const double xs[] = {0.0, 1.0, 2.0};
    const double ys[] = {0.0, 1.0, 0.0};
    cubic_spline<double, 4U> original;
    ASSERT_EQ(original.build(xs, ys, 3U, spline_kind::natural), status::ok);
    const cubic_spline<double, 4U> copy = original;
    EXPECT_TRUE(copy.valid());
    EXPECT_EQ(copy.size(), 3U);
    for (int i = 0; i <= 20; ++i)
    {
        const double x = 0.1 * static_cast<double>(i);
        EXPECT_EQ(copy.evaluate(x), original.evaluate(x));
        EXPECT_EQ(copy.derivative(x), original.derivative(x));
    }
}

constexpr double constexpr_spline_slope(castle::math::spline_kind kind)
{
    constexpr double xs[] = {0.0, 1.0, 2.0, 4.0};
    constexpr double ys[] = {0.0, 2.0, 4.0, 5.0};
    castle::math::cubic_spline<double, 4U> spline;
    static_cast<void>(spline.build(xs, ys, 4U, kind));
    return spline.derivative(1.0);
}

TEST(MathCubicSpline, ConstexprBuildAndEvaluation)
{
    static_assert(constexpr_spline_slope(castle::math::spline_kind::monotone) > 0.0, "cubic_spline");
    static_assert(constexpr_spline_slope(castle::math::spline_kind::natural) > 0.0, "cubic_spline");
    EXPECT_GT(constexpr_spline_slope(castle::math::spline_kind::monotone), 0.0);
}

} // namespace

#include "sample_support.hpp"

#include "castle/math/interpolation/cubic_spline.hpp"
#include "castle/math/near_equal.hpp"

int main()
{
    constexpr double eps = 1.0e-9;
    using castle::math::extrapolation;
    using castle::math::spline_kind;

    const double xs[] = {0.0, 1.0, 2.0, 3.0};
    const double cubic_ys[] = {0.0, 1.0, 8.0, 27.0}; // y = x^3

    // Empty spline.
    castle::math::cubic_spline<double, 8U> spline;
    CASTLE_SAMPLE_CHECK(spline.capacity() == 8U);
    CASTLE_SAMPLE_CHECK(!spline.valid());
    CASTLE_SAMPLE_CHECK(spline.size() == 0U);

    // Natural spline reproduces its knots and exposes the range.
    CASTLE_SAMPLE_CHECK(spline.build(xs, cubic_ys, 4U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(spline.valid());
    CASTLE_SAMPLE_CHECK(spline.size() == 4U);
    CASTLE_SAMPLE_CHECK(spline.x_min() == 0.0);
    CASTLE_SAMPLE_CHECK(spline.x_max() == 3.0);
    for (castle::size_type i = 0U; i < 4U; ++i)
    {
        CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.evaluate(xs[i]), cubic_ys[i], eps));
        CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline(xs[i]), cubic_ys[i], eps));
    }

    // Clamped spline with the true end slopes reproduces the cubic exactly.
    CASTLE_SAMPLE_CHECK(spline.build(xs, cubic_ys, 4U, spline_kind::clamped, 0.0, 27.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.evaluate(1.5), 3.375, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.derivative(1.5), 6.75, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.derivative(0.0), 0.0, eps));

    // Extrapolation: clamp holds the end value, linear follows the end tangent.
    CASTLE_SAMPLE_CHECK(spline.evaluate(-1.0) == 0.0);
    CASTLE_SAMPLE_CHECK(spline.evaluate(5.0) == 27.0);
    CASTLE_SAMPLE_CHECK(spline.derivative(-1.0) == 0.0);
    CASTLE_SAMPLE_CHECK(spline.derivative(5.0) == 0.0);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.evaluate(-1.0, extrapolation::linear), 0.0, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.evaluate(4.0, extrapolation::linear), 54.0, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.derivative(-1.0, extrapolation::linear), 0.0, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.derivative(4.0, extrapolation::linear), 27.0, eps));

    // Monotone (PCHIP) spline never overshoots monotone data and stays flat on flat runs.
    const double steps[] = {0.0, 0.0, 0.0, 1.0, 1.0, 1.0};
    const double step_xs[] = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
    CASTLE_SAMPLE_CHECK(spline.build(step_xs, steps, 6U, spline_kind::monotone) == castle::status::ok);
    for (int i = 0; i <= 50; ++i)
    {
        const double y = spline.evaluate(0.1 * i);
        CASTLE_SAMPLE_CHECK((y >= -1.0e-12) && (y <= 1.0 + 1.0e-12));
    }
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.evaluate(0.5), 0.0, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.evaluate(4.5), 1.0, eps));

    // Failure statuses leave the spline empty.
    CASTLE_SAMPLE_CHECK(spline.build(xs, nullptr, 4U) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(!spline.valid());
    CASTLE_SAMPLE_CHECK(spline.build(nullptr, cubic_ys, 4U) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(spline.build(xs, cubic_ys, 1U) == castle::status::invalid_argument);
    const double unsorted[] = {0.0, 2.0, 1.0, 3.0};
    CASTLE_SAMPLE_CHECK(spline.build(unsorted, cubic_ys, 4U) == castle::status::invalid_argument);
    castle::math::cubic_spline<double, 3U> small;
    CASTLE_SAMPLE_CHECK(small.build(xs, cubic_ys, 4U) == castle::status::full);
    CASTLE_SAMPLE_CHECK(!small.valid());

    // A spline can be rebuilt after a failure.
    CASTLE_SAMPLE_CHECK(spline.build(xs, cubic_ys, 4U, spline_kind::natural) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(spline.valid());

    // Two knots give a straight line.
    const double line_xs[] = {0.0, 2.0};
    const double line_ys[] = {1.0, 5.0};
    CASTLE_SAMPLE_CHECK(spline.build(line_xs, line_ys, 2U, spline_kind::natural) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.evaluate(1.0), 3.0, eps));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(spline.derivative(1.0), 2.0, eps));
    return 0;
}

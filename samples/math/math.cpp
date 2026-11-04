#include "sample_support.hpp"

#include "castle/math/math.hpp"

int main()
{
    static_assert(castle::sqrt<81U>::value == 9U, "");

    const float epsilon = castle::meta::floating_epsilon<float>::value;

    CASTLE_SAMPLE_CHECK(castle::math::clamp(150, 0, 100) == 100);
    CASTLE_SAMPLE_CHECK(castle::math::powi(3U, 3U) == 27U);
    CASTLE_SAMPLE_CHECK(castle::math::is_equal(1.0f, 1.0f + (epsilon * 0.5f)));
    CASTLE_SAMPLE_CHECK(castle::math::is_zero(epsilon * 0.5f));

    // The umbrella also exposes complex numbers, interpolation tables and numerical methods.
    const castle::math::complex<double> rotated = castle::math::complex<double>(0.0, 1.0) * castle::math::complex<double>(0.0, 1.0);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(rotated.real(), -1.0, 1.0e-12));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(rotated.imag(), 0.0, 1.0e-12));

    const double xs[3] = {0.0, 1.0, 2.0};
    const double ys[3] = {0.0, 10.0, 20.0};
    const castle::math::linear_table<double> table(xs, ys, 3U);
    CASTLE_SAMPLE_CHECK(table.valid());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(table(1.5), 15.0, 1.0e-12));

    const auto square = [](double x) { return x * x; };
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::integrate_simpson(square, 0.0, 3.0, 100U), 9.0, 1.0e-9));

    const auto shifted = [](double x) { return (x * x) - 2.0; };
    const castle::math::numeric_result<double> root = castle::math::find_root_bisection(shifted, 0.0, 2.0, 1.0e-12);
    CASTLE_SAMPLE_CHECK(root.ok());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(root.value, 1.4142135623730951, 1.0e-9));

    return 0;
}

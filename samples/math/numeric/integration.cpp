#include "sample_support.hpp"

#include "castle/math/exponential.hpp"
#include "castle/math/near_equal.hpp"
#include "castle/math/numeric/integration.hpp"

namespace
{

double square(double x)
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

double abs_diff(double a, double b)
{
    return (a > b) ? (a - b) : (b - a);
}

} // namespace

int main()
{
    using namespace castle::math;

    const auto cube = [](double x) { return x * x * x; };
    const auto lorentz = [](double x) { return 1.0 / (1.0 + x * x); };
    const auto expo = [](double x) { return castle::math::exp(x); };
    const auto linear_f = [](float x) { return 2.0F * x + 1.0F; };

    // Trapezoid: exact for linear, converges for smooth functions; works with lambdas, function pointers, functors.
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_trapezoid([](double x) { return 2.0 * x + 1.0; }, 0.0, 3.0, 1U), 12.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_trapezoid(square, 0.0, 3.0, 1000U), 9.0) < 1e-4);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_trapezoid(cubic_functor{}, 0.0, 2.0, 2000U), 4.0) < 1e-5);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_trapezoid(square, 3.0, 0.0, 1000U), -9.0) < 1e-4);
    CASTLE_SAMPLE_CHECK(near_equal(integrate_trapezoid(linear_f, 0.0F, 3.0F, 4U), 12.0F, 1e-5F));

    // Simpson: exact for cubics; odd interval counts round up to even.
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_simpson(cube, 0.0, 2.0, 2U), 4.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_simpson(cube, 0.0, 2.0, 3U), 4.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_simpson(cube, 2.0, 0.0, 4U), -4.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_simpson(lorentz, 0.0, 1.0, 100U), 0.7853981633974483) < 1e-9);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_simpson(expo, 0.0, 1.0, 64U), 1.718281828459045) < 1e-8);
    CASTLE_SAMPLE_CHECK(near_equal(integrate_simpson([](float x) { return x * x; }, 0.0F, 3.0F, 10U), 9.0F, 1e-5F));

    // Gauss-Legendre: exact up to degree 9 per panel.
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_gauss_legendre(cube, 0.0, 2.0), 4.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_gauss_legendre([](double x) { return x * x * x * x * x * x * x * x * x; }, 0.0, 1.0), 0.1) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_gauss_legendre(lorentz, 0.0, 1.0, 4U), 0.7853981633974483) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_gauss_legendre(expo, 1.0, 0.0, 2U), -1.718281828459045) < 1e-12);
    CASTLE_SAMPLE_CHECK(near_equal(integrate_gauss_legendre([](float x) { return x * x; }, 0.0F, 3.0F, 2U), 9.0F, 1e-5F));

    // Romberg: converges on smooth integrands, reports iterations and error.
    const auto converged = integrate_romberg(cube, 0.0, 2.0, 1e-12);
    CASTLE_SAMPLE_CHECK(converged.ok());
    CASTLE_SAMPLE_CHECK(abs_diff(converged.value, 4.0) < 1e-10);
    CASTLE_SAMPLE_CHECK(converged.iterations >= 2U);
    CASTLE_SAMPLE_CHECK(converged.iterations <= 12U);
    CASTLE_SAMPLE_CHECK(converged.error <= 1e-12 * 4.0);

    const auto smooth = integrate_romberg(expo, 0.0, 1.0, 1e-10);
    CASTLE_SAMPLE_CHECK(smooth.ok());
    CASTLE_SAMPLE_CHECK(abs_diff(smooth.value, 1.718281828459045) < 1e-9);

    // Tiny budget: out_of_range, best estimate still returned.
    const auto limited = integrate_romberg<3U>(lorentz, 0.0, 1.0, 1e-14);
    CASTLE_SAMPLE_CHECK(!limited.ok());
    CASTLE_SAMPLE_CHECK(limited.code == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(limited.iterations == 3U);
    CASTLE_SAMPLE_CHECK(abs_diff(limited.value, 0.7853981633974483) < 1e-3);

    const auto single_float = integrate_romberg([](float x) { return x * x; }, 0.0F, 3.0F, 1e-5F);
    CASTLE_SAMPLE_CHECK(single_float.ok());
    CASTLE_SAMPLE_CHECK(near_equal(single_float.value, 9.0F, 1e-4F));

    // Sample integrators: y = x^2 at x = 0..4 with dx = 1.
    const double y5[] = {0.0, 1.0, 4.0, 9.0, 16.0};
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_trapezoid_samples(y5, 5U, 1.0), 22.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_trapezoid_samples(y5, 0U, 1.0), 0.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_trapezoid_samples(y5, 1U, 1.0), 0.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_trapezoid_samples(y5, 2U, 0.5), 0.25) < 1e-12);

    CASTLE_SAMPLE_CHECK(abs_diff(integrate_simpson_samples(y5, 5U, 1.0), 64.0 / 3.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_simpson_samples(y5, 1U, 1.0), 0.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_simpson_samples(y5, 2U, 1.0), 0.5) < 1e-12);  // trapezoid fallback
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_simpson_samples(y5, 3U, 1.0), 8.0 / 3.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_simpson_samples(y5, 4U, 1.0), 9.0) < 1e-12);  // Simpson 3/8 only
    const double y6[] = {0.0, 1.0, 4.0, 9.0, 16.0, 25.0};
    CASTLE_SAMPLE_CHECK(abs_diff(integrate_simpson_samples(y6, 6U, 1.0), 125.0 / 3.0) < 1e-12);  // Simpson + 3/8 tail

    const float yf[] = {0.0F, 1.0F, 4.0F, 9.0F, 16.0F};
    CASTLE_SAMPLE_CHECK(near_equal(integrate_simpson_samples(yf, 5U, 1.0F), 64.0F / 3.0F, 1e-5F));
    CASTLE_SAMPLE_CHECK(near_equal(integrate_trapezoid_samples(yf, 5U, 1.0F), 22.0F, 1e-5F));

    // Cumulative trapezoid.
    double running[5] = {};
    CASTLE_SAMPLE_CHECK(integrate_cumulative_trapezoid(y5, 5U, 1.0, running) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(abs_diff(running[0], 0.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(running[1], 0.5) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(running[2], 3.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(running[3], 9.5) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(running[4], 22.0) < 1e-12);

    double aliased[5] = {0.0, 1.0, 4.0, 9.0, 16.0};
    CASTLE_SAMPLE_CHECK(integrate_cumulative_trapezoid(aliased, 5U, 1.0, aliased) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(abs_diff(aliased[4], 22.0) < 1e-12);

    CASTLE_SAMPLE_CHECK(integrate_cumulative_trapezoid<double>(nullptr, 5U, 1.0, running) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(integrate_cumulative_trapezoid<double>(y5, 5U, 1.0, nullptr) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(integrate_cumulative_trapezoid(y5, 0U, 1.0, running) == castle::status::empty);

    return 0;
}

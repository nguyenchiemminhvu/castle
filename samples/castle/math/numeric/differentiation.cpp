#include "sample_support.hpp"

#include "castle/math/exponential.hpp"
#include "castle/math/near_equal.hpp"
#include "castle/math/numeric/differentiation.hpp"

namespace
{

double cube_fn(double x)
{
    return x * x * x;
}

struct quartic_functor
{
    double operator()(double x) const
    {
        return x * x * x * x;
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

    const auto expo = [](double x) { return castle::math::exp(x); };
    const double e1 = 2.718281828459045;

    // First-order one-sided differences on x^3 at x = 2 (f' = 12).
    CASTLE_SAMPLE_CHECK(abs_diff(derivative_forward(cube_fn, 2.0, 1e-5), 12.0) < 1e-3);
    CASTLE_SAMPLE_CHECK(abs_diff(derivative_backward(cube_fn, 2.0, 1e-5), 12.0) < 1e-3);
    CASTLE_SAMPLE_CHECK(abs_diff(derivative_forward(cube_fn, 2.0, -1e-5), 12.0) < 1e-3);

    // Second-order central difference.
    CASTLE_SAMPLE_CHECK(abs_diff(derivative_central(cube_fn, 2.0, 1e-4), 12.0) < 1e-6);
    CASTLE_SAMPLE_CHECK(abs_diff(derivative_central(expo, 1.0, 1e-4), e1) < 1e-6);
    CASTLE_SAMPLE_CHECK(near_equal(derivative_central([](float x) { return x * x; }, 3.0F, 0.5F), 6.0F, 1e-5F));

    // Five-point stencil: exact for polynomials up to degree 4 (x^4 at 1.5 -> 13.5).
    CASTLE_SAMPLE_CHECK(abs_diff(derivative_five_point(quartic_functor{}, 1.5, 0.25), 13.5) < 1e-10);
    CASTLE_SAMPLE_CHECK(abs_diff(derivative_five_point(expo, 1.0, 0.01), e1) < 1e-8);

    // Richardson extrapolation: default 4 levels and explicit levels.
    CASTLE_SAMPLE_CHECK(abs_diff(derivative_richardson(cube_fn, 2.0, 0.5), 12.0) < 1e-9);
    CASTLE_SAMPLE_CHECK(abs_diff(derivative_richardson<2U>(cube_fn, 2.0, 0.1), 12.0) < 1e-9);
    CASTLE_SAMPLE_CHECK(abs_diff(derivative_richardson<6U>(expo, 1.0, 0.5), e1) < 1e-9);

    // Second derivative of x^3 at 2 is 12; exp'' = exp.
    CASTLE_SAMPLE_CHECK(abs_diff(second_derivative(cube_fn, 2.0, 1e-2), 12.0) < 1e-6);
    CASTLE_SAMPLE_CHECK(abs_diff(second_derivative(expo, 1.0, 1e-3), e1) < 1e-5);

    // Sampled data: y = x^2 at x = 0..4 -> dy/dx = 2x.
    const double y[] = {0.0, 1.0, 4.0, 9.0, 16.0};
    double dy[5] = {};
    CASTLE_SAMPLE_CHECK(derivative_samples(y, 5U, 1.0, dy) == castle::status::ok);
    for (unsigned i = 0U; i < 5U; ++i)
    {
        CASTLE_SAMPLE_CHECK(abs_diff(dy[i], 2.0 * static_cast<double>(i)) < 1e-12);
    }

    // Two samples: forward difference everywhere.
    const double two[] = {1.0, 3.0};
    double two_out[2] = {};
    CASTLE_SAMPLE_CHECK(derivative_samples(two, 2U, 0.5, two_out) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(abs_diff(two_out[0], 4.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(two_out[1], 4.0) < 1e-12);

    // In-place and negative spacing (samples listed from x = 4 down to x = 0).
    double inplace[5] = {16.0, 9.0, 4.0, 1.0, 0.0};
    CASTLE_SAMPLE_CHECK(derivative_samples(inplace, 5U, -1.0, inplace) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(abs_diff(inplace[0], 8.0) < 1e-12);
    CASTLE_SAMPLE_CHECK(abs_diff(inplace[4], 0.0) < 1e-12);

    const float yf[] = {0.0F, 1.0F, 4.0F, 9.0F};
    float dyf[4] = {};
    CASTLE_SAMPLE_CHECK(derivative_samples(yf, 4U, 1.0F, dyf) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near_equal(dyf[3], 6.0F, 1e-5F));

    // Error paths.
    CASTLE_SAMPLE_CHECK(derivative_samples<double>(nullptr, 5U, 1.0, dy) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(derivative_samples<double>(y, 5U, 1.0, nullptr) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(derivative_samples(y, 1U, 1.0, dy) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(derivative_samples(y, 5U, 0.0, dy) == castle::status::invalid_argument);

    return 0;
}

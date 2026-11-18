#include "sample_support.hpp"

#include "castle/math/near_equal.hpp"
#include "castle/math/numeric/root_finding.hpp"

namespace
{

double square_minus_two(double x)
{
    return (x * x) - 2.0;
}

struct cubic_functor
{
    double operator()(double x) const
    {
        return (x * x * x) - 8.0;
    }
};

} // namespace

int main()
{
    const double root2 = 1.4142135623730951;
    const double tolerance = 1.0e-10;

    // Bisection with a lambda: f(x) = x^2 - 2 on [0, 2].
    const auto f = [](double x) { return (x * x) - 2.0; };
    const castle::math::numeric_result<double> bisection =
        castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-12);
    CASTLE_SAMPLE_CHECK(bisection.ok());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(bisection.value, root2, tolerance));
    CASTLE_SAMPLE_CHECK(bisection.iterations > 0U);
    CASTLE_SAMPLE_CHECK(bisection.error <= 1.0e-12 * 2.0);

    // The bracket may be given in either order, and callables may be function pointers or functors.
    const auto reversed = castle::math::find_root_bisection(&square_minus_two, 2.0, 0.0, 1.0e-12);
    CASTLE_SAMPLE_CHECK(reversed.ok());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(reversed.value, root2, tolerance));

    const auto cubic = castle::math::find_root_bisection(cubic_functor(), 0.0, 4.0, 1.0e-12);
    CASTLE_SAMPLE_CHECK(cubic.ok());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(cubic.value, 2.0, tolerance));

    // An exact zero at an end point is returned immediately, without iterating.
    const auto linear = castle::math::find_root_bisection([](double x) { return x - 1.0; }, 0.0, 1.0, 1.0e-12);
    CASTLE_SAMPLE_CHECK(linear.ok());
    CASTLE_SAMPLE_CHECK(linear.value == 1.0);
    CASTLE_SAMPLE_CHECK(linear.iterations == 0U);

    // A bracket without a sign change is rejected.
    const auto no_sign_change = castle::math::find_root_bisection(f, 2.0, 3.0, 1.0e-12);
    CASTLE_SAMPLE_CHECK(no_sign_change.code == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(!no_sign_change.ok());

    // A tiny iteration budget reports out_of_range but still returns the best estimate.
    const auto exhausted = castle::math::find_root_bisection(f, 0.0, 2.0, 1.0e-12, 3U);
    CASTLE_SAMPLE_CHECK(exhausted.code == castle::status::out_of_range);
    CASTLE_SAMPLE_CHECK(exhausted.iterations == 3U);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(exhausted.value, root2, 0.25));

    // Newton-Raphson needs the derivative.
    const auto df = [](double x) { return 2.0 * x; };
    const auto newton = castle::math::find_root_newton(f, df, 1.0, 1.0e-12);
    CASTLE_SAMPLE_CHECK(newton.ok());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(newton.value, root2, tolerance));
    CASTLE_SAMPLE_CHECK(newton.iterations < 10U);

    // A vanishing derivative is rejected: f'(0) == 0.
    const auto flat_newton = castle::math::find_root_newton(f, df, 0.0, 1.0e-12);
    CASTLE_SAMPLE_CHECK(flat_newton.code == castle::status::invalid_argument);

    const auto newton_budget = castle::math::find_root_newton(f, df, 100.0, 1.0e-12, 2U);
    CASTLE_SAMPLE_CHECK(newton_budget.code == castle::status::out_of_range);

    // The secant method is derivative-free.
    const auto secant = castle::math::find_root_secant(f, 1.0, 2.0, 1.0e-12);
    CASTLE_SAMPLE_CHECK(secant.ok());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(secant.value, root2, tolerance));

    // Equal function values at both starting points give a flat secant.
    const auto flat_secant = castle::math::find_root_secant(f, -1.0, 1.0, 1.0e-12);
    CASTLE_SAMPLE_CHECK(flat_secant.code == castle::status::invalid_argument);

    const auto secant_budget = castle::math::find_root_secant(f, 100.0, 200.0, 1.0e-12, 2U);
    CASTLE_SAMPLE_CHECK(secant_budget.code == castle::status::out_of_range);

    // Single precision works as well.
    const auto ff = [](float x) { return (x * x) - 2.0F; };
    const auto single = castle::math::find_root_bisection(ff, 0.0F, 2.0F, 1.0e-6F);
    CASTLE_SAMPLE_CHECK(single.ok());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(single.value, 1.4142135F, 1.0e-5F));

    return 0;
}

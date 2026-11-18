#include "sample_support.hpp"

#include "castle/math/near_equal.hpp"
#include "castle/math/numeric.hpp"

int main()
{
    const double epsilon = 1.0e-6;

    const auto square = [](double x) { return x * x; };
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::integrate_simpson(square, 0.0, 3.0, 100U), 9.0, epsilon));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::derivative_central(square, 3.0, 1.0e-4), 6.0, epsilon));

    const auto shifted = [](double x) { return (x * x) - 2.0; };
    const castle::math::numeric_result<double> root = castle::math::find_root_bisection(shifted, 0.0, 2.0, 1.0e-12);
    CASTLE_SAMPLE_CHECK(root.ok());
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(root.value, 1.4142135623730951, epsilon));

    const auto decay = [](double, double y) { return -y; };
    double y = 1.0;
    for (int i = 0; i < 10; ++i)
    {
        y = castle::math::rk4_step(decay, 0.1 * static_cast<double>(i), y, 0.1);
    }
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(y, 0.36787944117144233, epsilon));

    return 0;
}

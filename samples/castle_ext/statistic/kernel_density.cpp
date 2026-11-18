#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/kernel_density.hpp"
#include "castle_ext/statistic/sample_window.hpp"

int main()
{
    // Reported horizontal accuracy (hAcc, metres) of recent fixes.
    castle::statistic::sample_window<double, 8U> accuracy;
    const double reported[] = {1.0, 1.2, 1.1, 0.9, 1.0, 3.0};
    for (const double value : reported)
    {
        accuracy.push(value);
    }

    // Rule-of-thumb bandwidth: 1.06 * s * n^(-1/5).
    double h = 0.0;
    CASTLE_SAMPLE_CHECK(castle::statistic::normal_reference_bandwidth(accuracy.begin(), accuracy.end(), h) ==
                        castle::status::ok);
    CASTLE_SAMPLE_CHECK(h > 0.0);

    // Smooth density estimate: high near the cluster around 1 m, lower near the outlier.
    double near_cluster = 0.0;
    double near_outlier = 0.0;
    CASTLE_SAMPLE_CHECK(castle::statistic::kernel_density(accuracy.begin(), accuracy.end(), 1.0, h, near_cluster) ==
                        castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::statistic::kernel_density(accuracy.begin(), accuracy.end(), 3.0, h, near_outlier) ==
                        castle::status::ok);
    CASTLE_SAMPLE_CHECK(near_cluster > near_outlier);

    // Compact kernels: a single sample at 0 evaluated at x = 0.5, h = 1.
    const double single[] = {0.0};
    double density = 0.0;
    CASTLE_SAMPLE_CHECK(castle::statistic::kernel_density<castle::statistic::epanechnikov_kernel>(
                            single, single + 1, 0.5, 1.0, density) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(density - 0.5625) < 1e-12);
    CASTLE_SAMPLE_CHECK(castle::statistic::kernel_density<castle::statistic::uniform_kernel>(
                            single, single + 1, 0.5, 1.0, density) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(density - 0.5) < 1e-12);

    // Invalid bandwidth is rejected and the output is left alone.
    CASTLE_SAMPLE_CHECK(castle::statistic::kernel_density(single, single + 1, 0.5, 0.0, density) ==
                        castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(castle::math::abs(density - 0.5) < 1e-12);
    return 0;
}

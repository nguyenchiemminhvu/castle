#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/statistic.hpp"

int main()
{
    // Horizontal position errors (east, north) in metres from a receiver at rest.
    const double east[] = {0.3, -0.4, 0.0, 0.6, -0.3};
    const double north[] = {0.4, 0.3, 1.0, 0.8, -0.4};
    const double radius_expected[] = {0.5, 0.5, 1.0, 1.0, 0.5};

    castle::statistic::exact_cep<double, 100U> horizontal;
    castle::statistic::running_moments<double> moments;
    castle::statistic::sliding_moments<double, 4U> recent;
    castle::statistic::histogram<double, 4U> bins(0.0, 2.0);
    castle::statistic::p2_quantile<double> median(0.5);
    castle::statistic::ewma<double> smoothed(0.5);
    castle::statistic::online_regression<double> trend;

    for (unsigned i = 0U; i < 5U; ++i)
    {
        CASTLE_SAMPLE_CHECK(horizontal.push_residual(east[i], north[i]) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(moments.push(radius_expected[i]) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(recent.push(radius_expected[i]) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(bins.push(radius_expected[i]) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(median.push(radius_expected[i]) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(smoothed.push(radius_expected[i]) == castle::status::ok);
        CASTLE_SAMPLE_CHECK(trend.push(static_cast<double>(i), radius_expected[i]) == castle::status::ok);
    }

    // CEP of the latest fixes: sorted radii {0.5, 0.5, 0.5, 1, 1}, median 0.5.
    double value = 0.0;
    CASTLE_SAMPLE_CHECK(horizontal.cep50(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 0.5) < 1e-9);
    CASTLE_SAMPLE_CHECK(horizontal.cep95(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(value > 0.9 && value < 1.0 + 1e-9);

    // Mean of all radii: (0.5 + 0.5 + 1 + 1 + 0.5) / 5 = 0.7.
    CASTLE_SAMPLE_CHECK(moments.mean(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 0.7) < 1e-9);

    // The 4-sample window dropped the first radius: (0.5 + 1 + 1 + 0.5) / 4 = 0.75.
    CASTLE_SAMPLE_CHECK(recent.mean(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 0.75) < 1e-9);

    // Histogram of radii over [0, 2): all five fixes counted, none outside.
    CASTLE_SAMPLE_CHECK(bins.count() == 5U);
    CASTLE_SAMPLE_CHECK(bins.overflow() == 0U);

    // P2 holds the first five samples exactly: the median is 0.5.
    CASTLE_SAMPLE_CHECK(median.quantile(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 0.5) < 1e-9);

    // EWMA with alpha 0.5: 0.5, 0.5, 0.75, 0.875, 0.6875.
    CASTLE_SAMPLE_CHECK(smoothed.mean(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 0.6875) < 1e-9);

    // Regression of radius against fix index: a mild upward trend.
    double slope = 0.0;
    double intercept = 0.0;
    CASTLE_SAMPLE_CHECK(trend.fit(slope, intercept) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(slope > 0.0);

    // Kernel density over the retained, sorted radii.
    const auto sorted = horizontal.estimator().sorted();
    double h = 0.0;
    CASTLE_SAMPLE_CHECK(castle::statistic::normal_reference_bandwidth(sorted.begin(), sorted.end(), h) ==
                        castle::status::ok);
    double density = 0.0;
    CASTLE_SAMPLE_CHECK(castle::statistic::kernel_density(sorted.begin(), sorted.end(), 0.5, h, density) ==
                        castle::status::ok);
    CASTLE_SAMPLE_CHECK(density > 0.0);

    // Exact order statistics of a caller-owned array.
    double scratch[] = {3.0, 1.0, 2.0};
    double q = 0.0;
    CASTLE_SAMPLE_CHECK(castle::statistic::quantile_inplace(scratch, scratch + 3, 0.5, q) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(q - 2.0) < 1e-12);
    return 0;
}

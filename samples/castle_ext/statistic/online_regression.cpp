#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/online_regression.hpp"

int main()
{
    // Receiver clock offset (microseconds) against time (seconds): a drift of
    // 2 us/s on top of a 1 us offset, learned one pair at a time.
    castle::statistic::online_regression<double> drift;
    CASTLE_SAMPLE_CHECK(drift.empty());

    for (int second = 0; second < 5; ++second)
    {
        const double t = static_cast<double>(second);
        CASTLE_SAMPLE_CHECK(drift.push(t, 1.0 + 2.0 * t) == castle::status::ok);
    }
    CASTLE_SAMPLE_CHECK(drift.count() == 5U);

    double slope = 0.0;
    double intercept = 0.0;
    CASTLE_SAMPLE_CHECK(drift.fit(slope, intercept) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(slope - 2.0) < 1e-9);
    CASTLE_SAMPLE_CHECK(castle::math::abs(intercept - 1.0) < 1e-9);

    // A perfectly linear stream is perfectly correlated.
    double r = 0.0;
    CASTLE_SAMPLE_CHECK(drift.correlation(r) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(r - 1.0) < 1e-9);

    // Covariance of x = {0..4} with y = 1 + 2x: Sxy = 20.
    double cov = 0.0;
    CASTLE_SAMPLE_CHECK(drift.population_covariance(cov) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(cov - 4.0) < 1e-9);
    CASTLE_SAMPLE_CHECK(drift.sample_covariance(cov) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(cov - 5.0) < 1e-9);
    // Two points with the same x cannot define a line: fit() rejects them.
    castle::statistic::online_regression<double> vertical;
    CASTLE_SAMPLE_CHECK(vertical.push(1.0, 1.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(vertical.push(1.0, 2.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(vertical.fit(slope, intercept) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(drift.count() == 5U);

    drift.clear();
    CASTLE_SAMPLE_CHECK(drift.empty());
    return 0;
}

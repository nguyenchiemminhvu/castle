#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/running_moments.hpp"

namespace
{
bool near(double actual, double expected, double tolerance)
{
    return castle::math::abs(actual - expected) <= tolerance;
}
} // namespace

int main()
{
    // Summarize a batch of fix-to-fix distance errors (metres) in one pass.
    castle::statistic::running_moments<double, 4U> errors;
    const double batch[] = {1.0, 2.0, 3.0, 4.0, 10.0};
    CASTLE_SAMPLE_CHECK(errors.push_range(batch, batch + 5) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(errors.count() == 5U);

    double value = 0.0;
    CASTLE_SAMPLE_CHECK(errors.mean(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 4.0, 1e-12));
    CASTLE_SAMPLE_CHECK(errors.population_variance(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 10.0, 1e-9));
    CASTLE_SAMPLE_CHECK(errors.sample_variance(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 12.5, 1e-9));
    CASTLE_SAMPLE_CHECK(errors.skewness(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 1.13841995766, 1e-9));
    CASTLE_SAMPLE_CHECK(errors.excess_kurtosis(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, -0.212, 1e-9));
    CASTLE_SAMPLE_CHECK(errors.range(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 9.0, 1e-12));

    // Non-finite readings are rejected and leave the statistics untouched.
    CASTLE_SAMPLE_CHECK(errors.push(__builtin_nan("")) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(errors.count() == 5U);

    // Two receivers (or two threads) can summarize halves and merge them.
    castle::statistic::running_moments<double, 4U> first_half;
    castle::statistic::running_moments<double, 4U> second_half;
    CASTLE_SAMPLE_CHECK(first_half.push(1.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(first_half.push(2.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(first_half.push(3.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(second_half.push(4.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(second_half.push(10.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(first_half.merge(second_half) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(first_half.population_variance(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 10.0, 1e-9));

    // Integer samples report in double.
    castle::statistic::running_moments<int, 2U> counts;
    CASTLE_SAMPLE_CHECK(counts.push(2) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(counts.push(4) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(counts.mean(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 3.0, 1e-12));

    // The core accumulator also retracts samples.
    castle::statistic::central_moments<double, 2U> core;
    CASTLE_SAMPLE_CHECK(core.push(1.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(core.push(3.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(core.push(8.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(core.remove(8.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(core.mean(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 2.0, 1e-12));

    errors.clear();
    CASTLE_SAMPLE_CHECK(errors.empty());

    return 0;
}

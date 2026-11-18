#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/sliding_moments.hpp"

namespace
{
bool near(double actual, double expected, double tolerance)
{
    return castle::math::abs(actual - expected) <= tolerance;
}
} // namespace

int main()
{
    // Moving statistics of the newest four speed readings (m/s).
    castle::statistic::sliding_moments<double, 4U, 4U> speed;
    CASTLE_SAMPLE_CHECK(speed.empty());

    // A single GNSS glitch of 1e9 sits in the window with three real readings.
    CASTLE_SAMPLE_CHECK(speed.push(1.0e9) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(speed.push(1.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(speed.push(2.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(speed.push(3.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(speed.full());

    // The next reading evicts the glitch; the statistics recover to full precision.
    CASTLE_SAMPLE_CHECK(speed.push(4.0) == castle::status::ok);
    double value = 0.0;
    CASTLE_SAMPLE_CHECK(speed.mean(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 2.5, 1e-9));
    CASTLE_SAMPLE_CHECK(speed.population_variance(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 1.25, 1e-9));
    CASTLE_SAMPLE_CHECK(speed.sample_variance(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 5.0 / 3.0, 1e-9));
    CASTLE_SAMPLE_CHECK(speed.skewness(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 0.0, 1e-9));
    CASTLE_SAMPLE_CHECK(speed.excess_kurtosis(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, -1.36, 1e-9));

    // Extremes are scanned over the window on demand.
    CASTLE_SAMPLE_CHECK(speed.minimum(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 1.0, 1e-12));
    CASTLE_SAMPLE_CHECK(speed.maximum(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 4.0, 1e-12));
    CASTLE_SAMPLE_CHECK(speed.range(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 3.0, 1e-12));

    // The retained samples and the underlying accumulator are readable.
    CASTLE_SAMPLE_CHECK(speed.samples().size() == 4U);
    CASTLE_SAMPLE_CHECK(speed.moments().count() == 4U);

    // Non-finite readings are rejected without changing the window.
    CASTLE_SAMPLE_CHECK(speed.push(__builtin_inf()) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(speed.count() == 4U);

    // Integer samples report in double.
    castle::statistic::sliding_moments<int, 3U> counts;
    for (int i = 1; i <= 4; ++i)
    {
        CASTLE_SAMPLE_CHECK(counts.push(i) == castle::status::ok);
    }
    CASTLE_SAMPLE_CHECK(counts.mean(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 3.0, 1e-12));

    speed.clear();
    CASTLE_SAMPLE_CHECK(speed.empty());
    CASTLE_SAMPLE_CHECK(speed.mean(value) == castle::status::empty);

    return 0;
}

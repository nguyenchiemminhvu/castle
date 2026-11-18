#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/sorted_window.hpp"

namespace
{
bool near(double actual, double expected)
{
    return castle::math::abs(actual - expected) < 1e-12;
}
} // namespace

int main()
{
    // Exact statistics of the newest five latency readings, in milliseconds.
    castle::statistic::sorted_window<int, 5U> latency;

    const int readings[] = {5, 1, 4, 1, 3};
    for (const int reading : readings)
    {
        CASTLE_SAMPLE_CHECK(latency.push(reading) == castle::status::ok);
    }
    CASTLE_SAMPLE_CHECK(latency.full());
    CASTLE_SAMPLE_CHECK(latency.count() == 5U);

    // The sorted view is always ascending: 1 1 3 4 5.
    CASTLE_SAMPLE_CHECK(latency.sorted()[0U] == 1);
    CASTLE_SAMPLE_CHECK(latency.sorted()[2U] == 3);
    CASTLE_SAMPLE_CHECK(latency.sorted()[4U] == 5);

    double value = 0.0;
    CASTLE_SAMPLE_CHECK(latency.median(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 3.0));
    CASTLE_SAMPLE_CHECK(latency.minimum(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 1.0));
    CASTLE_SAMPLE_CHECK(latency.maximum(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 5.0));
    CASTLE_SAMPLE_CHECK(latency.range(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 4.0));
    CASTLE_SAMPLE_CHECK(latency.mode(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 1.0));

    // Pushing into a full window evicts the oldest sample (5): window is 1 4 1 3 9 -> sorted 1 1 3 4 9.
    CASTLE_SAMPLE_CHECK(latency.push(9) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(latency.sorted()[4U] == 9);
    CASTLE_SAMPLE_CHECK(latency.samples()[0U] == 1);
    CASTLE_SAMPLE_CHECK(latency.samples()[4U] == 9);

    // Linear 90th percentile: position 0.9 * 4 = 3.6 -> 4 + (9 - 4) * 0.6 = 7.
    CASTLE_SAMPLE_CHECK(latency.quantile(0.9, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 7.0));
    // Nearest rank returns a value that was actually measured: ceil(0.9 * 5) = 5th sample.
    CASTLE_SAMPLE_CHECK(latency.quantile(0.9, value, castle::statistic::quantile_method::nearest_rank)
                        == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 9.0));

    // Q3 - Q1 = 4 - 1.
    CASTLE_SAMPLE_CHECK(latency.interquartile_range(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 3.0));

    // Fraction of readings at or below 3 ms: 3 of 5.
    CASTLE_SAMPLE_CHECK(latency.cdf(3.0, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 0.6));

    // Floating-point windows reject NaN. (A volatile zero keeps the compiler from folding 0/0.)
    volatile float zero = 0.0f;
    const float not_a_number = zero / zero;
    castle::statistic::sorted_window<float, 4U> temperature;
    CASTLE_SAMPLE_CHECK(temperature.push(21.5f) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(temperature.push(not_a_number) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(temperature.count() == 1U);

    latency.clear();
    CASTLE_SAMPLE_CHECK(latency.empty());
    CASTLE_SAMPLE_CHECK(latency.median(value) == castle::status::empty);

    return 0;
}

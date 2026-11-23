#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/histogram.hpp"

namespace
{
bool near(double actual, double expected)
{
    return castle::math::abs(actual - expected) < 1e-9;
}
} // namespace

int main()
{
    // Horizontal error histogram: 50 bins of 0.2 m covering [0 m, 10 m).
    castle::statistic::histogram<double, 50U> errors(0.0, 10.0);
    CASTLE_SAMPLE_CHECK(errors.configured());
    CASTLE_SAMPLE_CHECK(near(errors.bin_width(), 0.2));

    CASTLE_SAMPLE_CHECK(errors.push(1.1) == castle::status::ok);     // bin 5 = [1.0, 1.2)
    CASTLE_SAMPLE_CHECK(errors.push(2.9, 3U) == castle::status::ok); // weight 3: bin 14 = [2.8, 3.0)
    CASTLE_SAMPLE_CHECK(errors.count() == 4U);

    uint32_t weight = 0U;
    CASTLE_SAMPLE_CHECK(errors.bin(14U, weight) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(weight == 3U);

    double lower = 0.0;
    double upper = 0.0;
    CASTLE_SAMPLE_CHECK(errors.bin_edges(14U, lower, upper) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(lower, 2.8));
    CASTLE_SAMPLE_CHECK(near(upper, 3.0));

    // Weight is spread evenly inside a bin: target 1 of 4 ends bin 5, target 2 is 1/3 into bin 14.
    double value = 0.0;
    CASTLE_SAMPLE_CHECK(errors.quantile(0.25, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 1.2));
    CASTLE_SAMPLE_CHECK(errors.quantile(0.5, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 2.0 + 0.8 + 0.2 / 3.0));
    CASTLE_SAMPLE_CHECK(errors.mode(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 2.9));
    CASTLE_SAMPLE_CHECK(errors.cdf(3.0, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 1.0));

    // Values outside [0, 10) are tallied separately and never distort the edge bins.
    CASTLE_SAMPLE_CHECK(errors.push(25.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(errors.overflow() == 1U);
    CASTLE_SAMPLE_CHECK(errors.quantile(1.0, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 10.0));

    // Invalid input is rejected without touching the histogram.
    CASTLE_SAMPLE_CHECK(errors.push(1.0, 0U) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(errors.count() == 5U);

    // Retract weight that was added before (this is how sliding_histogram evicts samples).
    CASTLE_SAMPLE_CHECK(errors.remove(2.9, 3U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(errors.remove(2.9) == castle::status::not_found);
    CASTLE_SAMPLE_CHECK(errors.count() == 2U);

    // Clearing keeps the range.
    errors.clear();
    CASTLE_SAMPLE_CHECK(errors.empty());
    CASTLE_SAMPLE_CHECK(errors.configured());

    // Invalid ranges leave the histogram unconfigured.
    castle::statistic::histogram<double, 4U> unconfigured;
    CASTLE_SAMPLE_CHECK(unconfigured.push(1.0) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(unconfigured.configure(1.0, 1.0) == castle::status::invalid_config);
    CASTLE_SAMPLE_CHECK(!unconfigured.configured());

    // Expanding histogram: the range doubles until the value fits; eight bins stay eight bins.
    castle::statistic::histogram<double, 8U, uint32_t, castle::statistic::histogram_range::expanding> radius(0.0, 1.0);
    CASTLE_SAMPLE_CHECK(radius.push(0.3) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(radius.push(5.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(radius.lower(), 0.0));
    CASTLE_SAMPLE_CHECK(near(radius.upper(), 8.0));
    CASTLE_SAMPLE_CHECK(radius.overflow() == 0U);
    CASTLE_SAMPLE_CHECK(radius.bin(0U, weight) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(weight == 1U);
    CASTLE_SAMPLE_CHECK(radius.bin(5U, weight) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(weight == 1U);

    return 0;
}

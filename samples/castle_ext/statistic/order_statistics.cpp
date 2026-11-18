#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/order_statistics.hpp"

namespace
{
bool near(double actual, double expected)
{
    return castle::math::abs(actual - expected) < 1e-12;
}
} // namespace

int main()
{
    // Horizontal errors of four GNSS fixes, in metres, in arrival order.
    double errors[4U] = {2.0, 0.5, 1.5, 1.0};

    // Validate and sort once, then run any number of O(1) queries.
    CASTLE_SAMPLE_CHECK(castle::statistic::sort_samples(errors, errors + 4) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(errors[0U] == 0.5);
    CASTLE_SAMPLE_CHECK(errors[3U] == 2.0);

    double value = 0.0;
    CASTLE_SAMPLE_CHECK(castle::statistic::median_sorted(errors, errors + 4, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 1.25));

    // Linear (type 7) interpolation: 1.5 + 0.5 * 0.85.
    CASTLE_SAMPLE_CHECK(castle::statistic::quantile_sorted(errors, errors + 4, 0.95, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 1.925));

    // Nearest rank (type 1) always returns a measured sample.
    CASTLE_SAMPLE_CHECK(castle::statistic::quantile_sorted(
                            errors, errors + 4, 0.95, value, castle::statistic::quantile_method::nearest_rank)
                        == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 2.0));

    // Q3 - Q1 = 1.625 - 0.875.
    CASTLE_SAMPLE_CHECK(castle::statistic::interquartile_range_sorted(errors, errors + 4, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 0.75));

    // Invalid requests leave the output untouched.
    value = -1.0;
    CASTLE_SAMPLE_CHECK(castle::statistic::quantile_sorted(errors, errors + 4, 1.5, value)
                        == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(near(value, -1.0));
    CASTLE_SAMPLE_CHECK(castle::statistic::quantile_sorted(errors, errors, 0.5, value) == castle::status::empty);

    // One-off query on unsorted data; the range is sorted as a side effect.
    double unsorted[4U] = {4.0, 1.0, 3.0, 2.0};
    CASTLE_SAMPLE_CHECK(castle::statistic::quantile_inplace(unsorted, unsorted + 4, 0.5, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 2.5));
    CASTLE_SAMPLE_CHECK(unsorted[0U] == 1.0);
    CASTLE_SAMPLE_CHECK(unsorted[3U] == 4.0);

    // Mode of discrete data; ties would resolve to the smallest value.
    uint16_t counts[6U] = {1U, 2U, 2U, 3U, 3U, 3U};
    CASTLE_SAMPLE_CHECK(castle::statistic::mode_sorted(counts, counts + 6, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 3.0));

    return 0;
}

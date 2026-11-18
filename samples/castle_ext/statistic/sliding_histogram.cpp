#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/sliding_histogram.hpp"

namespace
{
bool near(double actual, double expected)
{
    return castle::math::abs(actual - expected) < 1e-9;
}
} // namespace

int main()
{
    // Newest three radial errors in metres; ten 1 m bins over [0 m, 10 m).
    castle::statistic::sliding_histogram<unsigned, 3U, 10U> errors(0.0, 10.0);
    CASTLE_SAMPLE_CHECK(errors.configured());

    const unsigned fixes[] = {1U, 2U, 3U, 4U, 11U};
    for (const unsigned fix : fixes)
    {
        CASTLE_SAMPLE_CHECK(errors.push(fix) == castle::status::ok);
    }

    // The window now holds {3, 4, 11}; 1 and 2 were evicted and retracted from the histogram.
    CASTLE_SAMPLE_CHECK(errors.full());
    CASTLE_SAMPLE_CHECK(errors.count() == 3U);
    CASTLE_SAMPLE_CHECK(errors.samples()[0U] == 3U);
    CASTLE_SAMPLE_CHECK(errors.samples()[2U] == 11U);
    CASTLE_SAMPLE_CHECK(errors.distribution().overflow() == 1U);

    // 11 m is outside the range, so it counts as overflow instead of landing in a bin.
    uint8_t weight = 9U;
    CASTLE_SAMPLE_CHECK(errors.distribution().bin(1U, weight) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(weight == 0U);
    CASTLE_SAMPLE_CHECK(errors.distribution().bin(3U, weight) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(weight == 1U);

    // Target 1.5 of 3 sits halfway through bin 4 = [4, 5).
    double value = 0.0;
    CASTLE_SAMPLE_CHECK(errors.median(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 4.5));

    // Overflow weight is reported at the upper bound.
    CASTLE_SAMPLE_CHECK(errors.quantile(1.0, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 10.0));

    // Bins 3 and 4 tie, so the mode is the centre of the lower one.
    CASTLE_SAMPLE_CHECK(errors.mode(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 3.5));

    // One of three samples is below 4 m.
    CASTLE_SAMPLE_CHECK(errors.cdf(4.0, value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(near(value, 1.0 / 3.0));

    // An unconfigured instance refuses samples until a range is set; configure() clears the window.
    castle::statistic::sliding_histogram<unsigned, 3U, 10U> other;
    CASTLE_SAMPLE_CHECK(other.push(1U) == castle::status::not_configured);
    CASTLE_SAMPLE_CHECK(other.configure(0.0, 20.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(other.push(15U) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(errors.configure(0.0, 20.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(errors.empty());

    errors.push(5U);
    errors.clear();
    CASTLE_SAMPLE_CHECK(errors.empty());
    CASTLE_SAMPLE_CHECK(errors.median(value) == castle::status::empty);

    return 0;
}

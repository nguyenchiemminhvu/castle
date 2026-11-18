#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle_ext/statistic/ewma.hpp"

int main()
{
    // Smooth a speed signal with alpha = 0.2 (newest sample weighs 20 %).
    castle::statistic::ewma<double> speed(0.2);
    CASTLE_SAMPLE_CHECK(speed.configured());
    CASTLE_SAMPLE_CHECK(speed.empty());

    double value = 0.0;
    CASTLE_SAMPLE_CHECK(speed.mean(value) == castle::status::empty);

    // The first sample initializes the mean; the variance starts at zero.
    CASTLE_SAMPLE_CHECK(speed.push(10.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(speed.mean(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 10.0) < 1e-12);

    // diff = 2, increment = 0.4 -> mean 10.4, variance 0.8 * (0 + 2 * 0.4) = 0.64.
    CASTLE_SAMPLE_CHECK(speed.push(12.0) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(speed.mean(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 10.4) < 1e-12);
    CASTLE_SAMPLE_CHECK(speed.variance(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 0.64) < 1e-12);
    CASTLE_SAMPLE_CHECK(speed.stddev(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 0.8) < 1e-9);

    // Invalid input is rejected and leaves the state unchanged.
    volatile double zero = 0.0;
    CASTLE_SAMPLE_CHECK(speed.push(zero / zero) == castle::status::invalid_argument);
    CASTLE_SAMPLE_CHECK(speed.mean(value) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(castle::math::abs(value - 10.4) < 1e-12);

    // clear() forgets the samples but keeps alpha.
    speed.clear();
    CASTLE_SAMPLE_CHECK(speed.empty());
    CASTLE_SAMPLE_CHECK(castle::math::abs(speed.alpha() - 0.2) < 1e-12);

    // An invalid smoothing factor leaves the filter unconfigured.
    castle::statistic::ewma<double> bad(1.5);
    CASTLE_SAMPLE_CHECK(!bad.configured());
    CASTLE_SAMPLE_CHECK(bad.push(1.0) == castle::status::not_configured);

    return 0;
}

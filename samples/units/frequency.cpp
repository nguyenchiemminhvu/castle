#include "sample_support.hpp"

#include "castle/units/frequency.hpp"
#include "castle/units/time.hpp"

#include <stdint.h>

namespace
{

// A timer driver: the typed parameters make it impossible to pass a period
// or a raw number of the wrong scale.
struct timer_config
{
    uint32_t reload;
    castle::units::seconds<double> period;
};

timer_config configure_timer(castle::units::hertz<int64_t> clock, castle::units::hertz<int64_t> tick)
{
    timer_config config;
    config.reload = static_cast<uint32_t>(clock.value() / tick.value());
    config.period = 1.0 / castle::units::hertz<double>(tick);
    return config;
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    // Any lossless frequency unit converts implicitly into the parameter.
    const timer_config config = configure_timer(16_MHz, 2_kHz);
    CASTLE_SAMPLE_CHECK(config.reload == 8000U);
    CASTLE_SAMPLE_CHECK(config.period.count() == 0.0005);

    // The reciprocal of a period is a frequency; 20 ms is 50 Hz.
    const castle::units::hertz<double> rate = castle::units::inverse(20.0_ms);
    CASTLE_SAMPLE_CHECK(rate.value() > 49.999 && rate.value() < 50.001);

    // Frequency times a duration is a plain cycle count.
    const double cycles = 50.0_Hz * 2.0_s;
    CASTLE_SAMPLE_CHECK(cycles == 100.0);

    // Exact integral conversion is implicit; truncating needs an explicit step.
    const castle::units::millihertz<int64_t> slow = castle::units::hertz<int64_t>(3);
    const castle::units::hertz<int64_t> coarse(castle::units::millihertz<int64_t>(2500));
    CASTLE_SAMPLE_CHECK(slow.value() == 3000);
    CASTLE_SAMPLE_CHECK(coarse.value() == 2);
    CASTLE_SAMPLE_CHECK(2_GHz == 2000_MHz);
    return 0;
}

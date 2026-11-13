#include "sample_support.hpp"

#include "castle/units/time.hpp"
#include "castle/units/distance.hpp"

#include <stdint.h>

int main()
{
    using namespace castle::units::literals;

    // Integer literals are the int64_t chrono aliases used by timers.
    const castle::chrono::milliseconds timeout = 250_ms;
    CASTLE_SAMPLE_CHECK(timeout.count() == 250);
    CASTLE_SAMPLE_CHECK(2_s + 500_ms == 2500_ms);

    // Floating literals keep sub-tick precision.
    const castle::units::seconds<double> dt = 0.02_s;
    CASTLE_SAMPLE_CHECK(dt.count() == 0.02);

    // Durations take part in units arithmetic directly.
    const auto speed = 100.0_m / 4_s;
    CASTLE_SAMPLE_CHECK(speed.value() == 25.0);
    const castle::units::seconds<double> eta = 1.0_km / speed;
    CASTLE_SAMPLE_CHECK(eta.count() == 40.0);

    // Small targets can use a 32-bit tick count.
    const castle::units::milliseconds<uint32_t> tick(castle::units::seconds<uint32_t>(3));
    CASTLE_SAMPLE_CHECK(tick.count() == 3000U);
    return 0;
}

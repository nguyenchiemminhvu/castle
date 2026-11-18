#include "sample_support.hpp"

#include "castle/units/energy.hpp"
#include "castle/units/power.hpp"
#include "castle/units/time.hpp"

#include <stdint.h>

namespace
{

// Power-budget check: both arguments are typed in watts.
bool fits_budget(castle::units::watts<double> load, castle::units::watts<double> budget)
{
    return load <= budget;
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    const castle::units::watts<double> load = 1.5_W + 250.0_mW;
    CASTLE_SAMPLE_CHECK(load.value() == 1.75);
    CASTLE_SAMPLE_CHECK(fits_budget(load, 2.0_W));
    CASTLE_SAMPLE_CHECK(!fits_budget(load, 1.0_W));

    // Energy over time is power.
    const castle::units::watts<double> average = 100.0_J / 4.0_s;
    CASTLE_SAMPLE_CHECK(average.value() == 25.0);

    // Power over a duration is energy, convertible to watt-hours.
    const castle::units::watt_hours<double> used = 2.0_W * castle::units::hours<double>(1.5);
    CASTLE_SAMPLE_CHECK(used.value() == 3.0);

    const castle::units::microwatts<int32_t> sleep = castle::units::milliwatts<int32_t>(3);
    CASTLE_SAMPLE_CHECK(sleep.value() == 3000);
    return 0;
}

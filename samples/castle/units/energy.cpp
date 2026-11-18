#include "sample_support.hpp"

#include "castle/units/distance.hpp"
#include "castle/units/energy.hpp"
#include "castle/units/force.hpp"

#include <stdint.h>

namespace
{

// Battery gauge: remaining capacity in joules regardless of how it was specified.
castle::units::joules<double> remaining(castle::units::joules<double> capacity,
                                        castle::units::joules<double> used)
{
    return capacity - used;
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    // A 3 Wh cell, 2 kJ already drawn.
    const castle::units::joules<double> left = remaining(castle::units::watt_hours<double>(3.0), 2.0_kJ);
    CASTLE_SAMPLE_CHECK(left.value() == 8800.0);

    const castle::units::milliwatt_hours<int32_t> pack = castle::units::watt_hours<int32_t>(2);
    CASTLE_SAMPLE_CHECK(pack.value() == 2000);
    CASTLE_SAMPLE_CHECK(1_kWh == 1000_Wh);
    CASTLE_SAMPLE_CHECK(3600_J == 1_Wh);

    // Force times distance is work in joules.
    const castle::units::joules<double> work = 2.0_N * 3.0_m;
    CASTLE_SAMPLE_CHECK(work.value() == 6.0);
    return 0;
}

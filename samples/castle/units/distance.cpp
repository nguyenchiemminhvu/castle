#include "sample_support.hpp"

#include "castle/units/distance.hpp"

#include <stdint.h>

namespace
{

// The parameter type documents and enforces the unit; callers may pass any
// length unit that converts losslessly.
castle::units::meters<double> add_margin(castle::units::meters<double> range)
{
    return range + castle::units::centimeters<double>(50.0);
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    const castle::units::meters<double> route = 1.2_km + 350.0_m;
    CASTLE_SAMPLE_CHECK(route.value() == 1550.0);

    const castle::units::meters<double> padded = add_margin(2.0_km);
    CASTLE_SAMPLE_CHECK(padded.value() == 2000.5);

    // Exact integral conversion is implicit; truncating needs an explicit step.
    const castle::units::millimeters<int32_t> gap = castle::units::centimeters<int32_t>(12);
    const castle::units::centimeters<int32_t> coarse(castle::units::millimeters<int32_t>(129));
    CASTLE_SAMPLE_CHECK(gap.value() == 120);
    CASTLE_SAMPLE_CHECK(coarse.value() == 12);

    // Imperial and nautical units use exact international definitions.
    CASTLE_SAMPLE_CHECK(1_ft == 12_in);
    CASTLE_SAMPLE_CHECK(castle::units::meters<int32_t>(1_nmi).value() == 1852);
    const castle::units::feet<double> altitude = 30.48_m;
    CASTLE_SAMPLE_CHECK(altitude.value() > 99.999 && altitude.value() < 100.001);
    return 0;
}

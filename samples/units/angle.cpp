#include "sample_support.hpp"

#include "castle/units/angle.hpp"
#include "castle/units/distance.hpp"

#include <stdint.h>

namespace
{

bool near(double a, double b)
{
    const double diff = a - b;
    return (diff < 1e-9) && (diff > -1e-9);
}

// A heading is always degrees in [0, 360), whatever unit the caller has.
castle::units::degrees<double> normalize_heading(castle::units::degrees<double> heading)
{
    return castle::units::wrap_positive(heading);
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    CASTLE_SAMPLE_CHECK(near(normalize_heading(370.0_deg).value(), 10.0));
    CASTLE_SAMPLE_CHECK(near(normalize_heading(-0.25_rev).value(), 270.0));

    // Trigonometry takes any angle unit; no manual degree/radian conversion.
    CASTLE_SAMPLE_CHECK(near(castle::units::sin(30.0_deg), 0.5));
    CASTLE_SAMPLE_CHECK(near(castle::units::cos(castle::units::radians<double>(0.0)), 1.0));

    // Bearing from east/north displacements in metres.
    const castle::units::degrees<double> bearing =
        castle::units::atan2(castle::units::meters<double>(100.0), 100.0_m);
    CASTLE_SAMPLE_CHECK(near(bearing.value(), 45.0));

    // Signed error between two headings, in [-180, 180).
    const castle::units::degrees<double> error = castle::units::wrap_signed(350.0_deg - 10.0_deg);
    CASTLE_SAMPLE_CHECK(near(error.value(), -20.0));

    // Fixed-point sensor field: heading in 1e-2 degrees stored as int32_t.
    using centidegree_unit = castle::units::scaled_unit_t<castle::units::degree_unit, castle::units::centi>;
    const castle::units::quantity<centidegree_unit, int32_t> raw_heading(-9000);
    CASTLE_SAMPLE_CHECK(castle::units::wrap_positive(raw_heading).value() == 27000);
    const castle::units::degrees<double> heading = raw_heading;
    CASTLE_SAMPLE_CHECK(near(heading.value(), -90.0));
    return 0;
}

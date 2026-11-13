#include "sample_support.hpp"

#include "castle/units/distance.hpp"
#include "castle/units/time.hpp"
#include "castle/units/velocity.hpp"

#include <stdint.h>

namespace
{

bool near(double a, double b)
{
    const double diff = a - b;
    return (diff < 1e-9) && (diff > -1e-9);
}

// The wheel-speed driver takes metres per second; callers may pass km/h, mph or knots.
bool over_limit(castle::units::meters_per_second<double> speed, castle::units::meters_per_second<double> limit)
{
    return speed > limit;
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    CASTLE_SAMPLE_CHECK(over_limit(100.0_kmph, 25.0_mps));
    CASTLE_SAMPLE_CHECK(!over_limit(80.0_kmph, 25.0_mps));
    CASTLE_SAMPLE_CHECK(over_limit(30.0_kn, 15.0_mps));

    // Speed from an odometer delta and a timestamp delta.
    const castle::units::meters<double> delta = 100.0_m;
    const castle::units::meters_per_second<double> speed = delta / 4.0_s;
    CASTLE_SAMPLE_CHECK(near(speed.value(), 25.0));

    // Dead reckoning: velocity * duration is a distance.
    const castle::units::meters<double> travelled = speed * 10.0_s;
    CASTLE_SAMPLE_CHECK(near(travelled.value(), 250.0));

    // Exact international definitions.
    const castle::units::kilometers_per_hour<double> knot_kmph = 1.0_kn;
    CASTLE_SAMPLE_CHECK(near(knot_kmph.value(), 1.852));
    CASTLE_SAMPLE_CHECK(1000_mmps == 1_mps);
    CASTLE_SAMPLE_CHECK(near(castle::units::meters_per_second<double>(1.0_mph).value(), 0.44704));
    CASTLE_SAMPLE_CHECK(near(castle::units::meters_per_second<double>(castle::units::feet_per_second<double>(1.0)).value(), 0.3048));
    return 0;
}

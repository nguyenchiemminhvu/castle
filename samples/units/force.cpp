#include "sample_support.hpp"

#include "castle/units/force.hpp"
#include "castle/units/mass.hpp"

#include <stdint.h>

namespace
{

using acceleration = castle::units::quantity<
    castle::units::unit<castle::units::dimension<1, 0, -2, 0, 0, 0, 0, 0>>, double>;

// Actuator driver: the force limit is a typed quantity in newtons.
bool within_limit(castle::units::newtons<double> demand, castle::units::newtons<double> limit)
{
    return demand <= limit;
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    const castle::units::newtons<double> thrust = 1.2_kN + 300.0_N;
    CASTLE_SAMPLE_CHECK(thrust.value() == 1500.0);
    CASTLE_SAMPLE_CHECK(within_limit(thrust, 2.0_kN));
    CASTLE_SAMPLE_CHECK(!within_limit(thrust, 1400.0_N));

    // Mass times acceleration is a force in newtons.
    const castle::units::newtons<double> push = 4.0_kg * acceleration(2.5);
    CASTLE_SAMPLE_CHECK(push.value() == 10.0);

    const castle::units::millinewtons<int32_t> fine = castle::units::newtons<int32_t>(2);
    CASTLE_SAMPLE_CHECK(fine.value() == 2000);

    const castle::units::newtons<double> pound_force = 1.0_lbf;
    CASTLE_SAMPLE_CHECK(pound_force.value() > 4.4482 && pound_force.value() < 4.4483);
    return 0;
}

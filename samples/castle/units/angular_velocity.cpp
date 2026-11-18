#include "sample_support.hpp"

#include "castle/units/angle.hpp"
#include "castle/units/angular_velocity.hpp"
#include "castle/units/time.hpp"

#include <stdint.h>

namespace
{

// A motor driver takes the typed speed; callers may use rpm, deg/s or rad/s.
bool within_limit(castle::units::degrees_per_second<double> speed)
{
    return speed <= castle::units::degrees_per_second<double>(3600.0);
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    // 3000 rpm is 18000 deg/s: above the 3600 deg/s (600 rpm) limit.
    CASTLE_SAMPLE_CHECK(!within_limit(3000.0_rpm));
    CASTLE_SAMPLE_CHECK(within_limit(60.0_rpm));
    CASTLE_SAMPLE_CHECK(within_limit(1.0_rps));

    // Encoder: angle turned over a sampling window gives the angular velocity.
    const auto speed = castle::units::revolutions<double>(120.0) / castle::units::minutes<double>(1.0);
    CASTLE_SAMPLE_CHECK(speed.value() == 120.0);
    const castle::units::degrees_per_second<double> speed_deg = speed;
    CASTLE_SAMPLE_CHECK(speed_deg.value() == 720.0);

    // Dead reckoning: angular velocity times a duration is an angle.
    const auto turned = castle::units::degrees_per_second<double>(45.0) * castle::units::seconds<double>(2.0);
    CASTLE_SAMPLE_CHECK(turned.value() == 90.0);

    // 60 rpm is exactly 360 deg/s for integers; rad/s needs floating point.
    const castle::units::degrees_per_second<int32_t> exact = castle::units::revolutions_per_minute<int32_t>(60);
    CASTLE_SAMPLE_CHECK(exact.value() == 360);
    const castle::units::radians_per_second<double> omega = 60.0_rpm;
    CASTLE_SAMPLE_CHECK(omega.value() > 6.2831 && omega.value() < 6.2832);
    CASTLE_SAMPLE_CHECK(1_rps == 60_rpm);
    return 0;
}

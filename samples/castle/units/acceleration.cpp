#include "sample_support.hpp"

#include "castle/units/acceleration.hpp"
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

// An IMU driver reports milli-g counts; the crash detector works in m/s^2.
castle::units::meters_per_second_squared<double> to_si(castle::units::milli_standard_gravities<int32_t> raw)
{
    return castle::units::meters_per_second_squared<double>(raw);
}

bool is_crash(castle::units::meters_per_second_squared<double> accel)
{
    using namespace castle::units::literals;
    return accel > 4.0_gn || accel < -4.0_gn;
}

} // namespace

int main()
{
    using namespace castle::units::literals;

    CASTLE_SAMPLE_CHECK(near(to_si(castle::units::milli_standard_gravities<int32_t>(1000)).value(), 9.80665));
    CASTLE_SAMPLE_CHECK(!is_crash(to_si(castle::units::milli_standard_gravities<int32_t>(1000))));
    CASTLE_SAMPLE_CHECK(is_crash(to_si(castle::units::milli_standard_gravities<int32_t>(-6000))));

    // Acceleration from two velocity samples.
    const castle::units::meters_per_second<double> v0 = 10.0_mps;
    const castle::units::meters_per_second<double> v1 = 30.0_mps;
    const castle::units::meters_per_second_squared<double> accel = (v1 - v0) / 4.0_s;
    CASTLE_SAMPLE_CHECK(near(accel.value(), 5.0));

    // Velocity gained under constant acceleration.
    const castle::units::meters_per_second<double> gained = accel * 2.0_s;
    CASTLE_SAMPLE_CHECK(near(gained.value(), 10.0));

    CASTLE_SAMPLE_CHECK(1000_mgn == 1_gn);
    CASTLE_SAMPLE_CHECK(near(castle::units::meters_per_second_squared<double>(castle::units::feet_per_second_squared<double>(1.0)).value(), 0.3048));
    return 0;
}

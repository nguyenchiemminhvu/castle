#include "sample_support.hpp"

#include "castle/math/geometry/elevation.hpp"
#include "castle/math/near_equal.hpp"

int main()
{
    // Satellites in the receiver's local ENU frame (x = East, y = North, z = Up), meters.
    const castle::math::point3d<double> receiver(0.0, 0.0, 0.0);
    const castle::math::point3d<double> high_sat(3000000.0, 4000000.0, 5000000.0);
    const castle::math::point3d<double> low_sat(20000000.0, 0.0, 1000000.0);
    const castle::math::point3d<double> zenith_sat(0.0, 0.0, 20200000.0);

    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::elevation_degrees(receiver, high_sat), 45.0, 1.0e-12));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(
        castle::math::elevation(receiver, high_sat), 0.7853981633974483, 1.0e-12));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::elevation_degrees(receiver, zenith_sat), 90.0, 1.0e-12));

    // Typical GNSS elevation mask: ignore satellites below 10 degrees.
    const double mask_deg = 10.0;
    CASTLE_SAMPLE_CHECK(castle::math::elevation_degrees(receiver, high_sat) >= mask_deg);
    CASTLE_SAMPLE_CHECK(castle::math::elevation_degrees(receiver, low_sat) < mask_deg);

    // Targets below the horizon have negative elevation.
    const castle::math::point3d<float> drone(0.0f, 0.0f, 100.0f);
    const castle::math::point3d<float> ground(100.0f, 0.0f, 0.0f);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::elevation_degrees(drone, ground), -45.0f, 1.0e-5f));
    return 0;
}

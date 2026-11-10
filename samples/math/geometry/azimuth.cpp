#include "sample_support.hpp"

#include "castle/math/geometry/azimuth.hpp"
#include "castle/math/near_equal.hpp"

int main()
{
    // Boost.Geometry reference example: (0, 0) -> (1, 1) is north-east.
    const castle::math::point2d<double> p1(0.0, 0.0);
    const castle::math::point2d<double> p2(1.0, 1.0);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::azimuth(p1, p2), 0.785398, 1.0e-6));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::azimuth_degrees(p1, p2), 45.0, 1.0e-12));

    // Compass convention: x = East, y = North, clockwise from North in [0, 360).
    const castle::math::point2d<float> base(10.0f, 10.0f);
    CASTLE_SAMPLE_CHECK(castle::math::azimuth_degrees(base, castle::math::point2d<float>(10.0f, 20.0f)) == 0.0f);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(
        castle::math::azimuth_degrees(base, castle::math::point2d<float>(20.0f, 10.0f)), 90.0f, 1.0e-5f));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(
        castle::math::azimuth_degrees(base, castle::math::point2d<float>(10.0f, 0.0f)), 180.0f, 1.0e-5f));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(
        castle::math::azimuth_degrees(base, castle::math::point2d<float>(0.0f, 10.0f)), 270.0f, 1.0e-5f));

    // GNSS: satellite position in the receiver's local ENU frame (meters).
    // Azimuth uses only the horizontal (east, north) offset.
    const castle::math::point3d<double> receiver(0.0, 0.0, 0.0);
    const castle::math::point3d<double> satellite(-11000000.0, -11000000.0, 15000000.0);
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(castle::math::azimuth_degrees(receiver, satellite), 225.0, 1.0e-12));
    CASTLE_SAMPLE_CHECK(castle::math::near_equal(
        castle::math::azimuth(receiver, satellite), 3.926990816987241, 1.0e-12));

    // Coincident points (or a target straight overhead) have no defined heading: 0 is returned.
    CASTLE_SAMPLE_CHECK(castle::math::azimuth(receiver, castle::math::point3d<double>(0.0, 0.0, 20000000.0)) == 0.0);
    return 0;
}

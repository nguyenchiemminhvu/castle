#include "sample_support.hpp"

#include "castle/error/status.hpp"
#include "castle/math/abs.hpp"
#include "castle_ext/geodetic/distance.hpp"

int main()
{
    using castle::geodetic::geo_point;

    // New York City and Los Angeles: the great-circle distance between them
    // is a commonly published reference value (~3936 km).
    constexpr geo_point<double> new_york(40.7128, -74.0060);
    constexpr geo_point<double> los_angeles(34.0522, -118.2437);

    const double haversine_m = castle::geodetic::haversine_distance(new_york, los_angeles);
    CASTLE_SAMPLE_CHECK(castle::math::abs(haversine_m - 3936000.0) < 15000.0);

    const double bearing_deg = castle::geodetic::initial_bearing(new_york, los_angeles);
    CASTLE_SAMPLE_CHECK(bearing_deg >= 0.0 && bearing_deg < 360.0);

    // Vincenty's ellipsoidal solution is accurate to millimeters and agrees
    // closely with the spherical Haversine approximation above.
    castle::geodetic::vincenty_result<double> inverse{};
    const castle::status inverse_status = castle::geodetic::vincenty_inverse(new_york, los_angeles, inverse);
    CASTLE_SAMPLE_CHECK(castle::succeeded(inverse_status));
    CASTLE_SAMPLE_CHECK(castle::math::abs(inverse.distance - 3936000.0) < 15000.0);

    // The direct problem reconstructs the destination from the inverse result.
    geo_point<double> destination;
    double final_bearing_deg = 0.0;
    const castle::status direct_status = castle::geodetic::vincenty_direct(
        new_york, inverse.initial_bearing_deg, inverse.distance, destination, final_bearing_deg);
    CASTLE_SAMPLE_CHECK(castle::succeeded(direct_status));
    CASTLE_SAMPLE_CHECK(castle::math::abs(destination.latitude() - los_angeles.latitude()) < 1.0e-4);
    CASTLE_SAMPLE_CHECK(castle::math::abs(destination.longitude() - los_angeles.longitude()) < 1.0e-4);

    return 0;
}

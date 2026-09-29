#include "sample_support.hpp"

#include "castle/math/abs.hpp"
#include "castle_ext/geodetic/transform.hpp"

int main()
{
    using castle_ext::geodetic::geo_point;

    // Beijing, expressed in WGS-84 (e.g. as read from a GPS receiver).
    constexpr geo_point<double> beijing_wgs84(39.8946, 116.291);
    CASTLE_SAMPLE_CHECK(castle_ext::geodetic::is_in_gcj02_region(beijing_wgs84));

    // Public Chinese mapping services expect GCJ-02 ("Mars Coordinates").
    const geo_point<double> beijing_gcj02 = castle_ext::geodetic::wgs84_to_gcj02(beijing_wgs84);
    CASTLE_SAMPLE_CHECK(beijing_gcj02.latitude() != beijing_wgs84.latitude());
    CASTLE_SAMPLE_CHECK(beijing_gcj02.longitude() != beijing_wgs84.longitude());
    CASTLE_SAMPLE_CHECK(castle::math::abs(beijing_gcj02.latitude() - beijing_wgs84.latitude()) < 0.01);
    CASTLE_SAMPLE_CHECK(castle::math::abs(beijing_gcj02.longitude() - beijing_wgs84.longitude()) < 0.01);

    // GCJ-02 has no closed-form inverse; gcj02_to_wgs84() numerically refines
    // an initial guess and converges to sub-centimeter residuals.
    const geo_point<double> round_trip = castle_ext::geodetic::gcj02_to_wgs84(beijing_gcj02);
    CASTLE_SAMPLE_CHECK(castle::math::abs(round_trip.latitude() - beijing_wgs84.latitude()) < 1.0e-5);
    CASTLE_SAMPLE_CHECK(castle::math::abs(round_trip.longitude() - beijing_wgs84.longitude()) < 1.0e-5);

    // Points outside mainland China are left unchanged by both directions.
    constexpr geo_point<double> paris(48.8566, 2.3522);
    CASTLE_SAMPLE_CHECK(!castle_ext::geodetic::is_in_gcj02_region(paris));
    CASTLE_SAMPLE_CHECK(castle_ext::geodetic::wgs84_to_gcj02(paris) == paris);
    CASTLE_SAMPLE_CHECK(castle_ext::geodetic::gcj02_to_wgs84(paris) == paris);

    return 0;
}

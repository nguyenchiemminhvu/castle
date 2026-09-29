#include "sample_support.hpp"

#include "castle_ext/geodetic/datum.hpp"

int main()
{
    using castle_ext::geodetic::wgs84_ellipsoid;
    using castle_ext::geodetic::pz90_ellipsoid;
    using castle_ext::geodetic::krasovsky_ellipsoid;
    using castle_ext::geodetic::gcj02_region;

    // WGS-84: the default ellipsoid policy for distance/transform algorithms.
    CASTLE_SAMPLE_CHECK(wgs84_ellipsoid<double>::semi_major_axis() == 6378137.0);
    CASTLE_SAMPLE_CHECK(wgs84_ellipsoid<double>::semi_major_axis() > wgs84_ellipsoid<double>::semi_minor_axis());

    // PZ-90.11: the GLONASS counterpart to WGS-84, close but distinct.
    CASTLE_SAMPLE_CHECK(pz90_ellipsoid<double>::semi_major_axis() == 6378136.0);
    CASTLE_SAMPLE_CHECK(pz90_ellipsoid<double>::semi_major_axis() != wgs84_ellipsoid<double>::semi_major_axis());

    // Krasovsky: only the parameters GCJ-02 needs are exposed.
    CASTLE_SAMPLE_CHECK(krasovsky_ellipsoid<double>::semi_major_axis() == 6378245.0);

    // GCJ-02 obfuscation only applies inside mainland China's bounding box.
    CASTLE_SAMPLE_CHECK(gcj02_region<double>::min_longitude() < gcj02_region<double>::max_longitude());
    CASTLE_SAMPLE_CHECK(gcj02_region<double>::min_latitude() < gcj02_region<double>::max_latitude());

    return 0;
}

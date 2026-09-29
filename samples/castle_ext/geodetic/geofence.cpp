#include "sample_support.hpp"

#include "castle/error/status.hpp"
#include "castle/math/abs.hpp"
#include "castle_ext/geodetic/geofence.hpp"

int main()
{
    using castle_ext::geodetic::geo_point;

    constexpr geo_point<double> hanoi(21.0278, 105.8342);

    // Circle: is Hanoi within 10 km of a depot 5.5 km away?
    constexpr geo_point<double> depot(21.03, 105.85);
    const bool near_depot = castle_ext::geodetic::point_in_circle(hanoi, depot, 10000.0);
    CASTLE_SAMPLE_CHECK(near_depot);

    const castle_ext::geodetic::circular_geofence<double> depot_fence(depot, 10000.0);
    CASTLE_SAMPLE_CHECK(depot_fence.contains(hanoi));

    // Region: a reusable, fixed-capacity polygon geofence around central Hanoi.
    castle_ext::geodetic::polygon_geofence<double, 8U> province;
    CASTLE_SAMPLE_CHECK(castle::succeeded(province.push_back(geo_point<double>(20.9, 105.5))));
    CASTLE_SAMPLE_CHECK(castle::succeeded(province.push_back(geo_point<double>(21.3, 105.5))));
    CASTLE_SAMPLE_CHECK(castle::succeeded(province.push_back(geo_point<double>(21.3, 106.1))));
    CASTLE_SAMPLE_CHECK(castle::succeeded(province.push_back(geo_point<double>(20.9, 106.1))));
    CASTLE_SAMPLE_CHECK(province.contains(hanoi));

    // Array of boundary: an ad-hoc, non-owned list of vertices, e.g. loaded
    // from a boundary table at runtime.
    const geo_point<double> boundary[] = {
        geo_point<double>(20.9, 105.5),
        geo_point<double>(21.3, 105.5),
        geo_point<double>(21.3, 106.1),
        geo_point<double>(20.9, 106.1),
    };
    const bool in_boundary = castle_ext::geodetic::point_in_boundary(hanoi, boundary, 4U);
    CASTLE_SAMPLE_CHECK(in_boundary);

    // A point far outside both fences is correctly rejected.
    constexpr geo_point<double> far_away(0.0, 0.0);
    CASTLE_SAMPLE_CHECK(!depot_fence.contains(far_away));
    CASTLE_SAMPLE_CHECK(!province.contains(far_away));

    // The generic is_inside() dispatcher accepts any fence exposing contains().
    CASTLE_SAMPLE_CHECK(castle_ext::geodetic::is_inside(hanoi, depot_fence));
    CASTLE_SAMPLE_CHECK(castle_ext::geodetic::is_inside(hanoi, province));

    return 0;
}

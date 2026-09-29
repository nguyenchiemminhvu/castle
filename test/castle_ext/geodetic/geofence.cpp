#include <gtest/gtest.h>

#include "castle/container/array_view.hpp"
#include "castle/error/status.hpp"
#include "castle_ext/geodetic/geofence.hpp"

namespace
{

using castle_ext::geodetic::geo_point;
using castle_ext::geodetic::circular_geofence;
using castle_ext::geodetic::polygon_geofence;

// A small square region around central Hanoi, used by several tests below.
CASTLE_CONST geo_point<double> hanoi(21.0278, 105.8342);
CASTLE_CONST geo_point<double> outside_hanoi(0.0, 0.0);

polygon_geofence<double, 8U> make_hanoi_square()
{
    polygon_geofence<double, 8U> region;
    region.push_back(geo_point<double>(20.9, 105.5));
    region.push_back(geo_point<double>(21.3, 105.5));
    region.push_back(geo_point<double>(21.3, 106.1));
    region.push_back(geo_point<double>(20.9, 106.1));
    return region;
}

TEST(GeodeticGeofence, ToPlanePointProjectsLonLat)
{
    constexpr geo_point<double> point(21.0278, 105.8342);
    constexpr castle::math::point2d<double> plane = castle_ext::geodetic::to_plane_point(point);

    static_assert(plane.x() == 105.8342, "x maps to longitude");
    static_assert(plane.y() == 21.0278, "y maps to latitude");

    EXPECT_DOUBLE_EQ(plane.x(), point.longitude());
    EXPECT_DOUBLE_EQ(plane.y(), point.latitude());
}

TEST(GeodeticGeofence, PointInCircleKnownDistance)
{
    // Hanoi and a point ~5.5 km away (roughly matches haversine_distance()).
    CASTLE_CONST geo_point<double> near_point(21.03, 105.85);

    EXPECT_TRUE(castle_ext::geodetic::point_in_circle(hanoi, near_point, 10000.0));
    EXPECT_FALSE(castle_ext::geodetic::point_in_circle(hanoi, near_point, 100.0));
}

TEST(GeodeticGeofence, PointInCircleMarginExtendsRadius)
{
    CASTLE_CONST geo_point<double> near_point(21.03, 105.85);
    CASTLE_CONST double distance_m = castle_ext::geodetic::haversine_distance(hanoi, near_point);

    EXPECT_FALSE(castle_ext::geodetic::point_in_circle(hanoi, near_point, distance_m - 100.0));
    EXPECT_TRUE(castle_ext::geodetic::point_in_circle(hanoi, near_point, distance_m - 100.0, 200.0));
}

TEST(GeodeticGeofence, PointInCircleZeroRadiusOnlyMatchesCenter)
{
    EXPECT_TRUE(castle_ext::geodetic::point_in_circle(hanoi, hanoi, 0.0));
    EXPECT_FALSE(castle_ext::geodetic::point_in_circle(outside_hanoi, hanoi, 0.0));
}

TEST(GeodeticGeofence, CircularGeofenceContainsMatchesFreeFunction)
{
    CASTLE_CONST geo_point<double> near_point(21.03, 105.85);
    CASTLE_CONST circular_geofence<double> fence(near_point, 10000.0);

    EXPECT_TRUE(fence.contains(hanoi));
    EXPECT_EQ(fence.center(), near_point);
    EXPECT_DOUBLE_EQ(fence.radius(), 10000.0);
}

TEST(GeodeticGeofence, CircularGeofenceDefaultConstructedIsZeroRadiusAtOrigin)
{
    constexpr circular_geofence<double> fence;
    static_assert(fence.radius() == 0.0, "default radius");
    EXPECT_EQ(fence.center(), geo_point<double>());
}

TEST(GeodeticGeofence, PolygonGeofencePushBackAndCapacity)
{
    polygon_geofence<double, 4U> region;
    EXPECT_TRUE(region.empty());
    EXPECT_EQ(region.capacity(), 4U);

    EXPECT_EQ(region.push_back(geo_point<double>(20.9, 105.5)), castle::status::ok);
    EXPECT_EQ(region.push_back(geo_point<double>(21.3, 105.5)), castle::status::ok);
    EXPECT_EQ(region.push_back(geo_point<double>(21.3, 106.1)), castle::status::ok);
    EXPECT_EQ(region.push_back(geo_point<double>(20.9, 106.1)), castle::status::ok);
    EXPECT_TRUE(region.full());
    EXPECT_EQ(region.size(), 4U);

    // Capacity exhausted.
    EXPECT_EQ(region.push_back(geo_point<double>(0.0, 0.0)), castle::status::full);

    EXPECT_EQ(region.pop_back(), castle::status::ok);
    EXPECT_EQ(region.size(), 3U);

    region.clear();
    EXPECT_TRUE(region.empty());
    EXPECT_EQ(region.pop_back(), castle::status::empty);
}

TEST(GeodeticGeofence, PolygonGeofenceContainsMatchesRayCasting)
{
    CASTLE_CONST polygon_geofence<double, 8U> region = make_hanoi_square();

    EXPECT_TRUE(region.contains(hanoi));
    EXPECT_FALSE(region.contains(outside_hanoi));
}

TEST(GeodeticGeofence, PolygonGeofenceContainsIsConstexpr)
{
    constexpr auto make_region = []() constexpr
    {
        polygon_geofence<double, 8U> region;
        region.push_back(geo_point<double>(20.9, 105.5));
        region.push_back(geo_point<double>(21.3, 105.5));
        region.push_back(geo_point<double>(21.3, 106.1));
        region.push_back(geo_point<double>(20.9, 106.1));
        return region;
    };
    constexpr auto region = make_region();
    static_assert(region.contains(geo_point<double>(21.0278, 105.8342)), "hanoi is inside the square");
    static_assert(!region.contains(geo_point<double>(0.0, 0.0)), "origin is outside the square");

    SUCCEED();
}

TEST(GeodeticGeofence, PolygonGeofenceFewerThanThreeVerticesIsAlwaysOutside)
{
    polygon_geofence<double, 8U> region;
    region.push_back(geo_point<double>(20.9, 105.5));
    region.push_back(geo_point<double>(21.3, 105.5));

    EXPECT_FALSE(region.contains(hanoi));
}

TEST(GeodeticGeofence, PointInBoundaryRawArrayMatchesPolygonGeofence)
{
    CASTLE_CONST geo_point<double> boundary[] = {
        geo_point<double>(20.9, 105.5),
        geo_point<double>(21.3, 105.5),
        geo_point<double>(21.3, 106.1),
        geo_point<double>(20.9, 106.1),
    };

    EXPECT_TRUE(castle_ext::geodetic::point_in_boundary(hanoi, boundary, 4U));
    EXPECT_FALSE(castle_ext::geodetic::point_in_boundary(outside_hanoi, boundary, 4U));
}

TEST(GeodeticGeofence, PointInBoundaryRejectsNullOrTooFewVertices)
{
    CASTLE_CONST geo_point<double>* null_boundary = nullptr;
    EXPECT_FALSE(castle_ext::geodetic::point_in_boundary(hanoi, null_boundary, 0U));

    CASTLE_CONST geo_point<double> two_points[] = {
        geo_point<double>(20.9, 105.5),
        geo_point<double>(21.3, 106.1),
    };
    EXPECT_FALSE(castle_ext::geodetic::point_in_boundary(hanoi, two_points, 2U));
}

TEST(GeodeticGeofence, PointInBoundaryArrayViewOverload)
{
    CASTLE_CONST geo_point<double> boundary[] = {
        geo_point<double>(20.9, 105.5),
        geo_point<double>(21.3, 105.5),
        geo_point<double>(21.3, 106.1),
        geo_point<double>(20.9, 106.1),
    };
    CASTLE_CONST castle_ext::geodetic::geo_boundary_view<double> view(boundary, 4U);

    EXPECT_TRUE(castle_ext::geodetic::point_in_boundary(hanoi, view));
    EXPECT_FALSE(castle_ext::geodetic::point_in_boundary(outside_hanoi, view));
}

TEST(GeodeticGeofence, PointInBoundaryPolygonGeofenceOverload)
{
    CASTLE_CONST polygon_geofence<double, 8U> region = make_hanoi_square();
    EXPECT_TRUE(castle_ext::geodetic::point_in_boundary(hanoi, region));
    EXPECT_FALSE(castle_ext::geodetic::point_in_boundary(outside_hanoi, region));
}

TEST(GeodeticGeofence, PointOnBoundaryEdgeCountsAsInside)
{
    CASTLE_CONST geo_point<double> boundary[] = {
        geo_point<double>(0.0, 0.0),
        geo_point<double>(0.0, 10.0),
        geo_point<double>(10.0, 10.0),
        geo_point<double>(10.0, 0.0),
    };
    // Midpoint of the bottom edge, exactly on the boundary.
    CASTLE_CONST geo_point<double> edge_point(0.0, 5.0);

    EXPECT_TRUE(castle_ext::geodetic::point_in_boundary(edge_point, boundary, 4U));
}

TEST(GeodeticGeofence, IsInsideDispatchesToCircularAndPolygonFences)
{
    CASTLE_CONST circular_geofence<double> circle(hanoi, 10000.0);
    CASTLE_CONST polygon_geofence<double, 8U> region = make_hanoi_square();

    EXPECT_TRUE(castle_ext::geodetic::is_inside(hanoi, circle));
    EXPECT_TRUE(castle_ext::geodetic::is_inside(hanoi, region));
    EXPECT_FALSE(castle_ext::geodetic::is_inside(outside_hanoi, circle));
    EXPECT_FALSE(castle_ext::geodetic::is_inside(outside_hanoi, region));
}

// ---------------------------------------------------------------------------
// China boundary regression: a coarse, real-world country outline used to
// validate the ray-casting / Jordan Curve Theorem implementation against
// well-known cities that are unambiguously inside or outside it.
// ---------------------------------------------------------------------------

CASTLE_CONST geo_point<double> china_boundary[] = {
    geo_point<double>(21.29206943536157, 107.8973733121041),
    geo_point<double>(17.75823240643058, 108.6066184329415),
    geo_point<double>(18.90784319702411, 125.9130060440320),
    geo_point<double>(31.13811552514781, 128.5628212270744),
    geo_point<double>(33.81966914945097, 124.9370956116744),
    geo_point<double>(39.13082542030939, 124.0232085095437),
    geo_point<double>(41.52880010079262, 131.9132668674246),
    geo_point<double>(48.37896049292777, 135.5923390136213),
    geo_point<double>(53.24011751868090, 127.4948282640856),
    geo_point<double>(54.51500942013367, 121.8292048750092),
    geo_point<double>(45.45218452643208, 111.8942585488605),
    geo_point<double>(42.94257243019249, 109.8113155785911),
    geo_point<double>(43.08617185473222, 96.84425525666065),
    geo_point<double>(44.38961290887570, 96.28206660835602),
    geo_point<double>(45.53328504242598, 91.46062858375311),
    geo_point<double>(47.77586155528959, 91.42377680197887),
    geo_point<double>(49.58468867471388, 87.00984935333790),
    geo_point<double>(47.46079695870164, 82.70286735723320),
    geo_point<double>(45.26213655741632, 79.66597344728299),
    geo_point<double>(42.68018245032157, 79.86445429887883),
    geo_point<double>(40.86161040366930, 74.42057874144240),
    geo_point<double>(39.43855188692437, 73.31406885411333),
    geo_point<double>(36.40141615382820, 72.60879732558129),
    geo_point<double>(33.26550801710336, 73.94200791623562),
    geo_point<double>(27.77975358751668, 83.90999314707722),
    geo_point<double>(27.11538043714165, 90.47821944445789),
    geo_point<double>(27.07390912431352, 97.61237960254650),
    geo_point<double>(23.65001692519949, 96.81733635768252),
    geo_point<double>(20.58000957873124, 101.0014362766858),
    geo_point<double>(22.99096565142264, 105.3101117918342),
};
CASTLE_CONST castle::size_type china_boundary_count =
    sizeof(china_boundary) / sizeof(china_boundary[0]);

TEST(GeodeticGeofence, ChinaBoundaryContainsMajorCities)
{
    CASTLE_CONST geo_point<double> beijing(39.9042, 116.4074);
    CASTLE_CONST geo_point<double> shanghai(31.2304, 121.4737);
    CASTLE_CONST geo_point<double> guangzhou(23.1291, 113.2644);
    CASTLE_CONST geo_point<double> xian(34.3416, 108.9398);
    CASTLE_CONST geo_point<double> urumqi(43.8256, 87.6168);
    CASTLE_CONST geo_point<double> lhasa(29.6520, 91.1721);

    EXPECT_TRUE(castle_ext::geodetic::point_in_boundary(beijing, china_boundary, china_boundary_count));
    EXPECT_TRUE(castle_ext::geodetic::point_in_boundary(shanghai, china_boundary, china_boundary_count));
    EXPECT_TRUE(castle_ext::geodetic::point_in_boundary(guangzhou, china_boundary, china_boundary_count));
    EXPECT_TRUE(castle_ext::geodetic::point_in_boundary(xian, china_boundary, china_boundary_count));
    EXPECT_TRUE(castle_ext::geodetic::point_in_boundary(urumqi, china_boundary, china_boundary_count));
    EXPECT_TRUE(castle_ext::geodetic::point_in_boundary(lhasa, china_boundary, china_boundary_count));
}

TEST(GeodeticGeofence, ChinaBoundaryExcludesNeighboringCapitals)
{
    CASTLE_CONST geo_point<double> tokyo(35.6895, 139.6917);
    CASTLE_CONST geo_point<double> hanoi_capital(21.0278, 105.8342);
    CASTLE_CONST geo_point<double> new_delhi(28.6139, 77.2090);
    CASTLE_CONST geo_point<double> seoul(37.5665, 126.9780);
    CASTLE_CONST geo_point<double> moscow(55.7558, 37.6173);

    EXPECT_FALSE(castle_ext::geodetic::point_in_boundary(tokyo, china_boundary, china_boundary_count));
    EXPECT_FALSE(castle_ext::geodetic::point_in_boundary(hanoi_capital, china_boundary, china_boundary_count));
    EXPECT_FALSE(castle_ext::geodetic::point_in_boundary(new_delhi, china_boundary, china_boundary_count));
    EXPECT_FALSE(castle_ext::geodetic::point_in_boundary(seoul, china_boundary, china_boundary_count));
    EXPECT_FALSE(castle_ext::geodetic::point_in_boundary(moscow, china_boundary, china_boundary_count));
}

TEST(GeodeticGeofence, ChinaBoundaryArrayViewOverloadAgreesWithRawArray)
{
    CASTLE_CONST castle_ext::geodetic::geo_boundary_view<double> view(china_boundary, china_boundary_count);
    CASTLE_CONST geo_point<double> beijing(39.9042, 116.4074);

    EXPECT_EQ(castle_ext::geodetic::point_in_boundary(beijing, view),
              castle_ext::geodetic::point_in_boundary(beijing, china_boundary, china_boundary_count));
}

} // namespace

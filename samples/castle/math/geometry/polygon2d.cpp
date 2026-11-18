#include "sample_support.hpp"

#include "castle/math/geometry/polygon2d.hpp"

int main()
{
    castle::math::polygon2d<int, 4> polygon;
    CASTLE_SAMPLE_CHECK(polygon.empty());
    CASTLE_SAMPLE_CHECK(polygon.capacity() == 4U);

    CASTLE_SAMPLE_CHECK(polygon.push_back(castle::math::point2d<int>(0, 0)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(polygon.push_back(castle::math::point2d<int>(4, 0)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(polygon.push_back(castle::math::point2d<int>(0, 3)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(polygon.size() == 3U);
    CASTLE_SAMPLE_CHECK(polygon.signed_area2() == 12);
    CASTLE_SAMPLE_CHECK(!polygon.clockwise());

    CASTLE_SAMPLE_CHECK(polygon.push_back(castle::math::point2d<int>(1, 1)) == castle::status::ok);
    CASTLE_SAMPLE_CHECK(polygon.full());
    CASTLE_SAMPLE_CHECK(polygon.pop_back() == castle::status::ok);
    CASTLE_SAMPLE_CHECK(polygon.size() == 3U);

    polygon.clear();
    CASTLE_SAMPLE_CHECK(polygon.empty());
    return 0;
}

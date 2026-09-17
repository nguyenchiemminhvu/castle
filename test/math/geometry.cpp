
#include <gtest/gtest.h>

#include "castle/math/geometry.h"

namespace
{

TEST(GeometryUmbrellaRoot, PublicEntryPoint)
{
    const castle::math::point2d<int> point(1, 2);
    const castle::math::vector2d<int> vector(3, 4);

    EXPECT_EQ(point + vector, castle::math::point2d<int>(4, 6));
    EXPECT_EQ(castle::math::squared_distance(point, point), 0);
}

} // namespace

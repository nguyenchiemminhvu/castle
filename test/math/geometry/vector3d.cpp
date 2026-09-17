
#include <gtest/gtest.h>

#include "castle/math/geometry/vector3d.h"

namespace
{

TEST(GeometryVector3d, ConstructionLengthAndProducts)
{
    constexpr castle::math::vector3d<int> value(1, 2, 3);
    static_assert(value.x() == 1 && value.y() == 2 && value.z() == 3, "vector values");
    static_assert(value.squared_length() == 14, "squared length");
    static_assert(value.dot(castle::math::vector3d<int>(2, 0, 1)) == 5, "dot");
    static_assert(value.cross(castle::math::vector3d<int>(2, 0, 1)) ==
                      castle::math::vector3d<int>(2, 5, -4), "cross");

    EXPECT_NEAR(castle::math::vector3d<double>(1.0, 2.0, 2.0).length(), 3.0, 1e-12);
    EXPECT_FALSE(value.degenerate());
    EXPECT_TRUE(castle::math::vector3d<int>().degenerate());
}

TEST(GeometryVector3d, UnaryBinaryAndScalarOperations)
{
    const castle::math::vector3d<int> a(2, 3, 4);
    const castle::math::vector3d<int> b(1, -1, 2);

    EXPECT_EQ(+a, a);
    EXPECT_EQ(-a, castle::math::vector3d<int>(-2, -3, -4));
    EXPECT_EQ(a + b, castle::math::vector3d<int>(3, 2, 6));
    EXPECT_EQ(a - b, castle::math::vector3d<int>(1, 4, 2));
    EXPECT_EQ(a * 2, castle::math::vector3d<int>(4, 6, 8));
    EXPECT_EQ(2 * a, castle::math::vector3d<int>(4, 6, 8));
    EXPECT_EQ(a / 2, castle::math::vector3d<int>(1, 1, 2));

    castle::math::vector3d<int> value = a;
    value += b;
    EXPECT_EQ(value, castle::math::vector3d<int>(3, 2, 6));
    value -= b;
    EXPECT_EQ(value, a);

    EXPECT_TRUE(a == castle::math::vector3d<int>(2, 3, 4));
    EXPECT_FALSE(a != castle::math::vector3d<int>(2, 3, 4));
    EXPECT_TRUE(a != b);
}

TEST(GeometryVector3d, FreeProductsAndPointOperations)
{
    const castle::math::vector3d<int> a(1, 0, 0);
    const castle::math::vector3d<int> b(0, 1, 0);
    const castle::math::vector3d<int> c(0, 0, 1);
    const castle::math::point3d<int> p(5, 6, 7);
    const castle::math::point3d<int> q(2, 1, 3);

    EXPECT_EQ(castle::math::dot(a, b), 0);
    EXPECT_EQ(castle::math::cross(a, b), c);
    EXPECT_EQ(castle::math::scalar_triple_product(a, b, c), 1);
    EXPECT_EQ(p - q, castle::math::vector3d<int>(3, 5, 4));
    EXPECT_EQ(p + a, castle::math::point3d<int>(6, 6, 7));
    EXPECT_EQ(p - a, castle::math::point3d<int>(4, 6, 7));
}

TEST(GeometryVector3d, Normalization)
{
    const castle::math::vector3d<float> value(2.0f, 0.0f, 0.0f);
    const castle::math::vector3d<float> unit = value.normalized();

    EXPECT_NEAR(unit.x(), 1.0f, 1e-5f);
    EXPECT_NEAR(unit.y(), 0.0f, 1e-5f);
    EXPECT_NEAR(unit.z(), 0.0f, 1e-5f);
}

} // namespace


#include <gtest/gtest.h>

#include "castle/math/geometry/vector2d.h"

namespace
{

TEST(GeometryVector2d, ConstructionLengthAndProducts)
{
    constexpr castle::math::vector2d<int> value(3, 4);
    static_assert(value.x() == 3 && value.y() == 4, "vector values");
    static_assert(value.squared_length() == 25, "squared length");
    static_assert(value.dot(castle::math::vector2d<int>(2, 1)) == 10, "dot");
    static_assert(value.cross(castle::math::vector2d<int>(2, 1)) == -5, "cross");

    EXPECT_EQ(value.x(), 3);
    EXPECT_EQ(value.y(), 4);
    EXPECT_EQ(value.squared_length(), 25);
    EXPECT_DOUBLE_EQ(castle::math::vector2d<double>(3.0, 4.0).length(), 5.0);
}

TEST(GeometryVector2d, UnaryBinaryAndScalarOperations)
{
    const castle::math::vector2d<int> a(2, 3);
    const castle::math::vector2d<int> b(4, -1);

    EXPECT_EQ((+a), a);
    EXPECT_EQ((-a), castle::math::vector2d<int>(-2, -3));
    EXPECT_EQ(a + b, castle::math::vector2d<int>(6, 2));
    EXPECT_EQ(a - b, castle::math::vector2d<int>(-2, 4));
    EXPECT_EQ(a * 3, castle::math::vector2d<int>(6, 9));
    EXPECT_EQ(3 * a, castle::math::vector2d<int>(6, 9));
    EXPECT_EQ(a / 2, castle::math::vector2d<int>(1, 1));

    castle::math::vector2d<int> value = a;
    value += b;
    EXPECT_EQ(value, castle::math::vector2d<int>(6, 2));
    value -= b;
    EXPECT_EQ(value, a);

    EXPECT_EQ(a.perpendicular(), castle::math::vector2d<int>(-3, 2));
    EXPECT_TRUE(a == castle::math::vector2d<int>(2, 3));
    EXPECT_FALSE(a != castle::math::vector2d<int>(2, 3));
    EXPECT_TRUE(a != b);
}

TEST(GeometryVector2d, FreeProductsAndPointOperations)
{
    const castle::math::vector2d<int> a(1, 2);
    const castle::math::vector2d<int> b(3, 4);
    const castle::math::point2d<int> p(5, 6);
    const castle::math::point2d<int> q(2, 1);

    EXPECT_EQ(castle::math::dot(a, b), 11);
    EXPECT_EQ(castle::math::cross(a, b), -2);
    EXPECT_EQ(p - q, castle::math::vector2d<int>(3, 5));
    EXPECT_EQ(p + a, castle::math::point2d<int>(6, 8));
    EXPECT_EQ(p - a, castle::math::point2d<int>(4, 4));
}

TEST(GeometryVector2d, Normalization)
{
    const castle::math::vector2d<float> value(3.0f, 4.0f);
    const castle::math::vector2d<float> unit = value.normalized();

    EXPECT_NEAR(unit.x(), 0.6f, 1e-5f);
    EXPECT_NEAR(unit.y(), 0.8f, 1e-5f);
}

} // namespace

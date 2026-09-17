
#include <gtest/gtest.h>

#include "castle/math/saturating.h"

#include <cstdint>

namespace
{

TEST(MathSaturating, UnsignedAdd)
{
    EXPECT_EQ(castle::math::saturating_add<uint8_t>(10U, 20U), 30U);
    EXPECT_EQ(castle::math::saturating_add<uint8_t>(250U, 10U), 255U);
    static_assert(castle::math::saturating_add<uint8_t>(250U, 10U) == 255U, "unsigned add");
}

TEST(MathSaturating, UnsignedSub)
{
    EXPECT_EQ(castle::math::saturating_sub<uint8_t>(20U, 10U), 10U);
    EXPECT_EQ(castle::math::saturating_sub<uint8_t>(10U, 20U), 0U);
    EXPECT_EQ(castle::math::saturating_sub<uint8_t>(10U, 10U), 0U);
}

TEST(MathSaturating, SignedAdd)
{
    EXPECT_EQ(castle::math::saturating_add<int8_t>(10, 20), 30);
    EXPECT_EQ(castle::math::saturating_add<int8_t>(120, 20), 127);
    EXPECT_EQ(castle::math::saturating_add<int8_t>(-120, -20), -128);
    EXPECT_EQ(castle::math::saturating_add<int8_t>(120, -20), 100);
    EXPECT_EQ(castle::math::saturating_add<int8_t>(-120, 20), -100);
    EXPECT_EQ(castle::math::saturating_add<int8_t>(10, 0), 10);
}

TEST(MathSaturating, SignedSub)
{
    EXPECT_EQ(castle::math::saturating_sub<int8_t>(30, 10), 20);
    EXPECT_EQ(castle::math::saturating_sub<int8_t>(-120, 20), -128);
    EXPECT_EQ(castle::math::saturating_sub<int8_t>(120, -20), 127);
    EXPECT_EQ(castle::math::saturating_sub<int8_t>(-120, -20), -100);
    EXPECT_EQ(castle::math::saturating_sub<int8_t>(10, 0), 10);
}

} // namespace

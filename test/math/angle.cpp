
#include <gtest/gtest.h>

#include "castle/math/angle.h"

namespace
{

TEST(MathAngle, ConstantsAndConversions)
{
    EXPECT_NEAR(castle::math::pi<float>(), 3.1415927f, 1e-6f);
    EXPECT_NEAR(castle::math::tau<double>(), 6.283185307179586, 1e-12);
    EXPECT_NEAR(castle::math::degrees_to_radians(180.0), castle::math::pi<double>(), 1e-12);
    EXPECT_NEAR(castle::math::radians_to_degrees(castle::math::pi<float>()), 180.0f, 1e-5f);

    static_assert(castle::math::pi<float>() > 3.14f, "pi");
    static_assert(castle::math::tau<float>() > 6.28f, "tau");
    static_assert(castle::math::degrees_to_radians(180.0) > 3.14, "degrees_to_radians");
    static_assert(castle::math::radians_to_degrees(3.141592653589793) > 179.9, "radians_to_degrees");
}

TEST(MathAngle, QuarterAndSignedAngles)
{
    EXPECT_NEAR(castle::math::degrees_to_radians(90.0), 1.5707963267948966, 1e-12);
    EXPECT_NEAR(castle::math::degrees_to_radians(-90.0), -1.5707963267948966, 1e-12);
    EXPECT_NEAR(castle::math::radians_to_degrees(-castle::math::pi<double>()), -180.0, 1e-12);
}

} // namespace

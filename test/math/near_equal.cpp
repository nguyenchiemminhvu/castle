
#include <gtest/gtest.h>

#include "castle/math/near_equal.h"

namespace
{

TEST(MathNearEqual, ExactAndAbsoluteScale)
{
    EXPECT_TRUE(castle::math::near_equal(1.0, 1.0, 0.0));
    EXPECT_TRUE(castle::math::near_equal(1.0, 1.000001, 1e-5));
    EXPECT_FALSE(castle::math::near_equal(1.0, 1.1, 1e-5));
    static_assert(castle::math::near_equal(1.0, 1.0, 0.0), "near_equal");
}

TEST(MathNearEqual, RelativeScaleForLargeValues)
{
    EXPECT_TRUE(castle::math::near_equal(1000000.0, 1000000.5, 1e-6));
    EXPECT_FALSE(castle::math::near_equal(1000000.0, 1000002.0, 1e-6));
}

} // namespace

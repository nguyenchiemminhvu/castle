
#include <gtest/gtest.h>

#include "castle/math/square.h"

namespace
{

TEST(MathSquare, IntegralAndFloating)
{
    EXPECT_EQ(castle::math::square(0), 0);
    EXPECT_EQ(castle::math::square(-5), 25);
    EXPECT_DOUBLE_EQ(castle::math::square(2.5), 6.25);
    static_assert(castle::math::square(7) == 49, "square");
}

} // namespace

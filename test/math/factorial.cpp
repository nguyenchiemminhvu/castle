#include <gtest/gtest.h>

#include "castle/math/factorial.hpp"

#include <stdint.h>

namespace
{

TEST(Factorial, Factorial)
{
    EXPECT_EQ(castle::math::factorial<0U>::value, 1U);
    EXPECT_EQ(castle::math::factorial<1U>::value, 1U);
    EXPECT_EQ(castle::math::factorial<5U>::value, 120U);
    EXPECT_EQ(castle::math::factorial_v(10U), 3628800U);
}

} // namespace
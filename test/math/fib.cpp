#include <gtest/gtest.h>

#include "castle/math/fib.h"

namespace
{

// ---------------------------------------------------------------------------
// fib<N>() - compile-time template form
// ---------------------------------------------------------------------------
TEST(MathFib, TemplateBaseCases)
{
    static_assert(castle::math::fib<0>() == 0u, "fib(0) == 0");
    static_assert(castle::math::fib<1>() == 1u, "fib(1) == 1");
    EXPECT_EQ(castle::math::fib<0>(), 0u);
    EXPECT_EQ(castle::math::fib<1>(), 1u);
}

TEST(MathFib, TemplateSequence)
{
    static_assert(castle::math::fib<2>() == 1u, "");
    static_assert(castle::math::fib<3>() == 2u, "");
    static_assert(castle::math::fib<4>() == 3u, "");
    static_assert(castle::math::fib<5>() == 5u, "");
    static_assert(castle::math::fib<10>() == 55u, "");
    static_assert(castle::math::fib<20>() == 6765u, "");
    EXPECT_EQ(castle::math::fib<10>(), 55u);
    EXPECT_EQ(castle::math::fib<20>(), 6765u);
}

TEST(MathFib, TemplateLarge)
{
    // fib(90) = 2880067194370816120 fits in 64-bit size_type.
    static_assert(castle::math::fib<90>() == 2880067194370816120ull, "");
    EXPECT_EQ(castle::math::fib<90>(), 2880067194370816120ull);
}

// ---------------------------------------------------------------------------
// fib(n) - runtime form
// ---------------------------------------------------------------------------
TEST(MathFib, RuntimeBaseCases)
{
    EXPECT_EQ(castle::math::fib(0u), 0u);
    EXPECT_EQ(castle::math::fib(1u), 1u);
}

TEST(MathFib, RuntimeSequence)
{
    EXPECT_EQ(castle::math::fib(2u), 1u);
    EXPECT_EQ(castle::math::fib(3u), 2u);
    EXPECT_EQ(castle::math::fib(4u), 3u);
    EXPECT_EQ(castle::math::fib(5u), 5u);
    EXPECT_EQ(castle::math::fib(6u), 8u);
    EXPECT_EQ(castle::math::fib(7u), 13u);
    EXPECT_EQ(castle::math::fib(10u), 55u);
    EXPECT_EQ(castle::math::fib(20u), 6765u);
}

TEST(MathFib, RuntimeMatchesTemplate)
{
    EXPECT_EQ(castle::math::fib(15u), castle::math::fib<15>());
    EXPECT_EQ(castle::math::fib(30u), castle::math::fib<30>());
}

TEST(MathFib, RuntimeConstexpr)
{
    static_assert(castle::math::fib(12u) == 144u, "");
    SUCCEED();
}

} // namespace

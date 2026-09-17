#include <gtest/gtest.h>

#include "castle/math/gcd.h"

#include <stdint.h>

namespace
{

// ---------------------------------------------------------------------------
// gcd<A, B>::value - compile-time Euclidean algorithm
// ---------------------------------------------------------------------------
TEST(MathGcd, CommonCases)
{
    static_assert(castle::math::gcd<48, 36>::value == 12, "");
    static_assert(castle::math::gcd<54, 24>::value == 6, "");
    static_assert(castle::math::gcd<12, 8>::value == 4, "");
    static_assert(castle::math::gcd<100, 10>::value == 10, "");
    SUCCEED();
}

TEST(MathGcd, Coprime)
{
    static_assert(castle::math::gcd<17, 5>::value == 1, "");
    static_assert(castle::math::gcd<13, 7>::value == 1, "");
    static_assert(castle::math::gcd<9, 28>::value == 1, "");
    SUCCEED();
}

TEST(MathGcd, WithZero)
{
    // gcd(A, 0) == A via the specialization.
    static_assert(castle::math::gcd<7, 0>::value == 7, "");
    static_assert(castle::math::gcd<1, 0>::value == 1, "");
    // gcd(0, B) recurses to gcd(B, 0) == B.
    static_assert(castle::math::gcd<0, 9>::value == 9, "");
    SUCCEED();
}

TEST(MathGcd, Identity)
{
    static_assert(castle::math::gcd<5, 5>::value == 5, "");
    static_assert(castle::math::gcd<1, 1>::value == 1, "");
    SUCCEED();
}

TEST(MathGcd, Divisible)
{
    static_assert(castle::math::gcd<20, 5>::value == 5, "");
    static_assert(castle::math::gcd<81, 27>::value == 27, "");
    SUCCEED();
}

TEST(MathGcd, RuntimeReadable)
{
    // The value is usable as an ordinary constant at runtime too.
    const intmax_t g = castle::math::gcd<48, 36>::value;
    EXPECT_EQ(g, 12);
}

} // namespace

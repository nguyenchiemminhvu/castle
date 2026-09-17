#include <gtest/gtest.h>

#include "castle/math/logarithm.h"

namespace
{

// ---------------------------------------------------------------------------
// logarithm<Value, Base>::value - floor of the integer logarithm
// ---------------------------------------------------------------------------
TEST(MathLogarithm, Base2)
{
    static_assert(castle::math::logarithm<1, 2>::value == 0u, "");
    static_assert(castle::math::logarithm<2, 2>::value == 1u, "");
    static_assert(castle::math::logarithm<3, 2>::value == 1u, "");
    static_assert(castle::math::logarithm<4, 2>::value == 2u, "");
    static_assert(castle::math::logarithm<8, 2>::value == 3u, "");
    static_assert(castle::math::logarithm<15, 2>::value == 3u, "");
    static_assert(castle::math::logarithm<16, 2>::value == 4u, "");
    SUCCEED();
}

TEST(MathLogarithm, Base10)
{
    static_assert(castle::math::logarithm<1, 10>::value == 0u, "");
    static_assert(castle::math::logarithm<9, 10>::value == 0u, "");
    static_assert(castle::math::logarithm<10, 10>::value == 1u, "");
    static_assert(castle::math::logarithm<99, 10>::value == 1u, "");
    static_assert(castle::math::logarithm<100, 10>::value == 2u, "");
    static_assert(castle::math::logarithm<1000, 10>::value == 3u, "");
    SUCCEED();
}

TEST(MathLogarithm, ZeroSpecialization)
{
    static_assert(castle::math::logarithm<0, 2>::value == 0u, "");
    static_assert(castle::math::logarithm<0, 10>::value == 0u, "");
    SUCCEED();
}

TEST(MathLogarithm, ArbitraryBase)
{
    static_assert(castle::math::logarithm<27, 3>::value == 3u, "");
    static_assert(castle::math::logarithm<26, 3>::value == 2u, "");
    static_assert(castle::math::logarithm<625, 5>::value == 4u, "");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// log2 / log10 convenience wrappers
// ---------------------------------------------------------------------------
TEST(MathLogarithm, Log2)
{
    static_assert(castle::math::log2<1>::value == 0u, "");
    static_assert(castle::math::log2<1024>::value == 10u, "");
    static_assert(castle::math::log2<1023>::value == 9u, "");
    static_assert(castle::math::log2<65536>::value == 16u, "");
    SUCCEED();
}

TEST(MathLogarithm, Log10)
{
    static_assert(castle::math::log10<1>::value == 0u, "");
    static_assert(castle::math::log10<100>::value == 2u, "");
    static_assert(castle::math::log10<999999>::value == 5u, "");
    static_assert(castle::math::log10<1000000>::value == 6u, "");
    SUCCEED();
}

TEST(MathLogarithm, RuntimeReadable)
{
    EXPECT_EQ((castle::math::logarithm<1000, 10>::value), 3u);
    EXPECT_EQ(castle::math::log2<256>::value, 8u);
}

} // namespace

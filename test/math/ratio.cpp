#include <gtest/gtest.h>

#include "castle/math/ratio.h"

#include <stdint.h>

namespace
{

// ---------------------------------------------------------------------------
// ratio<N, D> - automatic reduction and sign normalisation
// ---------------------------------------------------------------------------
TEST(MathRatio, Reduction)
{
    static_assert(castle::math::ratio<2, 4>::num == 1, "");
    static_assert(castle::math::ratio<2, 4>::den == 2, "");
    static_assert(castle::math::ratio<10, 15>::num == 2, "");
    static_assert(castle::math::ratio<10, 15>::den == 3, "");
    SUCCEED();
}

TEST(MathRatio, DefaultDenominator)
{
    static_assert(castle::math::ratio<5>::num == 5, "");
    static_assert(castle::math::ratio<5>::den == 1, "");
    SUCCEED();
}

TEST(MathRatio, SignNormalisation)
{
    // Sign always migrates to the numerator; denominator stays positive.
    static_assert(castle::math::ratio<1, -2>::num == -1, "");
    static_assert(castle::math::ratio<1, -2>::den == 2, "");
    static_assert(castle::math::ratio<-1, -2>::num == 1, "");
    static_assert(castle::math::ratio<-1, -2>::den == 2, "");
    static_assert(castle::math::ratio<-1, 2>::num == -1, "");
    static_assert(castle::math::ratio<-1, 2>::den == 2, "");
    SUCCEED();
}

TEST(MathRatio, Zero)
{
    static_assert(castle::math::ratio<0, 5>::num == 0, "");
    static_assert(castle::math::ratio<0, 5>::den == 1, "");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Arithmetic
// ---------------------------------------------------------------------------
TEST(MathRatio, Add)
{
    using r = castle::math::ratio_add_t<castle::math::ratio<1, 2>,
                                        castle::math::ratio<1, 3>>;
    static_assert(r::num == 5, "");
    static_assert(r::den == 6, "");
    SUCCEED();
}

TEST(MathRatio, Subtract)
{
    using r = castle::math::ratio_subtract_t<castle::math::ratio<3, 4>,
                                             castle::math::ratio<1, 4>>;
    static_assert(r::num == 1, "");
    static_assert(r::den == 2, "");
    SUCCEED();
}

TEST(MathRatio, Multiply)
{
    using r = castle::math::ratio_multiply_t<castle::math::ratio<2, 3>,
                                             castle::math::ratio<3, 4>>;
    static_assert(r::num == 1, "");
    static_assert(r::den == 2, "");
    SUCCEED();
}

TEST(MathRatio, Divide)
{
    using r = castle::math::ratio_divide_t<castle::math::ratio<1, 2>,
                                           castle::math::ratio<3, 4>>;
    static_assert(r::num == 2, "");
    static_assert(r::den == 3, "");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Comparisons
// ---------------------------------------------------------------------------
TEST(MathRatio, Equal)
{
    static_assert(castle::math::ratio_equal<castle::math::ratio<1, 2>,
                                            castle::math::ratio<2, 4>>::value, "");
    static_assert(castle::math::ratio_not_equal<castle::math::ratio<1, 2>,
                                                castle::math::ratio<1, 3>>::value, "");
    SUCCEED();
}

TEST(MathRatio, Ordering)
{
    static_assert(castle::math::ratio_less<castle::math::ratio<1, 2>,
                                           castle::math::ratio<2, 3>>::value, "");
    static_assert(castle::math::ratio_greater<castle::math::ratio<2, 3>,
                                              castle::math::ratio<1, 2>>::value, "");
    static_assert(castle::math::ratio_less_equal<castle::math::ratio<1, 2>,
                                                 castle::math::ratio<1, 2>>::value, "");
    static_assert(castle::math::ratio_greater_equal<castle::math::ratio<1, 2>,
                                                    castle::math::ratio<1, 2>>::value, "");
    static_assert(!castle::math::ratio_less<castle::math::ratio<2, 3>,
                                            castle::math::ratio<1, 2>>::value, "");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// Convenience SI / chrono ratios at castle:: scope
// ---------------------------------------------------------------------------
TEST(MathRatio, ConvenienceAliases)
{
    static_assert(castle::milli::num == 1, "");
    static_assert(castle::milli::den == 1000, "");
    static_assert(castle::kilo::num == 1000, "");
    static_assert(castle::kilo::den == 1, "");
    static_assert(castle::minutes::num == 60, "");
    static_assert(castle::hours::num == 3600, "");
    static_assert(castle::days::num == 86400, "");
    SUCCEED();
}

} // namespace

#include <gtest/gtest.h>

#include "castle/core/constants.hpp"

namespace
{

TEST(ConstantsTest, Pi)
{
    EXPECT_DOUBLE_EQ(castle::math::pi<double>(), 3.14159265358979323846);
}

TEST(ConstantsTest, Tau)
{
    EXPECT_DOUBLE_EQ(castle::math::tau<double>(), 6.28318530717958647692);
}

TEST(ConstantsTest, DegreesToRadians)
{
    EXPECT_DOUBLE_EQ(castle::math::degrees_to_radians<double>(180), 3.14159265358979323846);
}

TEST(ConstantsTest, RadiansToDegrees)
{
    EXPECT_DOUBLE_EQ(castle::math::radians_to_degrees<double>(3.14159265358979323846), 180);
}

TEST(ConstantsTest, E)
{
    EXPECT_DOUBLE_EQ(castle::math::e<double>(), 2.71828182845904523536);
}

TEST(ConstantsTest, GoldenRatio)
{
    EXPECT_DOUBLE_EQ(castle::math::golden_ratio<double>(), 1.61803398874989484820);
}

TEST(ConstantsTest, Log2E)
{
    EXPECT_DOUBLE_EQ(castle::math::log2_e<double>(), 1.44269504088896340736);
}

TEST(ConstantsTest, Log10E)
{
    EXPECT_DOUBLE_EQ(castle::math::log10_e<double>(), 0.43429448190325182765);
}

TEST(ConstantsTest, Ln2)
{
    EXPECT_DOUBLE_EQ(castle::math::ln_2<double>(), 0.69314718055994530942);
}

TEST(ConstantsTest, Ln10)
{
    EXPECT_DOUBLE_EQ(castle::math::ln_10<double>(), 2.30258509299404568402);
}

TEST(ConstantsTest, Sqrt2)
{
    EXPECT_DOUBLE_EQ(castle::math::sqrt_2<double>(), 1.41421356237309504880);
}

TEST(ConstantsTest, Sqrt3)
{
    EXPECT_DOUBLE_EQ(castle::math::sqrt_3<double>(), 1.73205080756887729352);
}

TEST(ConstantsTest, InvSqrt2)
{
    EXPECT_DOUBLE_EQ(castle::math::inv_sqrt_2<double>(), 0.70710678118654752440);
}

TEST(ConstantsTest, Phi)
{
    EXPECT_DOUBLE_EQ(castle::math::golden_ratio<double>(), 1.61803398874989484820);
}

} // namespace
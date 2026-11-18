
#include <gtest/gtest.h>

#include "castle/math/exponential.hpp"

namespace
{

TEST(MathExponential, ExpFloat)
{
    EXPECT_NEAR(castle::math::exp(0.0F), 1.0F, 1.0e-6F);
    EXPECT_NEAR(castle::math::exp(1.0F), 2.7182818F, 1.0e-5F);
    EXPECT_NEAR(castle::math::exp(-1.0F), 0.36787944F, 1.0e-6F);
    EXPECT_NEAR(castle::math::exp(2.0F), 7.3890561F, 1.0e-4F);
    EXPECT_TRUE(castle::math::exp(-200.0F) == 0.0F);
    EXPECT_TRUE(castle::math::exp(200.0F) > 1.0e30F);
}

TEST(MathExponential, ExpDouble)
{
    EXPECT_NEAR(castle::math::exp(0.0), 1.0, 1.0e-12);
    EXPECT_NEAR(castle::math::exp(1.0), 2.718281828459045, 1.0e-12);
    EXPECT_NEAR(castle::math::exp(-1.0), 0.36787944117144233, 1.0e-12);
    EXPECT_NEAR(castle::math::exp(10.0), 22026.465794806718, 1.0e-8);
    EXPECT_TRUE(castle::math::exp(-1000.0) == 0.0);
}

TEST(MathExponential, ExpLongDouble)
{
    EXPECT_NEAR(static_cast<double>(castle::math::exp(0.0L)), 1.0, 1.0e-12);
    EXPECT_NEAR(static_cast<double>(castle::math::exp(1.0L)), 2.718281828459045, 1.0e-12);
    EXPECT_NEAR(static_cast<double>(castle::math::exp(-1.0L)), 0.36787944117144233, 1.0e-12);
    EXPECT_NEAR(static_cast<double>(castle::math::exp(5.0L)), 148.4131591025766, 1.0e-9);
}

TEST(MathExponential, LogFloat)
{
    EXPECT_NEAR(castle::math::log(1.0F), 0.0F, 1.0e-6F);
    EXPECT_NEAR(castle::math::log(2.7182818F), 1.0F, 1.0e-6F);
    EXPECT_NEAR(castle::math::log(10.0F), 2.3025851F, 1.0e-6F);
    EXPECT_NEAR(castle::math::log(0.5F), -0.6931472F, 1.0e-6F);
    EXPECT_TRUE(castle::math::log(0.0F) < -1.0e30F);
    const float negative = castle::math::log(-1.0F);
    EXPECT_TRUE(negative != negative);
}

TEST(MathExponential, LogDouble)
{
    EXPECT_NEAR(castle::math::log(1.0), 0.0, 1.0e-12);
    EXPECT_NEAR(castle::math::log(2.718281828459045), 1.0, 1.0e-12);
    EXPECT_NEAR(castle::math::log(10.0), 2.302585092994046, 1.0e-12);
    EXPECT_NEAR(castle::math::log(0.25), -1.3862943611198906, 1.0e-12);
    EXPECT_TRUE(castle::math::log(0.0) < -1.0e300);
    const double negative = castle::math::log(-1.0);
    EXPECT_TRUE(negative != negative);
}

TEST(MathExponential, LogLongDouble)
{
    EXPECT_NEAR(static_cast<double>(castle::math::log(1.0L)), 0.0, 1.0e-12);
    EXPECT_NEAR(static_cast<double>(castle::math::log(2.718281828459045L)), 1.0, 1.0e-12);
    EXPECT_NEAR(static_cast<double>(castle::math::log(100.0L)), 4.605170185988092, 1.0e-12);
    EXPECT_TRUE(castle::math::log(0.0L) < -1.0e300L);
    const long double negative = castle::math::log(-1.0L);
    EXPECT_TRUE(negative != negative);
}

TEST(MathExponential, RoundTrip)
{
    EXPECT_NEAR(castle::math::log(castle::math::exp(1.5F)), 1.5F, 1.0e-5F);
    EXPECT_NEAR(castle::math::log(castle::math::exp(-3.25)), -3.25, 1.0e-12);
    EXPECT_NEAR(static_cast<double>(castle::math::exp(castle::math::log(7.5L))), 7.5, 1.0e-12);
}

} // namespace

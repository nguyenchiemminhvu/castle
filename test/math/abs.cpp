#include <gtest/gtest.h>

#include "castle/math/abs.h"

#include <stdint.h>

namespace
{

// ---------------------------------------------------------------------------
// abs - signed integrals
// ---------------------------------------------------------------------------
TEST(MathAbs, SignedPositive)
{
    EXPECT_EQ(castle::math::abs<int>(5), 5);
    EXPECT_EQ(castle::math::abs<long>(123456L), 123456L);
    EXPECT_EQ(castle::math::abs<int8_t>(static_cast<int8_t>(7)), static_cast<int8_t>(7));
}

TEST(MathAbs, SignedNegative)
{
    EXPECT_EQ(castle::math::abs<int>(-5), 5);
    EXPECT_EQ(castle::math::abs<long>(-123456L), 123456L);
    EXPECT_EQ(castle::math::abs<int16_t>(static_cast<int16_t>(-30000)), static_cast<int16_t>(30000));
}

TEST(MathAbs, SignedZero)
{
    EXPECT_EQ(castle::math::abs<int>(0), 0);
    EXPECT_EQ(castle::math::abs<int64_t>(0), 0);
}

TEST(MathAbs, SignedNearMinIsSafe)
{
    // min() itself traps through the error handler, but min()+1 is well defined.
    EXPECT_EQ(castle::math::abs<int8_t>(static_cast<int8_t>(-127)), static_cast<int8_t>(127));
    EXPECT_EQ(castle::math::abs<int>(-2147483647), 2147483647);
}

// ---------------------------------------------------------------------------
// abs - unsigned integrals (identity)
// ---------------------------------------------------------------------------
TEST(MathAbs, UnsignedIdentity)
{
    EXPECT_EQ(castle::math::abs<unsigned>(0u), 0u);
    EXPECT_EQ(castle::math::abs<unsigned>(42u), 42u);
    EXPECT_EQ(castle::math::abs<uint8_t>(static_cast<uint8_t>(255)), static_cast<uint8_t>(255));
    EXPECT_EQ(castle::math::abs<uint32_t>(0xFFFFFFFFu), 0xFFFFFFFFu);
}

// ---------------------------------------------------------------------------
// abs - floating point
// ---------------------------------------------------------------------------
TEST(MathAbs, FloatingPoint)
{
    EXPECT_DOUBLE_EQ(castle::math::abs<double>(-3.5), 3.5);
    EXPECT_DOUBLE_EQ(castle::math::abs<double>(3.5), 3.5);
    EXPECT_DOUBLE_EQ(castle::math::abs<double>(0.0), 0.0);
    EXPECT_FLOAT_EQ(castle::math::abs<float>(-1.25f), 1.25f);
}

// ---------------------------------------------------------------------------
// uabs - signed -> unsigned magnitude, safe for entire signed range
// ---------------------------------------------------------------------------
TEST(MathUAbs, SignedValues)
{
    EXPECT_EQ(castle::math::uabs<int>(-5), 5u);
    EXPECT_EQ(castle::math::uabs<int>(5), 5u);
    EXPECT_EQ(castle::math::uabs<int>(0), 0u);
}

TEST(MathUAbs, SignedMinRepresentable)
{
    EXPECT_EQ(castle::math::uabs<int8_t>(static_cast<int8_t>(-128)), static_cast<uint8_t>(128));
    EXPECT_EQ(castle::math::uabs<int16_t>(static_cast<int16_t>(-32768)),
              static_cast<uint16_t>(32768));
    EXPECT_EQ(castle::math::uabs<int32_t>(-2147483647 - 1), static_cast<uint32_t>(2147483648u));
}

TEST(MathUAbs, UnsignedIdentity)
{
    EXPECT_EQ(castle::math::uabs<unsigned>(7u), 7u);
    EXPECT_EQ(castle::math::uabs<uint8_t>(static_cast<uint8_t>(255)), static_cast<uint8_t>(255));
}

} // namespace

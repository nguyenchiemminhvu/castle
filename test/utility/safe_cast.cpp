#include <gtest/gtest.h>

#include "castle/utility/safe_cast.h"

#include <stdint.h>

namespace
{

TEST(SafeCastTest, BoolToIntegral)
{
    EXPECT_EQ(castle::safe_cast::bool_to_int32(true), 1);
    EXPECT_EQ(castle::safe_cast::bool_to_int32(false), 0);
    EXPECT_EQ(castle::safe_cast::bool_to_uint8(true), 1);
    EXPECT_FLOAT_EQ(castle::safe_cast::bool_to_float(true), 1.0f);
}

TEST(SafeCastTest, IntegralToBool)
{
    EXPECT_TRUE(castle::safe_cast::int8_to_bool(5));
    EXPECT_FALSE(castle::safe_cast::int8_to_bool(0));
    EXPECT_TRUE(castle::safe_cast::uint32_to_bool(1U));
    EXPECT_FALSE(castle::safe_cast::uint32_to_bool(0U));
}

TEST(SafeCastTest, WideningPreservesValue)
{
    EXPECT_EQ(castle::safe_cast::int8_to_int32(-42), -42);
    EXPECT_EQ(castle::safe_cast::uint8_to_uint32(200U), 200U);
    EXPECT_EQ(castle::safe_cast::int16_to_int64(1000), 1000);
}

TEST(SafeCastTest, NarrowingWithinRange)
{
    EXPECT_EQ(castle::safe_cast::int32_to_int8(100), static_cast<int8_t>(100));
    EXPECT_EQ(castle::safe_cast::int32_to_int16(30000), static_cast<int16_t>(30000));
}

TEST(SafeCastTest, NarrowingClampsAtUpperBound)
{
    EXPECT_EQ(castle::safe_cast::int32_to_int8(200), INT8_MAX);
    EXPECT_EQ(castle::safe_cast::int32_to_int16(100000), INT16_MAX);
    EXPECT_EQ(castle::safe_cast::int32_to_uint8(300), static_cast<uint8_t>(UINT8_MAX));
}

TEST(SafeCastTest, NarrowingClampsAtLowerBound)
{
    EXPECT_EQ(castle::safe_cast::int32_to_int8(-200), INT8_MIN);
    EXPECT_EQ(castle::safe_cast::int32_to_int16(-100000), INT16_MIN);
}

TEST(SafeCastTest, NegativeToUnsignedClampsToZero)
{
    EXPECT_EQ(castle::safe_cast::int32_to_uint8(-5), 0);
    EXPECT_EQ(castle::safe_cast::int16_to_uint32(-1), 0U);
    EXPECT_EQ(castle::safe_cast::int64_to_uint64(-1), 0ULL);
}

TEST(SafeCastTest, UnsignedToSignedClampsAtMax)
{
    EXPECT_EQ(castle::safe_cast::uint32_to_int32(0xFFFFFFFFU), INT32_MAX);
    EXPECT_EQ(castle::safe_cast::uint64_to_int64(0xFFFFFFFFFFFFFFFFULL), INT64_MAX);
    EXPECT_EQ(castle::safe_cast::uint16_to_int8(1000U), INT8_MAX);
}

TEST(SafeCastTest, FloatToIntegralTruncatesAndClamps)
{
    EXPECT_EQ(castle::safe_cast::float_to_int32(3.7f), 3);
    EXPECT_EQ(castle::safe_cast::float_to_uint8(-1.0f), 0);
    EXPECT_EQ(castle::safe_cast::float_to_uint8(1000.0f), static_cast<uint8_t>(UINT8_MAX));
}

TEST(SafeCastTest, DoubleToIntegralTruncatesAndClamps)
{
    EXPECT_EQ(castle::safe_cast::double_to_int32(42.9), 42);
    EXPECT_EQ(castle::safe_cast::double_to_int8(500.0), INT8_MAX);
    EXPECT_EQ(castle::safe_cast::double_to_int8(-500.0), INT8_MIN);
    EXPECT_EQ(castle::safe_cast::double_to_uint16(-3.0), 0);
}

TEST(SafeCastTest, IntegralToFloatingPoint)
{
    EXPECT_DOUBLE_EQ(castle::safe_cast::int32_to_double(1234), 1234.0);
    EXPECT_FLOAT_EQ(castle::safe_cast::uint8_to_float(255U), 255.0f);
}

TEST(SafeCastTest, SafeCastTemplateForwardsToSpecialization)
{
    EXPECT_EQ((castle::SAFE_CAST<int32_t, int8_t>(200)), INT8_MAX);
    EXPECT_EQ((castle::SAFE_CAST<int32_t, uint8_t>(-5)), 0);
    EXPECT_EQ((castle::SAFE_CAST<bool, int32_t>(true)), 1);
    EXPECT_TRUE((castle::SAFE_CAST<int32_t, bool>(7)));
}

TEST(SafeCastTest, SafeCastTemplateWideningWithinRange)
{
    EXPECT_EQ((castle::SAFE_CAST<int8_t, int32_t>(-1)), -1);
    EXPECT_EQ((castle::SAFE_CAST<uint8_t, uint32_t>(128U)), 128U);
}

} // namespace

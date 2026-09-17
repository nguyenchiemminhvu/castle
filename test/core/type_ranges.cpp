#include <gtest/gtest.h>

#include <cfloat>
#include <climits>
#include <limits>

#include "castle/core/type_ranges.h"

namespace
{

template <typename T>
void ExpectIntegralLimits(bool expect_signed)
{
    using L = castle::numeric_limits<T>;
    static_assert(L::is_specialized, "must be specialized");
    static_assert(L::is_integer, "integral type");
    static_assert(L::is_exact, "integral is exact");

    EXPECT_EQ(L::is_signed, expect_signed);
    EXPECT_EQ(L::min(), std::numeric_limits<T>::min());
    EXPECT_EQ(L::max(), std::numeric_limits<T>::max());
    EXPECT_EQ(L::lowest(), std::numeric_limits<T>::lowest());
}

TEST(CoreTypeRangesTest, SignedIntegralSpecializations)
{
    ExpectIntegralLimits<signed char>(true);
    ExpectIntegralLimits<short>(true);
    ExpectIntegralLimits<int>(true);
    ExpectIntegralLimits<long>(true);
    ExpectIntegralLimits<long long>(true);
}

TEST(CoreTypeRangesTest, UnsignedIntegralSpecializations)
{
    ExpectIntegralLimits<unsigned char>(false);
    ExpectIntegralLimits<unsigned short>(false);
    ExpectIntegralLimits<unsigned int>(false);
    ExpectIntegralLimits<unsigned long>(false);
    ExpectIntegralLimits<unsigned long long>(false);
}

TEST(CoreTypeRangesTest, IntBoundsMatchClimits)
{
    static_assert(castle::numeric_limits<int>::min() == INT_MIN, "int min");
    static_assert(castle::numeric_limits<int>::max() == INT_MAX, "int max");
    static_assert(castle::numeric_limits<unsigned int>::min() == 0u, "uint min");
    static_assert(castle::numeric_limits<unsigned int>::max() == UINT_MAX, "uint max");

    EXPECT_EQ(castle::numeric_limits<int>::min(), INT_MIN);
    EXPECT_EQ(castle::numeric_limits<int>::max(), INT_MAX);
}

TEST(CoreTypeRangesTest, CharSpecializationSignednessTracksPlatform)
{
    using L = castle::numeric_limits<char>;
    static_assert(L::is_specialized, "char specialized");
    static_assert(L::is_integer, "char is integer");
    EXPECT_EQ(L::is_signed, (CHAR_MIN < 0));
    EXPECT_EQ(L::min(), CHAR_MIN);
    EXPECT_EQ(L::max(), CHAR_MAX);
}

TEST(CoreTypeRangesTest, BoolSpecialization)
{
    using L = castle::numeric_limits<bool>;
    static_assert(L::is_specialized, "bool specialized");
    static_assert(!L::is_signed, "bool unsigned");
    static_assert(L::is_integer, "bool integer");
    static_assert(L::is_exact, "bool exact");
    static_assert(L::min() == false, "bool min");
    static_assert(L::max() == true, "bool max");
    static_assert(L::lowest() == false, "bool lowest");
    EXPECT_FALSE(L::min());
    EXPECT_TRUE(L::max());
}

TEST(CoreTypeRangesTest, FloatSpecialization)
{
    using L = castle::numeric_limits<float>;
    static_assert(L::is_specialized, "float specialized");
    static_assert(L::is_signed, "float signed");
    static_assert(!L::is_integer, "float not integer");
    static_assert(!L::is_exact, "float not exact");

    EXPECT_FLOAT_EQ(L::min(), FLT_MIN);
    EXPECT_FLOAT_EQ(L::max(), FLT_MAX);
    EXPECT_FLOAT_EQ(L::lowest(), -FLT_MAX);
}

TEST(CoreTypeRangesTest, DoubleSpecialization)
{
    using L = castle::numeric_limits<double>;
    static_assert(L::is_specialized, "double specialized");
    static_assert(L::is_signed, "double signed");
    static_assert(!L::is_integer, "double not integer");
    static_assert(!L::is_exact, "double not exact");

    EXPECT_DOUBLE_EQ(L::min(), DBL_MIN);
    EXPECT_DOUBLE_EQ(L::max(), DBL_MAX);
    EXPECT_DOUBLE_EQ(L::lowest(), -DBL_MAX);
}

TEST(CoreTypeRangesTest, LongDoubleSpecialization)
{
    using L = castle::numeric_limits<long double>;
    static_assert(L::is_specialized, "long double specialized");
    static_assert(L::is_signed, "long double signed");
    static_assert(!L::is_integer, "long double not integer");
    static_assert(!L::is_exact, "long double not exact");

    EXPECT_EQ(L::min(), LDBL_MIN);
    EXPECT_EQ(L::max(), LDBL_MAX);
    EXPECT_EQ(L::lowest(), -LDBL_MAX);
}

TEST(CoreTypeRangesTest, MinAndLowestDifferForFloatingPoint)
{
    // For floating-point, lowest() is the most negative value while min() is
    // the smallest positive normalized value.
    EXPECT_LT(castle::numeric_limits<double>::lowest(), 0.0);
    EXPECT_GT(castle::numeric_limits<double>::min(), 0.0);
}

} // namespace

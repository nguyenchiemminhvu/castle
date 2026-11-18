#include <gtest/gtest.h>

#include <limits>
#include <stdint.h>
#include <type_traits>

#include "castle_ext/statistic/validation.hpp"

namespace
{
template <typename T>
void expect_floating_finite_cases()
{
    using limits = std::numeric_limits<T>;
    // NaN fails the first comparison (short-circuit).
    EXPECT_FALSE(castle::statistic::is_finite(limits::quiet_NaN()));
    // -inf fails the first comparison.
    EXPECT_FALSE(castle::statistic::is_finite(-limits::infinity()));
    // +inf passes the first comparison and fails the second.
    EXPECT_FALSE(castle::statistic::is_finite(limits::infinity()));
    // Both bounds themselves are finite.
    EXPECT_TRUE(castle::statistic::is_finite(limits::max()));
    EXPECT_TRUE(castle::statistic::is_finite(limits::lowest()));
    EXPECT_TRUE(castle::statistic::is_finite(static_cast<T>(0)));
    EXPECT_TRUE(castle::statistic::is_finite(static_cast<T>(-1.5)));
    EXPECT_TRUE(castle::statistic::is_finite(limits::denorm_min()));
}

template <typename T>
void expect_probability_cases()
{
    using limits = std::numeric_limits<T>;
    // Below zero: first comparison fails.
    EXPECT_FALSE(castle::statistic::is_probability(static_cast<T>(-0.1)));
    EXPECT_FALSE(castle::statistic::is_probability(-limits::infinity()));
    // Above one: first comparison passes, second fails.
    EXPECT_FALSE(castle::statistic::is_probability(static_cast<T>(1.1)));
    EXPECT_FALSE(castle::statistic::is_probability(limits::infinity()));
    // NaN fails the first comparison.
    EXPECT_FALSE(castle::statistic::is_probability(limits::quiet_NaN()));
    // Closed interval: both ends are accepted.
    EXPECT_TRUE(castle::statistic::is_probability(static_cast<T>(0)));
    EXPECT_TRUE(castle::statistic::is_probability(static_cast<T>(1)));
    EXPECT_TRUE(castle::statistic::is_probability(static_cast<T>(0.95)));
}

TEST(ValidationTest, ResultTypeMapping)
{
    // Floating types keep their precision.
    static_assert(std::is_same<castle::statistic::result_t<float>, float>::value, "float -> float");
    static_assert(std::is_same<castle::statistic::result_t<double>, double>::value, "double -> double");
    static_assert(std::is_same<castle::statistic::result_t<long double>, long double>::value,
                  "long double -> long double");
    // Integral types report in double.
    static_assert(std::is_same<castle::statistic::result_t<int>, double>::value, "int -> double");
    static_assert(std::is_same<castle::statistic::result_t<uint8_t>, double>::value, "uint8_t -> double");
    static_assert(std::is_same<castle::statistic::result_t<uint32_t>, double>::value, "uint32_t -> double");
    static_assert(std::is_same<castle::statistic::result_t<int64_t>, double>::value, "int64_t -> double");
    EXPECT_TRUE((std::is_same<castle::statistic::result_t<int16_t>, double>::value));
}

TEST(ValidationTest, FloatingIsFinite)
{
    expect_floating_finite_cases<float>();
    expect_floating_finite_cases<double>();
    expect_floating_finite_cases<long double>();
}

TEST(ValidationTest, IntegralIsAlwaysFinite)
{
    EXPECT_TRUE(castle::statistic::is_finite(0));
    EXPECT_TRUE(castle::statistic::is_finite(std::numeric_limits<int>::max()));
    EXPECT_TRUE(castle::statistic::is_finite(std::numeric_limits<int>::lowest()));
    EXPECT_TRUE(castle::statistic::is_finite(std::numeric_limits<uint32_t>::max()));
    EXPECT_TRUE(castle::statistic::is_finite(static_cast<uint8_t>(255U)));
    EXPECT_TRUE(castle::statistic::is_finite(std::numeric_limits<int64_t>::min()));
}

TEST(ValidationTest, IsProbability)
{
    expect_probability_cases<float>();
    expect_probability_cases<double>();
    expect_probability_cases<long double>();
}

TEST(ValidationTest, UsableInConstantExpressions)
{
    static_assert(castle::statistic::is_finite(1.0), "1.0 is finite");
    static_assert(castle::statistic::is_finite(7), "integers are finite");
    static_assert(castle::statistic::is_probability(0.5), "0.5 is a probability");
    static_assert(!castle::statistic::is_probability(2.0), "2.0 is not a probability");
    SUCCEED();
}
} // namespace


#include <gtest/gtest.h>

#include "castle/math/numeric/result.hpp"

namespace
{

TEST(MathNumericResult, FieldsAndOkForConvergedDouble)
{
    const castle::math::numeric_result<double> r = {1.5, 1e-9, 5U, castle::status::ok};
    EXPECT_DOUBLE_EQ(r.value, 1.5);
    EXPECT_DOUBLE_EQ(r.error, 1e-9);
    EXPECT_EQ(r.iterations, 5U);
    EXPECT_EQ(r.code, castle::status::ok);
    EXPECT_TRUE(r.ok());
}

TEST(MathNumericResult, OkIsFalseForEveryNonOkStatus)
{
    const castle::math::numeric_result<double> out_of_range = {2.0, 0.5, 12U, castle::status::out_of_range};
    EXPECT_FALSE(out_of_range.ok());
    EXPECT_DOUBLE_EQ(out_of_range.value, 2.0);

    const castle::math::numeric_result<double> invalid = {0.0, 0.0, 0U, castle::status::invalid_argument};
    EXPECT_FALSE(invalid.ok());
    EXPECT_EQ(invalid.iterations, 0U);

    const castle::math::numeric_result<double> empty = {0.0, 0.0, 0U, castle::status::empty};
    EXPECT_FALSE(empty.ok());
}

TEST(MathNumericResult, FloatInstantiation)
{
    const castle::math::numeric_result<float> r = {0.25F, 1e-6F, 3U, castle::status::ok};
    EXPECT_FLOAT_EQ(r.value, 0.25F);
    EXPECT_FLOAT_EQ(r.error, 1e-6F);
    EXPECT_EQ(r.iterations, 3U);
    EXPECT_TRUE(r.ok());

    const castle::math::numeric_result<float> failed = {0.25F, 1e-2F, 3U, castle::status::out_of_range};
    EXPECT_FALSE(failed.ok());
}

TEST(MathNumericResult, OkIsConstexpr)
{
    constexpr castle::math::numeric_result<double> good = {1.0, 0.0, 1U, castle::status::ok};
    constexpr castle::math::numeric_result<double> bad = {1.0, 0.0, 1U, castle::status::out_of_range};
    static_assert(good.ok(), "ok");
    static_assert(!bad.ok(), "not ok");
    EXPECT_TRUE(good.ok());
    EXPECT_FALSE(bad.ok());
}

TEST(MathNumericResult, WithinToleranceAbsoluteScaleForSmallValues)
{
    EXPECT_TRUE(castle::math::detail::within_tolerance(1e-9, 0.0, 1e-8));
    EXPECT_TRUE(castle::math::detail::within_tolerance(1e-8, 0.5, 1e-8));
    EXPECT_FALSE(castle::math::detail::within_tolerance(2e-8, 0.5, 1e-8));
    EXPECT_FALSE(castle::math::detail::within_tolerance(2e-8, -0.5, 1e-8));
    EXPECT_TRUE(castle::math::detail::within_tolerance(0.0, 0.0, 0.0));
}

TEST(MathNumericResult, WithinToleranceRelativeScaleForLargeValues)
{
    EXPECT_TRUE(castle::math::detail::within_tolerance(5e-5, 1000.0, 1e-7));
    EXPECT_TRUE(castle::math::detail::within_tolerance(5e-5, -1000.0, 1e-7));
    EXPECT_FALSE(castle::math::detail::within_tolerance(5e-3, 1000.0, 1e-7));
    EXPECT_FALSE(castle::math::detail::within_tolerance(5e-3, -1000.0, 1e-7));
}

TEST(MathNumericResult, WithinToleranceFloatAndConstexpr)
{
    EXPECT_TRUE(castle::math::detail::within_tolerance(1e-7F, 0.5F, 1e-6F));
    EXPECT_FALSE(castle::math::detail::within_tolerance(1e-5F, 0.5F, 1e-6F));
    EXPECT_TRUE(castle::math::detail::within_tolerance(1e-4F, 500.0F, 1e-6F));
    static_assert(castle::math::detail::within_tolerance(0.0, 0.0, 1e-3), "within");
    static_assert(!castle::math::detail::within_tolerance(1.0, 0.0, 1e-3), "outside");
}

} // namespace

#include <gtest/gtest.h>
#include <cmath>
#include <limits>
#include <type_traits>

#include "castle_ext/statistic/online_regression.hpp"

namespace
{
using castle::statistic::online_regression;

TEST(OnlineRegressionTest, StartsEmptyAndReportsEmptyEverywhere)
{
    online_regression<double> model;
    EXPECT_TRUE(model.empty());
    EXPECT_EQ(model.count(), 0U);

    double out = 42.0;
    double slope = 7.0;
    double intercept = 9.0;
    EXPECT_EQ(model.mean_x(out), castle::status::empty);
    EXPECT_EQ(model.mean_y(out), castle::status::empty);
    EXPECT_EQ(model.population_covariance(out), castle::status::empty);
    EXPECT_EQ(model.sample_covariance(out), castle::status::empty);
    EXPECT_EQ(model.correlation(out), castle::status::empty);
    EXPECT_EQ(model.fit(slope, intercept), castle::status::empty);
    // Outputs are untouched on failure.
    EXPECT_EQ(out, 42.0);
    EXPECT_EQ(slope, 7.0);
    EXPECT_EQ(intercept, 9.0);
}

TEST(OnlineRegressionTest, RejectsNonFiniteXAndY)
{
    online_regression<double> model;
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();

    // First operand of the finiteness check fails.
    EXPECT_EQ(model.push(nan, 1.0), castle::status::invalid_argument);
    EXPECT_EQ(model.push(-inf, 1.0), castle::status::invalid_argument);
    // First operand passes, second fails.
    EXPECT_EQ(model.push(1.0, nan), castle::status::invalid_argument);
    EXPECT_EQ(model.push(1.0, inf), castle::status::invalid_argument);
    EXPECT_TRUE(model.empty());

    EXPECT_EQ(model.push(1.0, 2.0), castle::status::ok);
    EXPECT_EQ(model.count(), 1U);
    EXPECT_FALSE(model.empty());
}

TEST(OnlineRegressionTest, SingleObservation)
{
    online_regression<double> model;
    EXPECT_EQ(model.push(3.0, 5.0), castle::status::ok);

    double out = 0.0;
    EXPECT_EQ(model.mean_x(out), castle::status::ok);
    EXPECT_DOUBLE_EQ(out, 3.0);
    EXPECT_EQ(model.mean_y(out), castle::status::ok);
    EXPECT_DOUBLE_EQ(out, 5.0);
    // Sxy = 0 with one pair, divided by n = 1.
    EXPECT_EQ(model.population_covariance(out), castle::status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);

    out = 11.0;
    // Sample covariance and correlation need two pairs.
    EXPECT_EQ(model.sample_covariance(out), castle::status::insufficient_data);
    EXPECT_EQ(model.correlation(out), castle::status::insufficient_data);
    EXPECT_EQ(out, 11.0);

    // One point: not enough data to fit a line.
    double slope = 1.5;
    double intercept = 2.5;
    EXPECT_EQ(model.fit(slope, intercept), castle::status::insufficient_data);
    EXPECT_EQ(slope, 1.5);
    EXPECT_EQ(intercept, 2.5);
}

TEST(OnlineRegressionTest, HandComputedCovarianceCorrelationAndFit)
{
    // x = {1, 2, 3}, y = {2, 4, 7}
    // mean_x = 2, mean_y = 13/3
    // Sxx = 2, Syy = (-7/3)^2 + (-1/3)^2 + (8/3)^2 = 114/9 = 38/3
    // Sxy = (-1)(-7/3) + 0 + (1)(8/3) = 5
    online_regression<double> model;
    EXPECT_EQ(model.push(1.0, 2.0), castle::status::ok);
    EXPECT_EQ(model.push(2.0, 4.0), castle::status::ok);
    EXPECT_EQ(model.push(3.0, 7.0), castle::status::ok);
    EXPECT_EQ(model.count(), 3U);

    double out = 0.0;
    EXPECT_EQ(model.mean_x(out), castle::status::ok);
    EXPECT_NEAR(out, 2.0, 1e-12);
    EXPECT_EQ(model.mean_y(out), castle::status::ok);
    EXPECT_NEAR(out, 13.0 / 3.0, 1e-12);
    // Population covariance = 5 / 3.
    EXPECT_EQ(model.population_covariance(out), castle::status::ok);
    EXPECT_NEAR(out, 5.0 / 3.0, 1e-12);
    // Sample covariance = 5 / 2.
    EXPECT_EQ(model.sample_covariance(out), castle::status::ok);
    EXPECT_NEAR(out, 2.5, 1e-12);
    // r = 5 / sqrt(2 * 38/3) = 5 / sqrt(76/3) ~= 0.993399267799.
    EXPECT_EQ(model.correlation(out), castle::status::ok);
    EXPECT_NEAR(out, 5.0 / std::sqrt(76.0 / 3.0), 1e-12);
    EXPECT_NEAR(out, 0.993399267799, 1e-9);

    // slope = Sxy / Sxx = 2.5; intercept = 13/3 - 2.5 * 2 = -2/3.
    double slope = 0.0;
    double intercept = 0.0;
    EXPECT_EQ(model.fit(slope, intercept), castle::status::ok);
    EXPECT_NEAR(slope, 2.5, 1e-12);
    EXPECT_NEAR(intercept, -2.0 / 3.0, 1e-12);
}

TEST(OnlineRegressionTest, ExactLineGivesSlopeInterceptAndUnitCorrelation)
{
    // y = 1 + 2x for x = 0..4.
    online_regression<double> model;
    for (int i = 0; i < 5; ++i)
    {
        const double x = static_cast<double>(i);
        EXPECT_EQ(model.push(x, 1.0 + 2.0 * x), castle::status::ok);
    }

    double slope = 0.0;
    double intercept = 0.0;
    EXPECT_EQ(model.fit(slope, intercept), castle::status::ok);
    EXPECT_NEAR(slope, 2.0, 1e-12);
    EXPECT_NEAR(intercept, 1.0, 1e-12);

    // Perfectly correlated: clamped into [-1, 1] and equal to 1.
    double r = 0.0;
    EXPECT_EQ(model.correlation(r), castle::status::ok);
    EXPECT_LE(r, 1.0);
    EXPECT_GE(r, -1.0);
    EXPECT_NEAR(r, 1.0, 1e-12);
}

TEST(OnlineRegressionTest, NegativeCorrelation)
{
    // y = 10 - 2x for x = {1, 2, 3, 4}.
    online_regression<double> model;
    EXPECT_EQ(model.push(1.0, 8.0), castle::status::ok);
    EXPECT_EQ(model.push(2.0, 6.0), castle::status::ok);
    EXPECT_EQ(model.push(3.0, 4.0), castle::status::ok);
    EXPECT_EQ(model.push(4.0, 2.0), castle::status::ok);

    double r = 0.0;
    EXPECT_EQ(model.correlation(r), castle::status::ok);
    EXPECT_GE(r, -1.0);
    EXPECT_NEAR(r, -1.0, 1e-12);

    // Sxx = 5, Sxy = -10 -> slope -2, intercept = 5 - (-2)(2.5) = 10.
    double cov = 0.0;
    EXPECT_EQ(model.population_covariance(cov), castle::status::ok);
    EXPECT_NEAR(cov, -10.0 / 4.0, 1e-12);
    EXPECT_EQ(model.sample_covariance(cov), castle::status::ok);
    EXPECT_NEAR(cov, -10.0 / 3.0, 1e-12);

    double slope = 0.0;
    double intercept = 0.0;
    EXPECT_EQ(model.fit(slope, intercept), castle::status::ok);
    EXPECT_NEAR(slope, -2.0, 1e-12);
    EXPECT_NEAR(intercept, 10.0, 1e-12);
}

TEST(OnlineRegressionTest, ConstantXIsInvalidForCorrelationAndFit)
{
    // Every x equal: Sxx = 0 (first operand of the variance check fails).
    online_regression<double> model;
    EXPECT_EQ(model.push(1.0, 1.0), castle::status::ok);
    EXPECT_EQ(model.push(1.0, 2.0), castle::status::ok);
    EXPECT_EQ(model.push(1.0, 3.0), castle::status::ok);

    double r = 0.5;
    EXPECT_EQ(model.correlation(r), castle::status::invalid_argument);
    EXPECT_EQ(r, 0.5);

    double slope = 3.0;
    double intercept = 4.0;
    EXPECT_EQ(model.fit(slope, intercept), castle::status::invalid_argument);
    EXPECT_EQ(slope, 3.0);
    EXPECT_EQ(intercept, 4.0);

    // Covariance is still defined: Sxy = 0.
    double cov = 1.0;
    EXPECT_EQ(model.sample_covariance(cov), castle::status::ok);
    EXPECT_DOUBLE_EQ(cov, 0.0);
}

TEST(OnlineRegressionTest, ConstantYIsInvalidForCorrelationButFitsFlatLine)
{
    // x varies, y constant: Sxx > 0 but Syy = 0 (second operand fails).
    online_regression<double> model;
    EXPECT_EQ(model.push(1.0, 5.0), castle::status::ok);
    EXPECT_EQ(model.push(2.0, 5.0), castle::status::ok);
    EXPECT_EQ(model.push(3.0, 5.0), castle::status::ok);

    double r = 0.25;
    EXPECT_EQ(model.correlation(r), castle::status::invalid_argument);
    EXPECT_EQ(r, 0.25);

    // Flat line y = 5.
    double slope = 1.0;
    double intercept = 1.0;
    EXPECT_EQ(model.fit(slope, intercept), castle::status::ok);
    EXPECT_NEAR(slope, 0.0, 1e-12);
    EXPECT_NEAR(intercept, 5.0, 1e-12);
}

TEST(OnlineRegressionTest, ClearDiscardsEverything)
{
    online_regression<double> model;
    EXPECT_EQ(model.push(1.0, 2.0), castle::status::ok);
    EXPECT_EQ(model.push(2.0, 3.0), castle::status::ok);
    model.clear();
    EXPECT_TRUE(model.empty());
    EXPECT_EQ(model.count(), 0U);
    double out = 0.0;
    EXPECT_EQ(model.mean_x(out), castle::status::empty);

    // Reusable after clear: y = 3x on {10, 20}.
    EXPECT_EQ(model.push(10.0, 30.0), castle::status::ok);
    EXPECT_EQ(model.push(20.0, 60.0), castle::status::ok);
    EXPECT_EQ(model.mean_x(out), castle::status::ok);
    EXPECT_NEAR(out, 15.0, 1e-12);
    double slope = 0.0;
    double intercept = 0.0;
    EXPECT_EQ(model.fit(slope, intercept), castle::status::ok);
    EXPECT_NEAR(slope, 3.0, 1e-12);
    EXPECT_NEAR(intercept, 0.0, 1e-12);
}

TEST(OnlineRegressionTest, FloatSamplesStayInFloat)
{
    online_regression<float> model;
    static_assert(std::is_same<online_regression<float>::result_type, float>::value, "float result");
    EXPECT_EQ(model.push(std::numeric_limits<float>::quiet_NaN(), 1.0f), castle::status::invalid_argument);
    EXPECT_EQ(model.push(0.0f, 1.0f), castle::status::ok);
    EXPECT_EQ(model.push(1.0f, 3.0f), castle::status::ok);
    float slope = 0.0f;
    float intercept = 0.0f;
    EXPECT_EQ(model.fit(slope, intercept), castle::status::ok);
    EXPECT_NEAR(slope, 2.0f, 1e-6f);
    EXPECT_NEAR(intercept, 1.0f, 1e-6f);
}

TEST(OnlineRegressionTest, IntegralSamplesReportDouble)
{
    // Same data as the hand-computed test, as int (no finiteness check).
    online_regression<int> model;
    static_assert(std::is_same<online_regression<int>::result_type, double>::value, "double result");
    EXPECT_EQ(model.push(1, 2), castle::status::ok);
    EXPECT_EQ(model.push(2, 4), castle::status::ok);
    EXPECT_EQ(model.push(3, 7), castle::status::ok);

    double out = 0.0;
    EXPECT_EQ(model.mean_y(out), castle::status::ok);
    EXPECT_NEAR(out, 13.0 / 3.0, 1e-12);
    EXPECT_EQ(model.sample_covariance(out), castle::status::ok);
    EXPECT_NEAR(out, 2.5, 1e-12);
    double slope = 0.0;
    double intercept = 0.0;
    EXPECT_EQ(model.fit(slope, intercept), castle::status::ok);
    EXPECT_NEAR(slope, 2.5, 1e-12);
    EXPECT_NEAR(intercept, -2.0 / 3.0, 1e-12);
}
} // namespace

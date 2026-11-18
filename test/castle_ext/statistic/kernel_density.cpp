#include <gtest/gtest.h>
#include <cmath>
#include <limits>

#include "castle_ext/statistic/kernel_density.hpp"
#include "castle_ext/statistic/sample_window.hpp"
#include "castle_ext/statistic/sorted_window.hpp"

namespace
{
using namespace castle::statistic;

const double kPi = 3.14159265358979323846;

TEST(KernelDensityTest, UniformKernelSupportBoundary)
{
    EXPECT_DOUBLE_EQ(uniform_kernel::evaluate(0.0), 0.5);
    EXPECT_DOUBLE_EQ(uniform_kernel::evaluate(0.99), 0.5);
    // |u| exactly 1 is inside the support.
    EXPECT_DOUBLE_EQ(uniform_kernel::evaluate(1.0), 0.5);
    EXPECT_DOUBLE_EQ(uniform_kernel::evaluate(-1.0), 0.5);
    EXPECT_DOUBLE_EQ(uniform_kernel::evaluate(1.5), 0.0);
    EXPECT_DOUBLE_EQ(uniform_kernel::evaluate(-1.5), 0.0);
    EXPECT_FLOAT_EQ(uniform_kernel::evaluate(0.25f), 0.5f);
}

TEST(KernelDensityTest, TriangularKernelSupportBoundary)
{
    EXPECT_DOUBLE_EQ(triangular_kernel::evaluate(0.0), 1.0);
    EXPECT_DOUBLE_EQ(triangular_kernel::evaluate(0.5), 0.5);
    EXPECT_DOUBLE_EQ(triangular_kernel::evaluate(-0.5), 0.5);
    EXPECT_DOUBLE_EQ(triangular_kernel::evaluate(1.0), 0.0);
    EXPECT_DOUBLE_EQ(triangular_kernel::evaluate(-1.0), 0.0);
    EXPECT_DOUBLE_EQ(triangular_kernel::evaluate(1.5), 0.0);
    EXPECT_DOUBLE_EQ(triangular_kernel::evaluate(-2.0), 0.0);
}

TEST(KernelDensityTest, EpanechnikovKernelSupportBoundary)
{
    EXPECT_DOUBLE_EQ(epanechnikov_kernel::evaluate(0.0), 0.75);
    // 0.75 * (1 - 0.25) = 0.5625
    EXPECT_DOUBLE_EQ(epanechnikov_kernel::evaluate(0.5), 0.5625);
    EXPECT_DOUBLE_EQ(epanechnikov_kernel::evaluate(-0.5), 0.5625);
    EXPECT_DOUBLE_EQ(epanechnikov_kernel::evaluate(1.0), 0.0);
    EXPECT_DOUBLE_EQ(epanechnikov_kernel::evaluate(-1.0), 0.0);
    EXPECT_DOUBLE_EQ(epanechnikov_kernel::evaluate(1.5), 0.0);
    EXPECT_DOUBLE_EQ(epanechnikov_kernel::evaluate(-1.5), 0.0);
}

TEST(KernelDensityTest, GaussianKernelValues)
{
    // exp(0) / sqrt(2 pi)
    EXPECT_NEAR(gaussian_kernel::evaluate(0.0), 0.398942280401, 1e-9);
    // exp(-0.5) / sqrt(2 pi)
    EXPECT_NEAR(gaussian_kernel::evaluate(1.0), std::exp(-0.5) / std::sqrt(2.0 * kPi), 1e-9);
    EXPECT_NEAR(gaussian_kernel::evaluate(-1.0), gaussian_kernel::evaluate(1.0), 1e-12);
    // Unbounded support: still positive far out.
    EXPECT_GT(gaussian_kernel::evaluate(3.0), 0.0);
    EXPECT_NEAR(gaussian_kernel::evaluate(0.0f), 0.3989423f, 1e-6f);
}

TEST(KernelDensityTest, RejectsInvalidArguments)
{
    const double samples[] = {0.0, 1.0};
    const double nan = std::numeric_limits<double>::quiet_NaN();
    const double inf = std::numeric_limits<double>::infinity();
    double out = 7.0;

    // Non-finite x (first operand fails).
    EXPECT_EQ(kernel_density(samples, samples + 2, nan, 1.0, out), castle::status::invalid_argument);
    EXPECT_EQ(kernel_density(samples, samples + 2, inf, 1.0, out), castle::status::invalid_argument);
    // Non-finite bandwidth (second operand fails).
    EXPECT_EQ(kernel_density(samples, samples + 2, 0.0, nan, out), castle::status::invalid_argument);
    EXPECT_EQ(kernel_density(samples, samples + 2, 0.0, inf, out), castle::status::invalid_argument);
    // Finite but non-positive bandwidth (third operand fails).
    EXPECT_EQ(kernel_density(samples, samples + 2, 0.0, 0.0, out), castle::status::invalid_argument);
    EXPECT_EQ(kernel_density(samples, samples + 2, 0.0, -1.0, out), castle::status::invalid_argument);
    EXPECT_EQ(out, 7.0);
}

TEST(KernelDensityTest, EmptyRangeReportsEmpty)
{
    const double* none = nullptr;
    double out = 7.0;
    EXPECT_EQ(kernel_density(none, none, 0.0, 1.0, out), castle::status::empty);
    EXPECT_EQ(out, 7.0);
}

TEST(KernelDensityTest, DefaultKernelIsGaussian)
{
    const double samples[] = {0.0};
    double out = 0.0;
    // 1 / (1 * 1) * exp(0) / sqrt(2 pi)
    EXPECT_EQ(kernel_density(samples, samples + 1, 0.0, 1.0, out), castle::status::ok);
    EXPECT_NEAR(out, 0.398942280401, 1e-9);
}

TEST(KernelDensityTest, CompactKernelsSingleSample)
{
    const double samples[] = {0.0};
    double out = 0.0;
    // u = 0.5; uniform: 0.5 / (1 * 1)
    EXPECT_EQ(kernel_density<uniform_kernel>(samples, samples + 1, 0.5, 1.0, out), castle::status::ok);
    EXPECT_DOUBLE_EQ(out, 0.5);
    // epanechnikov: 0.75 * (1 - 0.25) = 0.5625
    EXPECT_EQ(kernel_density<epanechnikov_kernel>(samples, samples + 1, 0.5, 1.0, out), castle::status::ok);
    EXPECT_DOUBLE_EQ(out, 0.5625);
    // triangular with h = 2: u = 0.25, (1 - 0.25) / 2 = 0.375
    EXPECT_EQ(kernel_density<triangular_kernel>(samples, samples + 1, 0.5, 2.0, out), castle::status::ok);
    EXPECT_DOUBLE_EQ(out, 0.375);
    // Outside the support: density 0.
    EXPECT_EQ(kernel_density<uniform_kernel>(samples, samples + 1, 2.0, 1.0, out), castle::status::ok);
    EXPECT_DOUBLE_EQ(out, 0.0);
}

TEST(KernelDensityTest, MultipleSamplesAverageTheKernels)
{
    // Samples {0, 1, 3}, x = 1, h = 1, uniform: u = 1, 0, -2 -> 0.5 + 0.5 + 0 = 1; f = 1 / (3 * 1) = 1/3.
    const int samples[] = {0, 1, 3};
    double out = 0.0;
    EXPECT_EQ(kernel_density<uniform_kernel>(samples, samples + 3, 1.0, 1.0, out), castle::status::ok);
    EXPECT_NEAR(out, 1.0 / 3.0, 1e-12);
    // Triangular, h = 2: u = 0.5, 0, -1 -> 0.5 + 1 + 0 = 1.5; f = 1.5 / (3 * 2) = 0.25.
    EXPECT_EQ(kernel_density<triangular_kernel>(samples, samples + 3, 1.0, 2.0, out), castle::status::ok);
    EXPECT_NEAR(out, 0.25, 1e-12);
}

TEST(KernelDensityTest, FloatResult)
{
    const float samples[] = {0.0f};
    float out = 0.0f;
    EXPECT_EQ(kernel_density(samples, samples + 1, 0.0f, 1.0f, out), castle::status::ok);
    EXPECT_NEAR(out, 0.3989423f, 1e-6f);
}

TEST(KernelDensityTest, WorksOverSampleWindowAndSortedWindow)
{
    sample_window<double, 4U> window;
    window.push(0.0);
    window.push(1.0);
    window.push(3.0);
    double out = 0.0;
    // Same data and expectation as the array test above.
    EXPECT_EQ(kernel_density<uniform_kernel>(window.begin(), window.end(), 1.0, 1.0, out), castle::status::ok);
    EXPECT_NEAR(out, 1.0 / 3.0, 1e-12);

    // Newest two samples {1, 3}: u = 0, -2 -> 0.5; f = 0.5 / 2 = 0.25.
    const auto latest = window.latest(2U);
    EXPECT_EQ(kernel_density<uniform_kernel>(latest.begin(), latest.end(), 1.0, 1.0, out), castle::status::ok);
    EXPECT_NEAR(out, 0.25, 1e-12);

    sorted_window<double, 4U> sorted;
    sorted.push(3.0);
    sorted.push(0.0);
    sorted.push(1.0);
    const auto view = sorted.sorted();
    EXPECT_EQ(kernel_density<uniform_kernel>(view.begin(), view.end(), 1.0, 1.0, out), castle::status::ok);
    EXPECT_NEAR(out, 1.0 / 3.0, 1e-12);

    // Empty window.
    sample_window<double, 4U> none;
    EXPECT_EQ(kernel_density(none.begin(), none.end(), 1.0, 1.0, out), castle::status::empty);
}

TEST(NormalReferenceBandwidthTest, KnownValue)
{
    const double samples[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    double h = 0.0;
    EXPECT_EQ(normal_reference_bandwidth(samples, samples + 5, h), castle::status::ok);
    // sample sd = sqrt(2.5); h = 1.06 * sd * 5^(-1/5) ~= 1.21474
    const double expected = 1.06 * std::sqrt(2.5) * std::pow(5.0, -0.2);
    EXPECT_NEAR(h, expected, 1e-9);
    EXPECT_NEAR(h, 1.21474, 1e-5);
}

TEST(NormalReferenceBandwidthTest, FloatInstance)
{
    const float samples[] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};
    float h = 0.0f;
    EXPECT_EQ(normal_reference_bandwidth(samples, samples + 5, h), castle::status::ok);
    EXPECT_NEAR(h, static_cast<float>(1.06 * std::sqrt(2.5) * std::pow(5.0, -0.2)), 1e-4f);
}

TEST(NormalReferenceBandwidthTest, IntegralSamples)
{
    const int samples[] = {1, 2, 3, 4, 5};
    double h = 0.0;
    EXPECT_EQ(normal_reference_bandwidth(samples, samples + 5, h), castle::status::ok);
    EXPECT_NEAR(h, 1.06 * std::sqrt(2.5) * std::pow(5.0, -0.2), 1e-9);
}

TEST(NormalReferenceBandwidthTest, FailureStatuses)
{
    double h = 9.0;
    const double bad[] = {1.0, std::numeric_limits<double>::quiet_NaN(), 3.0};
    // Non-finite sample: push_range failure is propagated.
    EXPECT_EQ(normal_reference_bandwidth(bad, bad + 3, h), castle::status::invalid_argument);

    // Empty and single-sample ranges: sample_stddev needs two.
    const double one[] = {1.0};
    EXPECT_EQ(normal_reference_bandwidth(one, one, h), castle::status::empty);
    EXPECT_EQ(normal_reference_bandwidth(one, one + 1, h), castle::status::empty);

    // Zero spread.
    const double flat[] = {3.0, 3.0, 3.0};
    EXPECT_EQ(normal_reference_bandwidth(flat, flat + 3, h), castle::status::invalid_argument);
    EXPECT_EQ(h, 9.0);
}
} // namespace

// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file kernel_density.hpp
 * @brief Kernel density estimation (KDE) with compile-time selected kernels.
 *
 * `kernel_density<Kernel>(first, last, x, h, out)` evaluates the density
 * estimate `f(x) = 1 / (n * h) * sum K((x - x_i) / h)` over any input range:
 * a caller-owned array, a `sample_window` or one of its `latest()` views, or
 * `sorted_window::sorted()`. The kernel is a template parameter, so only the
 * selected kernel is compiled in:
 * - `uniform_kernel`, `triangular_kernel`, `epanechnikov_kernel`: compact
 *   support on [-1, 1], arithmetic only.
 * - `gaussian_kernel`: unbounded support; uses `castle::math::exp` (`<math.h>`).
 *
 * `normal_reference_bandwidth()` gives the rule-of-thumb bandwidth
 * `h = 1.06 * s * n^(-1/5)` (Silverman, 1986), where `s` is the sample
 * standard deviation; it is near-optimal for unimodal, roughly normal data.
 *
 * @code
 * #include "castle_ext/statistic/kernel_density.hpp"
 *
 * const double samples[] = {1.0, 1.5, 2.0, 4.0};
 * double h = 0.0;
 * double density = 0.0;
 * if (castle::statistic::normal_reference_bandwidth(samples, samples + 4, h) == castle::status::ok &&
 *     castle::statistic::kernel_density<castle::statistic::epanechnikov_kernel>(samples, samples + 4, 1.5, h, density)
 *         == castle::status::ok)
 * {
 *     // density estimate at x = 1.5
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_KERNEL_DENSITY_HPP
#define CASTLE_EXT_STATISTIC_KERNEL_DENSITY_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/exponential.hpp"
#include "castle/math/sqrt_real.hpp"
#include "castle_ext/statistic/validation.hpp"
#include "castle_ext/statistic/running_moments.hpp"

namespace castle
{
namespace statistic
{

/** @brief Uniform (box) kernel: `1/2` on [-1, 1]. */
struct uniform_kernel
{
    /** @brief Kernel value at the scaled distance `u`. */
    template <typename R>
    static R evaluate(R u) CASTLE_NOEXCEPT
    {
        return (castle::math::abs(u) <= static_cast<R>(1)) ? static_cast<R>(0.5) : static_cast<R>(0);
    }
};

/** @brief Triangular kernel: `1 - |u|` on [-1, 1]. */
struct triangular_kernel
{
    /** @brief Kernel value at the scaled distance `u`. */
    template <typename R>
    static R evaluate(R u) CASTLE_NOEXCEPT
    {
        CASTLE_CONST R magnitude = castle::math::abs(u);
        return (magnitude <= static_cast<R>(1)) ? static_cast<R>(1) - magnitude : static_cast<R>(0);
    }
};

/** @brief Epanechnikov kernel: `3/4 * (1 - u^2)` on [-1, 1] (minimum asymptotic MISE). */
struct epanechnikov_kernel
{
    /** @brief Kernel value at the scaled distance `u`. */
    template <typename R>
    static R evaluate(R u) CASTLE_NOEXCEPT
    {
        CASTLE_CONST R square = u * u;
        return (square <= static_cast<R>(1)) ? static_cast<R>(0.75) * (static_cast<R>(1) - square) : static_cast<R>(0);
    }
};

/** @brief Gaussian kernel: `exp(-u^2 / 2) / sqrt(2 * pi)`. */
struct gaussian_kernel
{
    /** @brief Kernel value at the scaled distance `u`. */
    template <typename R>
    static R evaluate(R u) CASTLE_NOEXCEPT
    {
        CASTLE_CONST R normalization = static_cast<R>(1) / castle::math::sqrt_real(castle::math::tau<R>());
        return normalization * castle::math::exp(static_cast<R>(-0.5) * u * u);
    }
};

/**
 * @brief Kernel density estimate at `x`.
 * @tparam TKernel Kernel policy (`uniform_kernel`, `triangular_kernel`, `epanechnikov_kernel`, `gaussian_kernel`).
 * @tparam TIterator Input iterator over arithmetic samples.
 * @tparam R Floating-point result type (deduced from `out`).
 * @param x Evaluation point.
 * @param bandwidth Positive, finite smoothing bandwidth `h`.
 * @param out Receives the density on success; unchanged on failure.
 * @return `status::ok`, `status::invalid_argument` for a non-finite `x` or a
 *         non-positive/non-finite bandwidth, or `status::empty` for an empty range.
 * @note Complexity: O(N) time, O(1) stack. Samples are assumed finite.
 */
template <typename TKernel = gaussian_kernel, typename TIterator, typename R>
castle::status kernel_density(TIterator first,
                              TIterator last,
                              meta::type_identity_t<R> x,
                              meta::type_identity_t<R> bandwidth,
                              R& out) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<R>::value,
                  "castle::statistic::kernel_density: R must be a floating-point type");
    if (!(is_finite(x) && is_finite(bandwidth) && (bandwidth > static_cast<R>(0))))
    {
        return castle::status::invalid_argument;
    }
    size_type count = 0U;
    R sum = static_cast<R>(0);
    while (first != last)
    {
        sum += TKernel::evaluate((x - static_cast<R>(*first)) / bandwidth);
        ++count;
        ++first;
    }
    if (count == 0U)
    {
        return castle::status::empty;
    }
    out = sum / (static_cast<R>(count) * bandwidth);
    return castle::status::ok;
}

/**
 * @brief Rule-of-thumb bandwidth `1.06 * s * n^(-1/5)` for normal-like data.
 * @tparam TIterator Input iterator over arithmetic samples.
 * @tparam R Floating-point result type (deduced from `out`).
 * @param out Receives the bandwidth on success; unchanged on failure.
 * @return `status::ok`, `status::empty` with fewer than two samples, or
 *         `status::invalid_argument` for a non-finite sample or zero spread.
 * @note Complexity: O(N) time, O(1) stack.
 */
template <typename TIterator, typename R>
castle::status normal_reference_bandwidth(TIterator first, TIterator last, R& out) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<R>::value,
                  "castle::statistic::normal_reference_bandwidth: R must be a floating-point type");
    running_moments<R> moments;
    CASTLE_CONST castle::status pushed = moments.push_range(first, last);
    if (pushed != castle::status::ok)
    {
        return pushed;
    }
    R deviation = static_cast<R>(0);
    CASTLE_CONST castle::status result = moments.sample_stddev(deviation);
    if (result != castle::status::ok)
    {
        return result;
    }
    if (!(deviation > static_cast<R>(0)))
    {
        return castle::status::invalid_argument;
    }
    CASTLE_CONST R n = static_cast<R>(moments.count());
    out = static_cast<R>(1.06) * deviation * castle::math::exp(static_cast<R>(-0.2) * castle::math::log(n));
    return castle::status::ok;
}

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_KERNEL_DENSITY_HPP

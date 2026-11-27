// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sliding_histogram.hpp
 * @brief Approximate quantiles, CDF, and mode over the newest `Capacity` samples with O(1) push.
 *
 * `sliding_histogram<T, Capacity, BinCount>` pairs a `sample_window` with a
 * fixed-range `histogram`. A push adds the new sample to its bin and retracts
 * the evicted sample from its bin, so each push is O(1) and queries are
 * O(BinCount). Compared with the exact `sorted_window` it stores the window
 * once instead of twice and never shifts elements, which suits long windows;
 * quantile resolution is one bin width.
 *
 * Bin counters use the smallest unsigned type that can hold `Capacity`.
 *
 * @code
 * #include "castle_ext/statistic/sliding_histogram.hpp"
 *
 * // One hour of 1 Hz radial errors, 0.1 m resolution up to 25.6 m.
 * castle::statistic::sliding_histogram<float, 3600U, 256U> errors(0.0f, 25.6f);
 * errors.push(1.3f);
 * float p95 = 0.0f;
 * if (errors.quantile(0.95f, p95) == castle::status::ok)
 * {
 *     // approximate 95th percentile of the last hour
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_SLIDING_HISTOGRAM_HPP
#define CASTLE_EXT_STATISTIC_SLIDING_HISTOGRAM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle_ext/statistic/validation.hpp"
#include "castle_ext/statistic/histogram.hpp"
#include "castle_ext/statistic/sample_window.hpp"

namespace castle
{
namespace statistic
{

/**
 * @brief Histogram of a drop-oldest window of the newest `Capacity` samples.
 * @tparam T Arithmetic sample type.
 * @tparam Capacity Window length. Must be greater than zero.
 * @tparam BinCount Number of equal-width bins over the configured range.
 * @note Values outside the range are kept in the histogram's underflow/overflow counters.
 * @note Satisfies the estimator contract of `cep` (`push`, `quantile`, `cdf`, `count`, `empty`, `clear`).
 * @warning Not thread-safe; protect shared instances externally.
 */
template <typename T, size_type Capacity, size_type BinCount>
class sliding_histogram
{
public:
    using value_type     = T;
    using result_type    = result_t<T>;
    using size_type      = castle::size_type;
    using histogram_type = histogram<T, BinCount, meta::smallest_uint_t<Capacity>, histogram_range::fixed>;
    using window_type    = sample_window<T, Capacity>;

    /** @brief Constructs an unconfigured instance; `push()` reports `status::not_configured`. */
    sliding_histogram() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Constructs an instance whose histogram covers `[lower, upper)`.
     * @note Invalid bounds leave it unconfigured; check `configured()`.
     */
    sliding_histogram(result_type lower, result_type upper) CASTLE_NOEXCEPT
        : histogram_(lower, upper)
    {
    }

    /**
     * @brief Sets the histogram range and discards every sample.
     * @return `status::ok`, or `status::invalid_config` for invalid bounds.
     * @note Complexity: O(BinCount).
     */
    castle::status configure(result_type lower, result_type upper) CASTLE_NOEXCEPT
    {
        window_.clear();
        return histogram_.configure(lower, upper);
    }

    /** @brief Reports whether a valid range is set. */
    bool configured() CASTLE_CONST CASTLE_NOEXCEPT { return histogram_.configured(); }

    /** @brief Window length. */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return Capacity; }

    /** @brief Number of samples in the window. */
    size_type count() CASTLE_CONST CASTLE_NOEXCEPT { return window_.size(); }

    /** @brief Reports whether the window holds no sample. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return window_.empty(); }

    /** @brief Reports whether the window holds `Capacity` samples. */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return window_.full(); }

    /**
     * @brief Adds a sample, evicting and retracting the oldest one when the window is full.
     * @return `status::ok`, `status::not_configured`, or `status::invalid_argument` for NaN or infinity.
     * @note Complexity: O(1) time and stack.
     */
    castle::status push(T sample) CASTLE_NOEXCEPT
    {
        if (!histogram_.configured())
        {
            return castle::status::not_configured;
        }
        CASTLE_IF_CONSTEXPR (meta::is_floating_point<T>::value)
        {
            if (!is_finite(sample))
            {
                return castle::status::invalid_argument;
            }
        }
        T evicted{};
        if (window_.push(sample, evicted))
        {
            // The range never changes while samples are held, so the evicted
            // value maps to the same bin it was counted in.
            (void)histogram_.remove(evicted);
        }
        return histogram_.push(sample);
    }

    /** @brief Removes every sample; the range is kept. Complexity: O(BinCount). */
    void clear() CASTLE_NOEXCEPT
    {
        window_.clear();
        histogram_.clear();
    }

    /** @brief Approximate quantile of the window; see `histogram::quantile()`. O(BinCount). */
    castle::status quantile(result_type probability, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return histogram_.quantile(probability, out);
    }

    /** @brief Approximate median of the window. O(BinCount). */
    castle::status median(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return histogram_.quantile(static_cast<result_type>(0.5), out);
    }

    /** @brief Approximate CDF of the window; see `histogram::cdf()`. O(BinCount). */
    castle::status cdf(result_type value, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return histogram_.cdf(value, out);
    }

    /** @brief Centre of the heaviest bin of the window; see `histogram::mode()`. O(BinCount). */
    castle::status mode(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return histogram_.mode(out);
    }

    /** @brief Histogram of the current window. */
    CASTLE_CONST histogram_type& distribution() CASTLE_CONST CASTLE_NOEXCEPT { return histogram_; }

    /** @brief Retained samples in arrival order. */
    CASTLE_CONST window_type& samples() CASTLE_CONST CASTLE_NOEXCEPT { return window_; }

private:
    window_type window_{};
    histogram_type histogram_{};
};

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_SLIDING_HISTOGRAM_HPP

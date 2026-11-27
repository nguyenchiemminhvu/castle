// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sliding_moments.hpp
 * @brief O(1) mean, variance, RMS, skewness, and kurtosis over the newest `Capacity` samples.
 *
 * Each push adds the new sample to a `central_moments` accumulator and, once
 * the window is full, retracts the evicted sample. Retraction cancels digits,
 * which is handled two ways:
 * - Slow drift: a second accumulator is built from scratch alongside. After
 *   `Capacity` evicting pushes it holds exactly the current window and
 *   replaces the first one, so rounding drift spans at most `2 * Capacity` updates.
 * - Catastrophic cancellation: when a retraction shrinks the variance by more
 *   than `1 / sqrt(epsilon)` (a large outlier leaving the window), more than
 *   half of the significant digits are lost, and the accumulator is rebuilt
 *   from the window at O(Capacity) cost.
 *
 * Pushes are therefore O(1), except for the rare rebuild after such an outlier.
 *
 * @code
 * #include "castle_ext/statistic/sliding_moments.hpp"
 *
 * castle::statistic::sliding_moments<float, 50U> speed;
 * speed.push(12.5f);
 * float rms = 0.0f;
 * if (speed.rms(rms) == castle::status::ok)
 * {
 *     // RMS of the newest (up to) 50 samples.
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_SLIDING_MOMENTS_HPP
#define CASTLE_EXT_STATISTIC_SLIDING_MOMENTS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/algorithm.hpp"
#include "castle_ext/statistic/validation.hpp"
#include "castle_ext/statistic/running_moments.hpp"
#include "castle_ext/statistic/sample_window.hpp"

namespace castle
{
namespace statistic
{

/**
 * @brief Moment statistics over a drop-oldest window of the newest `Capacity` samples.
 * @tparam T Arithmetic sample type.
 * @tparam Capacity Window length. Must be greater than zero.
 * @tparam Order Highest maintained central moment: 2, 3, or 4.
 * @note Storage: the sample window plus two `central_moments<result_t<T>, Order>`.
 * @note Minimum, maximum, and range scan the window in O(Capacity); use
 *       `sorted_window` when they are needed on every sample.
 * @warning Not thread-safe; protect shared instances externally.
 */
template <typename T = double, size_type Capacity = 100U, size_type Order = 2U>
class sliding_moments
{
    static_assert(meta::is_arithmetic<T>::value,
                  "castle::statistic::sliding_moments: T must be an arithmetic type");

public:
    using value_type   = T;
    using result_type  = result_t<T>;
    using size_type    = castle::size_type;
    using moments_type = central_moments<result_type, Order>;
    using window_type  = sample_window<T, Capacity>;

    /** @brief Constructs an empty window. */
    sliding_moments() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /** @brief Window length. */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT { return Capacity; }

    /** @brief Number of samples in the window. */
    size_type count() CASTLE_CONST CASTLE_NOEXCEPT { return window_.size(); }

    /** @brief Reports whether the window holds no sample. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return window_.empty(); }

    /** @brief Reports whether the window holds `Capacity` samples. */
    bool full() CASTLE_CONST CASTLE_NOEXCEPT { return window_.full(); }

    /**
     * @brief Adds a sample, evicting the oldest one when the window is full.
     * @return `status::ok`, or `status::invalid_argument` for NaN or infinity.
     * @note Complexity: O(1) time and stack; O(Capacity) when a retraction
     *       would cancel more than half of the variance's significant digits.
     */
    castle::status push(T sample) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (meta::is_floating_point<T>::value)
        {
            if (!is_finite(sample))
            {
                return castle::status::invalid_argument;
            }
        }
        CASTLE_CONST result_type value = static_cast<result_type>(sample);
        T evicted{};
        if (!window_.push(sample, evicted))
        {
            (void)current_.push(value);
            return castle::status::ok;
        }
        result_type variance_before = static_cast<result_type>(0);
        (void)current_.population_variance(variance_before);
        (void)current_.remove(static_cast<result_type>(evicted));
        (void)current_.push(value);
        result_type variance_after = static_cast<result_type>(0);
        (void)current_.population_variance(variance_after);
        if (variance_after * variance_after
            < variance_before * variance_before * meta::floating_epsilon<result_type>::value)
        {
            rebuild();
        }
        (void)fresh_.push(value);
        if (fresh_.count() == Capacity)
        {
            // `fresh_` now holds exactly the window, free of retraction error.
            current_ = fresh_;
            fresh_.clear();
        }
        return castle::status::ok;
    }

    /** @brief Removes every sample. */
    void clear() CASTLE_NOEXCEPT
    {
        window_.clear();
        current_.clear();
        fresh_.clear();
    }

    /** @brief Mean of the window; `status::empty` without samples. */
    castle::status mean(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT { return current_.mean(out); }

    /** @brief Sum of the window; zero without samples. */
    result_type sum() CASTLE_CONST CASTLE_NOEXCEPT { return current_.sum(); }

    /** @brief Sum of squared samples of the window; zero without samples. */
    result_type sum_squares() CASTLE_CONST CASTLE_NOEXCEPT { return current_.sum_squares(); }

    /** @brief Moving RMS; `status::empty` without samples. */
    castle::status rms(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT { return current_.rms(out); }

    /** @brief Population variance; `status::empty` without samples. */
    castle::status population_variance(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return current_.population_variance(out);
    }

    /** @brief Sample variance; `status::empty` without samples, `status::insufficient_data` with one sample. */
    castle::status sample_variance(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return current_.sample_variance(out);
    }

    /** @brief Population standard deviation; `status::empty` without samples. */
    castle::status population_stddev(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return current_.population_stddev(out);
    }

    /** @brief Sample standard deviation; `status::empty` without samples, `status::insufficient_data` with one sample. */
    castle::status sample_stddev(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return current_.sample_stddev(out);
    }

    /** @brief Population skewness (`Order >= 3`); see `central_moments::skewness()`. */
    castle::status skewness(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return current_.skewness(out);
    }

    /** @brief Population excess kurtosis (`Order >= 4`); see `central_moments::excess_kurtosis()`. */
    castle::status excess_kurtosis(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return current_.excess_kurtosis(out);
    }

    /**
     * @brief Smallest sample in the window.
     * @return `status::ok`, or `status::empty` without samples.
     * @note Complexity: O(Capacity).
     */
    castle::status minimum(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        T low{};
        T high{};
        CASTLE_CONST castle::status result = scan(low, high);
        if (result == castle::status::ok)
        {
            out = static_cast<result_type>(low);
        }
        return result;
    }

    /**
     * @brief Largest sample in the window.
     * @return `status::ok`, or `status::empty` without samples.
     * @note Complexity: O(Capacity).
     */
    castle::status maximum(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        T low{};
        T high{};
        CASTLE_CONST castle::status result = scan(low, high);
        if (result == castle::status::ok)
        {
            out = static_cast<result_type>(high);
        }
        return result;
    }

    /**
     * @brief `maximum - minimum` of the window.
     * @return `status::ok`, or `status::empty` without samples.
     * @note Complexity: O(Capacity).
     */
    castle::status range(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        T low{};
        T high{};
        CASTLE_CONST castle::status result = scan(low, high);
        if (result == castle::status::ok)
        {
            out = static_cast<result_type>(high) - static_cast<result_type>(low);
        }
        return result;
    }

    /** @brief Retained samples in arrival order. */
    CASTLE_CONST window_type& samples() CASTLE_CONST CASTLE_NOEXCEPT { return window_; }

    /** @brief Moment accumulator describing the current window. */
    CASTLE_CONST moments_type& moments() CASTLE_CONST CASTLE_NOEXCEPT { return current_; }

private:
    /** Re-accumulates the current window from scratch, discarding retraction error. */
    void rebuild() CASTLE_NOEXCEPT
    {
        current_.clear();
        for (CASTLE_CONST T sample : window_)
        {
            (void)current_.push(static_cast<result_type>(sample));
        }
    }

    castle::status scan(T& low, T& high) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (window_.empty())
        {
            return castle::status::empty;
        }
        low = window_[0U];
        high = low;
        for (CASTLE_CONST T sample : window_)
        {
            low = castle::min(low, sample);
            high = castle::max(high, sample);
        }
        return castle::status::ok;
    }

    window_type window_{};
    moments_type current_{};
    moments_type fresh_{};
};

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_SLIDING_MOMENTS_HPP

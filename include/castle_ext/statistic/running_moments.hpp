// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file running_moments.hpp
 * @brief Numerically stable one-pass mean, variance, skewness, and kurtosis.
 *
 * `central_moments<R, Order>` is the arithmetic core: it keeps the count, the
 * mean, and the central moment sums M2..M_Order, and updates them with the
 * pairwise formulas of Chan, Golub & LeVeque and Pébay (Welford's method is
 * the single-sample case). The same update adds a sample, retracts a sample
 * (merge with weight -1), or merges two partitions, so sliding windows and
 * parallel/partitioned pipelines reuse one tested code path.
 *
 * `running_moments<T, Order>` wraps the core for unbounded streams and batch
 * ranges: it rejects non-finite samples and tracks the minimum and maximum.
 *
 * `Order` selects the highest maintained moment: 2 (mean and variance), 3
 * (adds skewness), or 4 (adds excess kurtosis). Unused moments cost neither
 * storage nor arithmetic.
 *
 * @code
 * #include "castle_ext/statistic/running_moments.hpp"
 *
 * castle::statistic::running_moments<double> stats;
 * stats.push(2.0);
 * stats.push(4.0);
 * double mean = 0.0;
 * if (stats.mean(mean) == castle::status::ok)
 * {
 *     // mean == 3.0
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_RUNNING_MOMENTS_HPP
#define CASTLE_EXT_STATISTIC_RUNNING_MOMENTS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/algorithm.hpp"
#include "castle/dsa/array.hpp"
#include "castle/math/sqrt_real.hpp"
#include "castle_ext/statistic/validation.hpp"

namespace castle
{
namespace statistic
{

/**
 * @brief Count, mean, and central moment sums with add, remove, and merge updates.
 * @tparam R Floating-point computation and result type.
 * @tparam Order Highest maintained central moment: 2, 3, or 4.
 * @note Inputs are not validated; callers pass finite values. `running_moments`
 *       and `sliding_moments` provide validated front ends.
 * @note Storage: one `size_type`, one `R` mean, and `Order - 1` moment sums.
 */
template <typename R = double, size_type Order = 2U>
class central_moments
{
    static_assert(meta::is_floating_point<R>::value,
                  "castle::statistic::central_moments: R must be a floating-point type");
    static_assert(Order >= 2U && Order <= 4U,
                  "castle::statistic::central_moments: Order must be 2, 3, or 4");

public:
    using result_type = R;
    using size_type   = castle::size_type;

    /** @brief Highest maintained central moment. */
    static CASTLE_CONSTEXPR size_type order = Order;

    /** @brief Constructs an empty accumulator. */
    central_moments() CASTLE_NOEXCEPT
    {
        clear();
    }

    /**
     * @brief Adds one sample.
     * @param value Finite sample value.
     * @return `status::ok`, or `status::full` when the sample counter is exhausted.
     * @note Complexity: O(1) time and stack.
     */
    castle::status push(R value) CASTLE_NOEXCEPT
    {
        if (count_ == max_count)
        {
            return castle::status::full;
        }
        combine(static_cast<R>(count_), static_cast<R>(1), value, zero_moments());
        ++count_;
        return castle::status::ok;
    }

    /**
     * @brief Retracts one previously added sample (merge with weight -1).
     * @param value Sample value that was added earlier.
     * @return `status::ok`, or `status::empty` when no sample is held.
     * @note Complexity: O(1) time and stack.
     * @warning Retraction cancels digits; long add/remove sequences accumulate
     *          rounding error. `sliding_moments` bounds that drift by periodically
     *          replacing its accumulator with a freshly built one.
     */
    castle::status remove(R value) CASTLE_NOEXCEPT
    {
        if (count_ == 0U)
        {
            return castle::status::empty;
        }
        if (count_ == 1U)
        {
            clear();
            return castle::status::ok;
        }
        combine(static_cast<R>(count_), static_cast<R>(-1), value, zero_moments());
        --count_;
        return castle::status::ok;
    }

    /**
     * @brief Merges another accumulator, as if its samples had been added here.
     * @param other Accumulator to merge; may be `*this`.
     * @return `status::ok`, or `status::full` when the combined count overflows.
     * @note Complexity: O(1) time and stack.
     */
    castle::status merge(CASTLE_CONST central_moments& other) CASTLE_NOEXCEPT
    {
        if (other.count_ == 0U)
        {
            return castle::status::ok;
        }
        if (count_ == 0U)
        {
            *this = other;
            return castle::status::ok;
        }
        if (other.count_ > max_count - count_)
        {
            return castle::status::full;
        }
        CASTLE_CONST size_type other_count = other.count_;
        combine(static_cast<R>(count_), static_cast<R>(other_count), other.mean_, other.moments_);
        count_ += other_count;
        return castle::status::ok;
    }

    /** @brief Removes every sample. */
    void clear() CASTLE_NOEXCEPT
    {
        count_ = 0U;
        mean_ = static_cast<R>(0);
        for (size_type i = 0U; i < Order - 1U; ++i)
        {
            moments_[i] = static_cast<R>(0);
        }
    }

    /** @brief Number of samples held. */
    size_type count() CASTLE_CONST CASTLE_NOEXCEPT { return count_; }

    /** @brief Reports whether no sample is held. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return count_ == 0U; }

    /**
     * @brief Arithmetic mean.
     * @return `status::ok`, or `status::empty` without samples.
     */
    castle::status mean(R& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (count_ == 0U)
        {
            return castle::status::empty;
        }
        out = mean_;
        return castle::status::ok;
    }

    /** @brief Sum of the samples (`mean * count`); zero without samples. */
    R sum() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return mean_ * static_cast<R>(count_);
    }

    /** @brief Sum of squared samples (`M2 + count * mean^2`); zero without samples. */
    R sum_squares() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return second_moment() + static_cast<R>(count_) * mean_ * mean_;
    }

    /**
     * @brief Root mean square, `sqrt(sum_squares / count)`.
     * @return `status::ok`, or `status::empty` without samples.
     */
    castle::status rms(R& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (count_ == 0U)
        {
            return castle::status::empty;
        }
        out = castle::math::sqrt_real(sum_squares() / static_cast<R>(count_));
        return castle::status::ok;
    }

    /**
     * @brief Population variance, `M2 / n`.
     * @return `status::ok`, or `status::empty` without samples.
     */
    castle::status population_variance(R& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (count_ == 0U)
        {
            return castle::status::empty;
        }
        out = second_moment() / static_cast<R>(count_);
        return castle::status::ok;
    }

    /**
     * @brief Unbiased sample variance, `M2 / (n - 1)`.
     * @return `status::ok`, `status::empty` without samples, or
     *         `status::insufficient_data` with exactly one sample.
     */
    castle::status sample_variance(R& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (count_ == 0U)
        {
            return castle::status::empty;
        }
        if (count_ < 2U)
        {
            return castle::status::insufficient_data;
        }
        out = second_moment() / static_cast<R>(count_ - 1U);
        return castle::status::ok;
    }

    /** @brief Square root of `population_variance()`; same status rules. */
    castle::status population_stddev(R& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        R variance = static_cast<R>(0);
        CASTLE_CONST castle::status result = population_variance(variance);
        if (result == castle::status::ok)
        {
            out = castle::math::sqrt_real(variance);
        }
        return result;
    }

    /** @brief Square root of `sample_variance()`; same status rules. */
    castle::status sample_stddev(R& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        R variance = static_cast<R>(0);
        CASTLE_CONST castle::status result = sample_variance(variance);
        if (result == castle::status::ok)
        {
            out = castle::math::sqrt_real(variance);
        }
        return result;
    }

    /**
     * @brief Population skewness g1 = `sqrt(n) * M3 / M2^1.5`. Requires `Order >= 3`.
     * @return `status::ok`, `status::empty` without samples, or
     *         `status::invalid_argument` when the variance is zero.
     */
    castle::status skewness(R& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        static_assert(Order >= 3U, "castle::statistic: skewness() requires Order >= 3");
        if (count_ == 0U)
        {
            return castle::status::empty;
        }
        CASTLE_CONST R m2 = moments_[0];
        if (!(m2 > static_cast<R>(0)))
        {
            return castle::status::invalid_argument;
        }
        CASTLE_CONST R n = static_cast<R>(count_);
        out = castle::math::sqrt_real(n) * moments_[1] / (m2 * castle::math::sqrt_real(m2));
        return castle::status::ok;
    }

    /**
     * @brief Population excess kurtosis g2 = `n * M4 / M2^2 - 3`. Requires `Order >= 4`.
     * @return `status::ok`, `status::empty` without samples, or
     *         `status::invalid_argument` when the variance is zero.
     */
    castle::status excess_kurtosis(R& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        static_assert(Order >= 4U, "castle::statistic: excess_kurtosis() requires Order >= 4");
        if (count_ == 0U)
        {
            return castle::status::empty;
        }
        CASTLE_CONST R m2 = moments_[0];
        if (!(m2 > static_cast<R>(0)))
        {
            return castle::status::invalid_argument;
        }
        out = static_cast<R>(count_) * moments_[2] / (m2 * m2) - static_cast<R>(3);
        return castle::status::ok;
    }

private:
    using moment_array = castle::dsa::array<R, Order - 1U>;

    static CASTLE_CONSTEXPR size_type max_count = static_cast<size_type>(-1);

    static moment_array zero_moments() CASTLE_NOEXCEPT
    {
        moment_array zero{};
        for (size_type i = 0U; i < Order - 1U; ++i)
        {
            zero[i] = static_cast<R>(0);
        }
        return zero;
    }

    /** M2, clamped at zero: retraction can leave a tiny negative rounding residue. */
    R second_moment() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::max(static_cast<R>(0), moments_[0]);
    }

    /**
     * Combines this partition (count `na`) with partition B (count `nb`, mean
     * `mean_b`, moment sums `mb`). `nb = 1` adds a sample and `nb = -1` retracts
     * one. Higher moments update first because they read the old lower moments.
     * `mb` may alias `moments_`; every read happens before the matching write.
     */
    void combine(R na, R nb, R mean_b, CASTLE_CONST moment_array& mb) CASTLE_NOEXCEPT
    {
        CASTLE_CONST R n = na + nb;
        CASTLE_CONST R delta = mean_b - mean_;
        CASTLE_CONST R delta_n = delta / n;
        CASTLE_CONST R term = delta * delta_n * na * nb;

        CASTLE_IF_CONSTEXPR (Order >= 4U)
        {
            CASTLE_CONST R delta_n2 = delta_n * delta_n;
            moments_[2] += mb[2]
                         + term * delta_n2 * (na * na - na * nb + nb * nb)
                         + static_cast<R>(6) * delta_n2 * (na * na * mb[0] + nb * nb * moments_[0])
                         + static_cast<R>(4) * delta_n * (na * mb[1] - nb * moments_[1]);
        }
        CASTLE_IF_CONSTEXPR (Order >= 3U)
        {
            moments_[1] += mb[1]
                         + term * delta_n * (na - nb)
                         + static_cast<R>(3) * delta_n * (na * mb[0] - nb * moments_[0]);
        }
        moments_[0] += mb[0] + term;
        mean_ += delta_n * nb;
    }

    size_type count_;
    R mean_;
    moment_array moments_;
};

/**
 * @brief Validated running statistics over an unbounded stream or a batch range.
 * @tparam T Arithmetic sample type.
 * @tparam Order Highest maintained central moment: 2, 3, or 4.
 * @note Results use `result_t<T>`. All moment queries are inherited from `central_moments`.
 * @note Retraction is not offered: it would invalidate the tracked minimum and maximum.
 *       Use `sliding_moments` for windowed statistics.
 */
template <typename T = double, size_type Order = 2U>
class running_moments : private central_moments<result_t<T>, Order>
{
    static_assert(meta::is_arithmetic<T>::value,
                  "castle::statistic::running_moments: T must be an arithmetic type");

    using base_type = central_moments<result_t<T>, Order>;

public:
    using value_type  = T;
    using result_type = result_t<T>;
    using size_type   = castle::size_type;

    using base_type::count;
    using base_type::empty;
    using base_type::mean;
    using base_type::sum;
    using base_type::sum_squares;
    using base_type::rms;
    using base_type::population_variance;
    using base_type::sample_variance;
    using base_type::population_stddev;
    using base_type::sample_stddev;
    using base_type::skewness;
    using base_type::excess_kurtosis;

    /** @brief Constructs an empty accumulator. */
    running_moments() CASTLE_NOEXCEPT
        : base_type(), minimum_(static_cast<result_type>(0)), maximum_(static_cast<result_type>(0))
    {
    }

    /**
     * @brief Adds one sample.
     * @return `status::ok`, `status::invalid_argument` for NaN or infinity, or
     *         `status::full` when the sample counter is exhausted.
     * @note Complexity: O(1) time and stack.
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
        CASTLE_CONST bool first = empty();
        CASTLE_CONST castle::status result = base_type::push(value);
        if (result != castle::status::ok)
        {
            return result;
        }
        if (first)
        {
            minimum_ = value;
            maximum_ = value;
        }
        else
        {
            minimum_ = castle::min(minimum_, value);
            maximum_ = castle::max(maximum_, value);
        }
        return castle::status::ok;
    }

    /**
     * @brief Adds every sample of an iterator range (batch analysis).
     * @return `status::ok`, or the first failing `push()` status; samples before
     *         the failing one remain added.
     * @note Complexity: O(N) time, O(1) stack.
     */
    template <typename TIterator>
    castle::status push_range(TIterator first, TIterator last) CASTLE_NOEXCEPT
    {
        while (first != last)
        {
            CASTLE_CONST castle::status result = push(static_cast<T>(*first));
            if (result != castle::status::ok)
            {
                return result;
            }
            ++first;
        }
        return castle::status::ok;
    }

    /**
     * @brief Merges another accumulator, including its minimum and maximum.
     * @return `status::ok`, or `status::full` when the combined count overflows.
     * @note Complexity: O(1).
     */
    castle::status merge(CASTLE_CONST running_moments& other) CASTLE_NOEXCEPT
    {
        if (other.empty())
        {
            return castle::status::ok;
        }
        CASTLE_CONST bool first = empty();
        CASTLE_CONST result_type other_minimum = other.minimum_;
        CASTLE_CONST result_type other_maximum = other.maximum_;
        CASTLE_CONST castle::status result = base_type::merge(other);
        if (result != castle::status::ok)
        {
            return result;
        }
        if (first)
        {
            minimum_ = other_minimum;
            maximum_ = other_maximum;
        }
        else
        {
            minimum_ = castle::min(minimum_, other_minimum);
            maximum_ = castle::max(maximum_, other_maximum);
        }
        return castle::status::ok;
    }

    /** @brief Smallest sample; `status::empty` without samples. */
    castle::status minimum(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return report(minimum_, out);
    }

    /** @brief Largest sample; `status::empty` without samples. */
    castle::status maximum(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return report(maximum_, out);
    }

    /** @brief `maximum - minimum`; `status::empty` without samples. */
    castle::status range(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return report(maximum_ - minimum_, out);
    }

    /** @brief Removes every sample. */
    void clear() CASTLE_NOEXCEPT
    {
        base_type::clear();
        minimum_ = static_cast<result_type>(0);
        maximum_ = static_cast<result_type>(0);
    }

private:
    castle::status report(result_type value, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return castle::status::empty;
        }
        out = value;
        return castle::status::ok;
    }

    result_type minimum_;
    result_type maximum_;
};

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_RUNNING_MOMENTS_HPP

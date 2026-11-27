// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file sorted_window.hpp
 * @brief Exact sliding-window order statistics with O(1) quantile queries.
 *
 * `sorted_window<T, Capacity>` keeps the newest `Capacity` samples twice: in
 * arrival order (a `sample_window`, to know which sample leaves next) and in
 * ascending order. Each push binary-searches the evicted and the new sample in
 * the sorted copy and shifts only the elements between the two positions, so
 * a push costs O(log N) comparisons plus at most N moves, with O(1) stack.
 * Quantiles, median, minimum, maximum, range, and IQR are then O(1) reads;
 * the empirical CDF is O(log N) and the mode O(N).
 *
 * @code
 * #include "castle_ext/statistic/sorted_window.hpp"
 *
 * castle::statistic::sorted_window<float, 100U> latency;
 * latency.push(3.0f);
 * latency.push(1.0f);
 * float median = 0.0f;
 * if (latency.median(median) == castle::status::ok)
 * {
 *     // median == 2.0f
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_SORTED_WINDOW_HPP
#define CASTLE_EXT_STATISTIC_SORTED_WINDOW_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/algorithm.hpp"
#include "castle/dsa/array.hpp"
#include "castle/dsa/array_view.hpp"
#include "castle_ext/statistic/validation.hpp"
#include "castle_ext/statistic/order_statistics.hpp"
#include "castle_ext/statistic/sample_window.hpp"

namespace castle
{
namespace statistic
{

/**
 * @brief Exact order statistics over a drop-oldest window of the newest `Capacity` samples.
 * @tparam T Arithmetic sample type.
 * @tparam Capacity Window length. Must be greater than zero.
 * @note Storage: `2 * Capacity` samples plus the ring indices; no heap.
 * @note Satisfies the estimator contract of `cep` (`push`, `quantile`, `cdf`, `count`, `empty`, `clear`).
 * @warning Not thread-safe; protect shared instances externally.
 */
template <typename T = double, size_type Capacity = 100U>
class sorted_window
{
    static_assert(meta::is_arithmetic<T>::value,
                  "castle::statistic::sorted_window: T must be an arithmetic type");

public:
    using value_type  = T;
    using result_type = result_t<T>;
    using size_type   = castle::size_type;
    using window_type = sample_window<T, Capacity>;

    /** @brief Constructs an empty window. */
    sorted_window() CASTLE_NOEXCEPT CASTLE_DEFAULT;

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
     * @note Complexity: O(log N) comparisons plus O(N) element moves worst case; O(1) stack.
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
        T* CASTLE_CONST data = sorted_.data();
        CASTLE_CONST size_type held = window_.size();
        // Insert after any equal samples so equal values keep arrival order.
        CASTLE_CONST size_type insert_at = static_cast<size_type>(castle::upper_bound(data, data + held, sample) - data);

        T evicted{};
        if (!window_.push(sample, evicted))
        {
            for (size_type i = held; i > insert_at; --i)
            {
                data[i] = data[i - 1U];
            }
            data[insert_at] = sample;
            return castle::status::ok;
        }

        // Replace the evicted sample: shift only the elements between both positions.
        CASTLE_CONST size_type remove_at = static_cast<size_type>(castle::lower_bound(data, data + held, evicted) - data);
        if (insert_at > remove_at)
        {
            for (size_type i = remove_at; i + 1U < insert_at; ++i)
            {
                data[i] = data[i + 1U];
            }
            data[insert_at - 1U] = sample;
        }
        else
        {
            for (size_type i = remove_at; i > insert_at; --i)
            {
                data[i] = data[i - 1U];
            }
            data[insert_at] = sample;
        }
        return castle::status::ok;
    }

    /** @brief Removes every sample. */
    void clear() CASTLE_NOEXCEPT
    {
        window_.clear();
    }

    /**
     * @brief Exact quantile of the window.
     * @param probability Quantile in [0, 1].
     * @param out Receives the quantile on success; unchanged on failure.
     * @param method Quantile definition.
     * @return `status::ok`, `status::invalid_argument`, or `status::empty`.
     * @note Complexity: O(1).
     */
    castle::status quantile(result_type probability,
                            result_type& out,
                            quantile_method method = quantile_method::linear) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return quantile_sorted(sorted_begin(), sorted_end(), probability, out, method);
    }

    /** @brief Median (linear quantile at 0.5); `status::empty` without samples. O(1). */
    castle::status median(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return median_sorted(sorted_begin(), sorted_end(), out);
    }

    /** @brief Interquartile range `Q3 - Q1`; `status::empty` without samples. O(1). */
    castle::status interquartile_range(result_type& out,
                                       quantile_method method = quantile_method::linear) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return interquartile_range_sorted(sorted_begin(), sorted_end(), out, method);
    }

    /** @brief Smallest sample; `status::empty` without samples. O(1). */
    castle::status minimum(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return castle::status::empty;
        }
        out = static_cast<result_type>(sorted_[0U]);
        return castle::status::ok;
    }

    /** @brief Largest sample; `status::empty` without samples. O(1). */
    castle::status maximum(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return castle::status::empty;
        }
        out = static_cast<result_type>(sorted_[count() - 1U]);
        return castle::status::ok;
    }

    /** @brief `maximum - minimum`; `status::empty` without samples. O(1). */
    castle::status range(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (empty())
        {
            return castle::status::empty;
        }
        out = static_cast<result_type>(sorted_[count() - 1U]) - static_cast<result_type>(sorted_[0U]);
        return castle::status::ok;
    }

    /**
     * @brief Most frequent sample value (ties resolve to the smallest value).
     * @return `status::ok`, or `status::empty` without samples.
     * @note Exact equality; meaningful for discrete data. Complexity: O(N).
     */
    castle::status mode(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return mode_sorted(sorted_begin(), sorted_end(), out);
    }

    /**
     * @brief Empirical CDF: fraction of samples less than or equal to `value`.
     * @return `status::ok`, `status::invalid_argument` for NaN, or `status::empty`.
     * @note Complexity: O(log N).
     */
    castle::status cdf(result_type value, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (value != value) // Only NaN compares unequal to itself; infinities are valid bounds.
        {
            return castle::status::invalid_argument;
        }
        if (empty())
        {
            return castle::status::empty;
        }
        CASTLE_CONST T* CASTLE_CONST position =
            castle::upper_bound(sorted_begin(), sorted_end(), value,
                                [](result_type lhs, T rhs) CASTLE_NOEXCEPT { return lhs < static_cast<result_type>(rhs); });
        out = static_cast<result_type>(position - sorted_begin()) / static_cast<result_type>(count());
        return castle::status::ok;
    }

    /** @brief Ascending view of the window, for batch functions such as `kernel_density`. */
    castle::dsa::array_view<CASTLE_CONST T> sorted() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return castle::dsa::array_view<CASTLE_CONST T>(sorted_.data(), count());
    }

    /** @brief Retained samples in arrival order. */
    CASTLE_CONST window_type& samples() CASTLE_CONST CASTLE_NOEXCEPT { return window_; }

private:
    CASTLE_CONST T* sorted_begin() CASTLE_CONST CASTLE_NOEXCEPT { return sorted_.data(); }
    CASTLE_CONST T* sorted_end() CASTLE_CONST CASTLE_NOEXCEPT { return sorted_.data() + count(); }

    window_type window_{};
    castle::dsa::array<T, Capacity> sorted_{};
};

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_SORTED_WINDOW_HPP

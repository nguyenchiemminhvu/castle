// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file histogram.hpp
 * @brief Fixed-bin, equal-width weighted histogram with approximate quantiles, CDF, and mode.
 *
 * The bin count is a template parameter; the value range `[lower, upper)` is
 * configured at runtime. Two range policies are available:
 * - `histogram_range::fixed`: values below `lower` or at/above `upper` are
 *   tallied in separate underflow/overflow counters instead of distorting the
 *   edge bins. Samples can be retracted, which `sliding_histogram` relies on.
 * - `histogram_range::expanding`: auto-ranging with a fixed number of bins.
 *   When a value falls outside the range, the range doubles away from the
 *   value's side and adjacent bins merge pairwise, until the value fits. Use it
 *   when the spread of a stream (e.g. GNSS error radii) is not known upfront.
 *
 * Quantiles and the CDF assume that the weight of a bin is spread uniformly
 * across it. Underflow/overflow weight is located at the range edges, and the
 * CDF reports a conservative lower bound where that weight makes it ambiguous.
 *
 * @code
 * #include "castle_ext/statistic/histogram.hpp"
 *
 * castle::statistic::histogram<double, 50U> errors(0.0, 10.0);
 * errors.push(1.2);
 * errors.push(2.8, 3U); // weighted: three equal observations
 * double p95 = 0.0;
 * if (errors.quantile(0.95, p95) == castle::status::ok)
 * {
 *     // approximate 95th percentile, resolution 0.2
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_HISTOGRAM_HPP
#define CASTLE_EXT_STATISTIC_HISTOGRAM_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/algorithm.hpp"
#include "castle/dsa/array.hpp"
#include "castle/math/clamp.hpp"
#include "castle_ext/statistic/validation.hpp"

#include <stdint.h>

namespace castle
{
namespace statistic
{

/**
 * @brief Range policy of a `histogram`.
 */
enum class histogram_range : uint8_t
{
    fixed = 0U, /**< Out-of-range values go to the underflow/overflow counters. */
    expanding   /**< The range doubles (bins merge pairwise) until the value fits. */
};

/**
 * @brief Equal-width histogram over `[lower, upper)` with `BinCount` bins.
 * @tparam T Arithmetic sample type.
 * @tparam BinCount Number of bins. Must be greater than zero; even (and >= 2) for `expanding`.
 * @tparam TCount Weight/counter type: an unsigned integer for counts, or a floating-point type for real weights.
 * @tparam Range Range policy.
 * @note Storage: `BinCount + 3` counters, two bounds, and a flag.
 * @note Satisfies the estimator contract of `cep` (`push`, `quantile`, `cdf`, `count`, `empty`, `clear`).
 * @warning Not thread-safe; protect shared instances externally.
 */
template <typename T = double,
          size_type BinCount = 32U,
          typename TCount = uint32_t,
          histogram_range Range = histogram_range::fixed>
class histogram
{
    static_assert(meta::is_arithmetic<T>::value,
                  "castle::statistic::histogram: T must be an arithmetic type");
    static_assert(BinCount > 0U,
                  "castle::statistic::histogram: BinCount must be greater than zero");
    static_assert(meta::is_floating_point<TCount>::value
                      || (meta::is_integral<TCount>::value && meta::is_unsigned<TCount>::value
                          && !meta::is_same<TCount, bool>::value),
                  "castle::statistic::histogram: TCount must be an unsigned integer or floating-point type");
    static_assert(Range == histogram_range::fixed || (BinCount >= 2U && (BinCount % 2U) == 0U),
                  "castle::statistic::histogram: an expanding histogram needs an even BinCount");

public:
    using value_type  = T;
    using result_type = result_t<T>;
    using count_type  = TCount;
    using size_type   = castle::size_type;

    /** @brief Range policy selected at compile time. */
    static CASTLE_CONSTEXPR histogram_range range_policy = Range;

    /** @brief Constructs an unconfigured histogram; `push()` reports `status::not_configured`. */
    histogram() CASTLE_NOEXCEPT
        : lower_(static_cast<result_type>(0)), upper_(static_cast<result_type>(0)), configured_(false)
    {
        clear();
    }

    /**
     * @brief Constructs and configures a histogram over `[lower, upper)`.
     * @note Invalid bounds leave the histogram unconfigured; check `configured()`.
     */
    histogram(result_type lower, result_type upper) CASTLE_NOEXCEPT
        : lower_(static_cast<result_type>(0)), upper_(static_cast<result_type>(0)), configured_(false)
    {
        (void)configure(lower, upper);
    }

    /**
     * @brief Sets the range `[lower, upper)` and discards all weight.
     * @return `status::ok`, or `status::invalid_argument` when a bound is not finite,
     *         `lower >= upper`, or the width overflows (the histogram becomes unconfigured).
     * @note Complexity: O(BinCount).
     */
    castle::status configure(result_type lower, result_type upper) CASTLE_NOEXCEPT
    {
        clear();
        if (!(is_finite(lower) && is_finite(upper) && (lower < upper) && is_finite(upper - lower)))
        {
            lower_ = static_cast<result_type>(0);
            upper_ = static_cast<result_type>(0);
            configured_ = false;
            return castle::status::invalid_argument;
        }
        lower_ = lower;
        upper_ = upper;
        configured_ = true;
        return castle::status::ok;
    }

    /** @brief Reports whether a valid range is set. */
    bool configured() CASTLE_CONST CASTLE_NOEXCEPT { return configured_; }

    /** @brief Inclusive lower bound of the range (grows in `expanding` mode). */
    result_type lower() CASTLE_CONST CASTLE_NOEXCEPT { return lower_; }

    /** @brief Exclusive upper bound of the range (grows in `expanding` mode). */
    result_type upper() CASTLE_CONST CASTLE_NOEXCEPT { return upper_; }

    /** @brief Width of each bin; zero when unconfigured. */
    result_type bin_width() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (upper_ - lower_) / static_cast<result_type>(BinCount);
    }

    /** @brief Number of bins. */
    static CASTLE_CONSTEXPR size_type bin_count() CASTLE_NOEXCEPT { return BinCount; }

    /** @brief Total weight, including underflow and overflow. */
    count_type count() CASTLE_CONST CASTLE_NOEXCEPT { return total_; }

    /** @brief Reports whether no weight is held. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return total_ == static_cast<count_type>(0); }

    /** @brief Weight of values below `lower()`. */
    count_type underflow() CASTLE_CONST CASTLE_NOEXCEPT { return underflow_; }

    /** @brief Weight of values at or above `upper()`. */
    count_type overflow() CASTLE_CONST CASTLE_NOEXCEPT { return overflow_; }

    /**
     * @brief Adds a sample with a positive weight.
     * @param value Sample value.
     * @param weight Positive, finite weight; 1 counts one observation.
     * @return `status::ok`, `status::not_configured`, `status::invalid_argument`
     *         (non-finite value, or non-positive/non-finite weight), or
     *         `status::full` when an integral total would overflow.
     * @note Complexity: O(1) for `fixed`. For `expanding`, O(1) unless the range
     *       grows: each doubling costs O(BinCount), and a finite value needs at
     *       most about 2100 doublings (the double exponent range).
     */
    castle::status push(T value, count_type weight = static_cast<count_type>(1)) CASTLE_NOEXCEPT
    {
        if (!configured_)
        {
            return castle::status::not_configured;
        }
        if (!valid_input(value, weight))
        {
            return castle::status::invalid_argument;
        }
        CASTLE_IF_CONSTEXPR (meta::is_integral<count_type>::value)
        {
            if (weight > numeric_limits<count_type>::max() - total_)
            {
                return castle::status::full;
            }
        }
        CASTLE_CONST result_type x = static_cast<result_type>(value);
        CASTLE_IF_CONSTEXPR (Range == histogram_range::expanding)
        {
            expand_to(x);
        }
        count_type& slot = slot_for(x);
        slot = static_cast<count_type>(slot + weight);
        total_ = static_cast<count_type>(total_ + weight);
        return castle::status::ok;
    }

    /**
     * @brief Adds every sample of an iterator range with weight 1.
     * @return `status::ok`, or the first failing `push()` status.
     * @note Complexity: O(N) for `fixed`.
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
     * @brief Retracts weight previously added for a value. Only for `fixed` histograms.
     * @return `status::ok`, `status::not_configured`, `status::invalid_argument`, or
     *         `status::not_found` when the value's bin holds less than `weight`.
     * @note Complexity: O(1).
     */
    castle::status remove(T value, count_type weight = static_cast<count_type>(1)) CASTLE_NOEXCEPT
    {
        static_assert(Range == histogram_range::fixed,
                      "castle::statistic::histogram: remove() needs a fixed range");
        if (!configured_)
        {
            return castle::status::not_configured;
        }
        if (!valid_input(value, weight))
        {
            return castle::status::invalid_argument;
        }
        count_type& slot = slot_for(static_cast<result_type>(value));
        if (slot < weight)
        {
            return castle::status::not_found;
        }
        slot = static_cast<count_type>(slot - weight);
        total_ = static_cast<count_type>(total_ - weight);
        return castle::status::ok;
    }

    /** @brief Discards all weight; the range is kept. Complexity: O(BinCount). */
    void clear() CASTLE_NOEXCEPT
    {
        for (size_type i = 0U; i < BinCount; ++i)
        {
            bins_[i] = static_cast<count_type>(0);
        }
        underflow_ = static_cast<count_type>(0);
        overflow_ = static_cast<count_type>(0);
        total_ = static_cast<count_type>(0);
    }

    /**
     * @brief Weight held by one bin.
     * @return `status::ok`, or `status::out_of_range` for `index >= BinCount`.
     */
    castle::status bin(size_type index, count_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (index >= BinCount)
        {
            return castle::status::out_of_range;
        }
        out = bins_[index];
        return castle::status::ok;
    }

    /**
     * @brief Edges `[lower, upper)` of one bin.
     * @return `status::ok`, `status::not_configured`, or `status::out_of_range`.
     */
    castle::status bin_edges(size_type index, result_type& lower, result_type& upper) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!configured_)
        {
            return castle::status::not_configured;
        }
        if (index >= BinCount)
        {
            return castle::status::out_of_range;
        }
        lower = edge(index);
        upper = edge(index + 1U);
        return castle::status::ok;
    }

    /**
     * @brief Mode estimate: centre of the heaviest bin (the lowest one on ties).
     * @return `status::ok`, `status::not_configured`, or `status::empty` when no
     *         in-range weight is held.
     * @note Complexity: O(BinCount).
     */
    castle::status mode(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!configured_)
        {
            return castle::status::not_configured;
        }
        size_type best = 0U;
        for (size_type i = 1U; i < BinCount; ++i)
        {
            if (bins_[i] > bins_[best])
            {
                best = i;
            }
        }
        if (bins_[best] == static_cast<count_type>(0))
        {
            return castle::status::empty;
        }
        out = (edge(best) + edge(best + 1U)) * static_cast<result_type>(0.5);
        return castle::status::ok;
    }

    /**
     * @brief Approximate quantile, interpolating linearly inside the bin that holds it.
     * @param probability Quantile in [0, 1].
     * @param out Receives the estimate; `lower()` or `upper()` when it falls in the
     *            underflow or overflow weight.
     * @return `status::ok`, `status::not_configured`, `status::invalid_argument`, or `status::empty`.
     * @note Complexity: O(BinCount).
     */
    castle::status quantile(result_type probability, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!configured_)
        {
            return castle::status::not_configured;
        }
        if (!is_probability(probability))
        {
            return castle::status::invalid_argument;
        }
        if (empty())
        {
            return castle::status::empty;
        }
        CASTLE_CONST result_type target = probability * static_cast<result_type>(total_);
        result_type cumulative = static_cast<result_type>(underflow_);
        if ((underflow_ > static_cast<count_type>(0)) && (target <= cumulative))
        {
            out = lower_;
            return castle::status::ok;
        }
        for (size_type i = 0U; i < BinCount; ++i)
        {
            CASTLE_CONST result_type weight = static_cast<result_type>(bins_[i]);
            if ((bins_[i] > static_cast<count_type>(0)) && (target <= cumulative + weight))
            {
                CASTLE_CONST result_type fraction = (target - cumulative) / weight;
                out = lower_ + (static_cast<result_type>(i) + fraction) * bin_width();
                return castle::status::ok;
            }
            cumulative += weight;
        }
        out = upper_;
        return castle::status::ok;
    }

    /**
     * @brief Approximate CDF: fraction of the weight at or below `value`.
     * @return `status::ok`, `status::not_configured`, `status::invalid_argument` for a
     *         non-finite value, or `status::empty`.
     * @note Below `lower()` the result is 0 and at/above `upper()` it is
     *       `1 - overflow / count`: lower bounds, since out-of-range values are not located.
     * @note Complexity: O(BinCount).
     */
    castle::status cdf(result_type value, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!configured_)
        {
            return castle::status::not_configured;
        }
        if (!is_finite(value))
        {
            return castle::status::invalid_argument;
        }
        if (empty())
        {
            return castle::status::empty;
        }
        CASTLE_CONST result_type total = static_cast<result_type>(total_);
        if (value < lower_)
        {
            out = static_cast<result_type>(0);
            return castle::status::ok;
        }
        if (value >= upper_)
        {
            out = (total - static_cast<result_type>(overflow_)) / total;
            return castle::status::ok;
        }
        CASTLE_CONST size_type index = bin_index(value);
        result_type cumulative = static_cast<result_type>(underflow_);
        for (size_type i = 0U; i < index; ++i)
        {
            cumulative += static_cast<result_type>(bins_[i]);
        }
        CASTLE_CONST result_type fraction = castle::math::clamp((value - edge(index)) / bin_width(),
                                                                static_cast<result_type>(0),
                                                                static_cast<result_type>(1));
        out = (cumulative + fraction * static_cast<result_type>(bins_[index])) / total;
        return castle::status::ok;
    }

private:
    /** Finite value and positive finite weight; non-finite checks exist only for floating-point types. */
    static bool valid_input(T value, count_type weight) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (meta::is_floating_point<T>::value)
        {
            if (!is_finite(value))
            {
                return false;
            }
        }
        CASTLE_IF_CONSTEXPR (meta::is_floating_point<count_type>::value)
        {
            if (!is_finite(weight))
            {
                return false;
            }
        }
        return weight > static_cast<count_type>(0);
    }

    result_type edge(size_type index) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return lower_ + static_cast<result_type>(index) * bin_width();
    }

    /** Bin of an in-range value; rounding just below `upper_` is clamped to the last bin. */
    size_type bin_index(result_type x) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST result_type scaled = (x - lower_) / (upper_ - lower_) * static_cast<result_type>(BinCount);
        return castle::min(static_cast<size_type>(scaled), BinCount - 1U);
    }

    count_type& slot_for(result_type x) CASTLE_NOEXCEPT
    {
        if (x < lower_)
        {
            return underflow_;
        }
        if (x >= upper_)
        {
            return overflow_;
        }
        return bins_[bin_index(x)];
    }

    /**
     * Doubles the range away from `x` until it contains `x`. Stops early, leaving
     * `x` to the underflow/overflow counters, when the doubled range would no
     * longer be finite.
     */
    void expand_to(result_type x) CASTLE_NOEXCEPT
    {
        while (x >= upper_)
        {
            CASTLE_CONST result_type width = (upper_ - lower_) * static_cast<result_type>(2);
            CASTLE_CONST result_type next_upper = lower_ + width;
            if (!is_finite(width) || !is_finite(next_upper))
            {
                return;
            }
            for (size_type i = 0U; i < BinCount / 2U; ++i)
            {
                bins_[i] = static_cast<count_type>(bins_[2U * i] + bins_[2U * i + 1U]);
            }
            for (size_type i = BinCount / 2U; i < BinCount; ++i)
            {
                bins_[i] = static_cast<count_type>(0);
            }
            upper_ = next_upper;
        }
        while (x < lower_)
        {
            CASTLE_CONST result_type width = (upper_ - lower_) * static_cast<result_type>(2);
            CASTLE_CONST result_type next_lower = upper_ - width;
            if (!is_finite(width) || !is_finite(next_lower))
            {
                return;
            }
            for (size_type i = BinCount; i > BinCount / 2U; --i)
            {
                CASTLE_CONST size_type source = 2U * (i - 1U) - BinCount;
                bins_[i - 1U] = static_cast<count_type>(bins_[source] + bins_[source + 1U]);
            }
            for (size_type i = 0U; i < BinCount / 2U; ++i)
            {
                bins_[i] = static_cast<count_type>(0);
            }
            lower_ = next_lower;
        }
    }

    castle::dsa::array<count_type, BinCount> bins_{};
    count_type underflow_;
    count_type overflow_;
    count_type total_;
    result_type lower_;
    result_type upper_;
    bool configured_;
};

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_HISTOGRAM_HPP

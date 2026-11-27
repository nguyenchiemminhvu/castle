// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file t_digest.hpp
 * @brief Fixed-size merging t-digest: streaming quantile and CDF estimates in constant memory.
 *
 * Use `t_digest<T, Capacity>` when a stream of measurements is too long to store, but its
 * distribution matters: median and 95 % / 99 % percentiles of GNSS message jitter or position
 * error (CEP), latency percentiles of a bus or a task, sensor noise bands, or the fraction of
 * samples below a limit. The digest keeps at most `Capacity` weighted centroids plus a buffer of
 * `BufferSize` unmerged samples, whatever the stream length.
 *
 * The digest follows Dunning's merging t-digest ("Computing extremely accurate quantiles using
 * t-digests", 2019). Samples go to the buffer. When it is full, `compress()` sorts all entries by
 * mean and merges neighbours in one pass. The `k1` scale function `k(q) = delta / (2 pi) * asin(2q - 1)`
 * allows a centroid to span at most one unit of `k`: centroids are large near the median and
 * tiny near the tails, so extreme quantiles such as p99 stay accurate. With `delta = Capacity - 1`,
 * the pass never produces more than `Capacity` centroids.
 *
 * `quantile()` and `cdf()` interpolate linearly between centroid centers and the exact minimum
 * and maximum. While a stream has fewer samples than the merge thresholds allow, every centroid
 * holds one sample and the answers are exact interpolations of the sorted samples.
 *
 * @code
 * castle::dsa::t_digest<float, 64U> jitter; // 64 centroids + 64 buffered samples, 1 KiB
 * jitter.add(1.7F);                          // milliseconds, one call per GNSS message
 * float p95 = jitter.quantile(0.95F);
 * float below_2ms = jitter.cdf(2.0F);        // fraction of messages within 2 ms
 * @endcode
 */
#ifndef CASTLE_DSA_T_DIGEST_HPP
#define CASTLE_DSA_T_DIGEST_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/constants.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/type_ranges.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/algorithm.hpp"
#include "castle/dsa/array.hpp"
#include "castle/math/linalg/trigonometry.hpp"

#include <stdint.h>

namespace castle
{
namespace dsa
{

/**
 * @brief Fixed-size merging t-digest over `T` samples.
 * @tparam T Floating-point sample type: `float`, `double`, or `long double`.
 * @tparam Capacity Maximum number of merged centroids, at least 2. Sets the compression
 * `delta = Capacity - 1`: more centroids give smaller quantile errors.
 * @tparam BufferSize Number of samples buffered between merges, at least 1. A larger buffer makes
 * `add()` cheaper on average; it does not change the accuracy much.
 * @note Storage is one `dsa::array<centroid_type, Capacity + BufferSize>`, two sizes, one
 * `uint32_t` total weight, and the minimum and maximum. The digest is copyable.
 * @note `add()` is O(1) amortized plus one `compress()` every `BufferSize` samples.
 * `compress()` is O(n log n) for `n = Capacity + BufferSize` (heapsort, no extra memory) plus one
 * `asin` and one `sin` per output centroid. `quantile()`, `cdf()`, and `mean()` are O(Capacity)
 * after the merge.
 * @warning `quantile()` and `cdf()` call `compress()` first, so they are not `const`.
 */
template <typename T = float,
          size_type Capacity = 64U,
          size_type BufferSize = Capacity>
class t_digest
{
    static_assert(meta::is_floating_point<T>::value, "dsa::t_digest requires a floating-point sample type");
    static_assert(Capacity >= 2U, "dsa::t_digest requires at least two centroids");
    static_assert(BufferSize >= 1U, "dsa::t_digest requires a buffer of at least one sample");
    static_assert(Capacity <= static_cast<castle::size_type>(-1) / 2U - BufferSize, "dsa::t_digest is too large");

public:
    using value_type = T;
    using weight_type = uint32_t;
    using size_type = castle::size_type;

    /** @brief One cluster of samples: their mean and how many there are. */
    struct centroid_type
    {
        value_type mean;    /**< Mean of the samples in the cluster. */
        weight_type weight; /**< Number of samples in the cluster, at least 1. */
    };

    using const_iterator = CASTLE_CONST centroid_type*;

    /** @brief Constructs an empty digest. */
    t_digest() CASTLE_NOEXCEPT
        : entries_(), size_(0U), merged_(0U), total_(0U), min_(0), max_(0)
    {
    }

    /** @brief Returns the maximum number of merged centroids, `Capacity`. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return Capacity;
    }

    /** @brief Returns the number of samples buffered between merges, `BufferSize`. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR size_type buffer_size() CASTLE_NOEXCEPT
    {
        return BufferSize;
    }

    /** @brief Returns the compression `delta = Capacity - 1` of the `k1` scale function. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR value_type compression() CASTLE_NOEXCEPT
    {
        return static_cast<value_type>(Capacity - 1U);
    }

    /** @brief Returns the largest total weight, `UINT32_MAX`. */
    CASTLE_NODISCARD static CASTLE_CONSTEXPR weight_type max_weight() CASTLE_NOEXCEPT
    {
        return castle::numeric_limits<weight_type>::max();
    }

    /**
     * @brief Adds a sample.
     * @param value Sample value. Must be finite.
     * @param weight Number of times `value` occurred. Zero changes nothing.
     * @return `status::ok`, `status::invalid_argument` when `value` is NaN or infinite, or
     * `status::full` when `total() + weight` would exceed `max_weight()`. The digest is unchanged
     * on failure.
     * @note Complexity: O(1), plus one `compress()` when the buffer is full.
     */
    castle::status add(value_type value, weight_type weight = 1U) CASTLE_NOEXCEPT
    {
        // NaN and infinities are the only values for which `value - value` is not zero.
        if ((value - value) != static_cast<value_type>(0))
        {
            return castle::status::invalid_argument;
        }
        if (weight == 0U)
        {
            return castle::status::ok;
        }
        if (weight > max_weight() - total_)
        {
            return castle::status::full;
        }

        if (total_ == 0U)
        {
            min_ = value;
            max_ = value;
        }
        else if (value < min_)
        {
            min_ = value;
        }
        else if (value > max_)
        {
            max_ = value;
        }
        append(centroid_type{value, weight});
        return castle::status::ok;
    }

    /**
     * @brief Adds every sample of another digest.
     * @param other Digest to add; may be `*this`, which doubles every weight.
     * @return `status::ok`, or `status::full` when `total() + other.total()` would exceed
     * `max_weight()`. The digest is unchanged on failure.
     * @note The minimum and maximum stay exact. Complexity: O(other.size()) plus the merges it triggers.
     */
    castle::status merge(CASTLE_CONST t_digest& other) CASTLE_NOEXCEPT
    {
        if (other.total_ > max_weight() - total_)
        {
            return castle::status::full;
        }
        if (other.total_ == 0U)
        {
            return castle::status::ok;
        }
        if (&other == this)
        {
            for (size_type i = 0U; i < size_; ++i)
            {
                entries_[i].weight = static_cast<weight_type>(entries_[i].weight * 2U);
            }
            total_ = static_cast<weight_type>(total_ * 2U);
            return castle::status::ok;
        }

        if (total_ == 0U)
        {
            min_ = other.min_;
            max_ = other.max_;
        }
        else
        {
            min_ = (other.min_ < min_) ? other.min_ : min_;
            max_ = (other.max_ > max_) ? other.max_ : max_;
        }
        for (size_type i = 0U; i < other.size_; ++i)
        {
            append(other.entries_[i]);
        }
        return castle::status::ok;
    }

    /**
     * @brief Merges the buffered samples into the centroids.
     * @note Afterwards `begin() .. end()` holds at most `Capacity` centroids sorted by mean.
     * Does nothing when no sample is buffered. Complexity: O(n log n) for `n = size()`.
     */
    void compress() CASTLE_NOEXCEPT
    {
        if (merged_ == size_)
        {
            return;
        }

        castle::sort(entries_.data(), entries_.data() + size_, mean_less());

        CASTLE_CONST value_type total = static_cast<value_type>(total_);
        size_type out = 0U;
        weight_type before = 0U; // weight of the centroids before entries_[out]
        value_type limit = weight_limit(before, total);
        for (size_type i = 1U; i < size_; ++i)
        {
            centroid_type& current = entries_[out];
            CASTLE_CONST centroid_type next = entries_[i];
            CASTLE_CONST weight_type proposed = current.weight + next.weight;
            // The k1 bound keeps `out < Capacity`; the second test only guards against rounding.
            if ((static_cast<value_type>(before + proposed) <= limit) || (out + 1U == Capacity)) // LCOV_EXCL_BR_LINE
            {
                current.weight = proposed;
                current.mean += (next.mean - current.mean) * static_cast<value_type>(next.weight) / static_cast<value_type>(proposed);
            }
            else
            {
                before += current.weight;
                ++out;
                entries_[out] = next;
                limit = weight_limit(before, total);
            }
        }
        size_ = out + 1U;
        merged_ = size_;
    }

    /**
     * @brief Estimates the value below which a fraction `q` of the samples falls.
     * @param q Quantile in `[0, 1]`, for example `0.95` for p95. Values outside are clamped; NaN
     * counts as 0.
     * @return Estimated quantile: `min()` for `q = 0`, `max()` for `q = 1`. Zero when empty.
     * @note Interpolates linearly between `(0, min)`, the centroid centers, and `(total, max)`
     * in rank space. Ranks use `double`, so they are exact for every total. Complexity:
     * O(Capacity) after `compress()`.
     */
    value_type quantile(value_type q) CASTLE_NOEXCEPT
    {
        compress();
        if (total_ == 0U)
        {
            return static_cast<value_type>(0);
        }

        value_type fraction = static_cast<value_type>(0);
        if (q > fraction)
        {
            fraction = (q < static_cast<value_type>(1)) ? q : static_cast<value_type>(1);
        }
        CASTLE_CONST double target = static_cast<double>(fraction) * static_cast<double>(total_);
        double previous_rank = 0.0;
        value_type previous_value = min_;
        weight_type before = 0U;
        for (size_type i = 0U; i < size_; ++i)
        {
            CASTLE_CONST centroid_type& centroid = entries_[i];
            CASTLE_CONST double center = static_cast<double>(before) + 0.5 * static_cast<double>(centroid.weight);
            if (target <= center)
            {
                return lerp(previous_value, centroid.mean, (target - previous_rank) / (center - previous_rank));
            }
            previous_rank = center;
            previous_value = centroid.mean;
            before += centroid.weight;
        }
        return lerp(previous_value, max_, (target - previous_rank) / (static_cast<double>(total_) - previous_rank));
    }

    /**
     * @brief Estimates the fraction of samples at or below a value.
     * @param value Value to look up.
     * @return Fraction in `[0, 1]`: 0 below `min()` (and for NaN), 1 at or above `max()`. Zero when empty.
     * @note Inverse of `quantile()`: interpolates the rank between neighbouring centroid means.
     * Complexity: O(Capacity) after `compress()`.
     */
    value_type cdf(value_type value) CASTLE_NOEXCEPT
    {
        compress();
        if ((total_ == 0U) || !(value >= min_))
        {
            return static_cast<value_type>(0);
        }
        if (value >= max_)
        {
            return static_cast<value_type>(1);
        }

        CASTLE_CONST double total = static_cast<double>(total_);
        double previous_rank = 0.0;
        value_type previous_value = min_;
        weight_type before = 0U;
        for (size_type i = 0U; i < size_; ++i)
        {
            CASTLE_CONST centroid_type& centroid = entries_[i];
            CASTLE_CONST double center = static_cast<double>(before) + 0.5 * static_cast<double>(centroid.weight);
            if (value < centroid.mean)
            {
                // previous_value <= value < centroid.mean, so the divisor is positive.
                CASTLE_CONST double share = static_cast<double>((value - previous_value) / (centroid.mean - previous_value));
                return static_cast<value_type>((previous_rank + share * (center - previous_rank)) / total);
            }
            previous_rank = center;
            previous_value = centroid.mean;
            before += centroid.weight;
        }
        CASTLE_CONST double share = static_cast<double>((value - previous_value) / (max_ - previous_value));
        return static_cast<value_type>((previous_rank + share * (total - previous_rank)) / total);
    }

    /**
     * @brief Returns the mean of every sample, including buffered ones.
     * @return Weighted mean of all entries; zero when empty. Complexity: O(size()).
     */
    CASTLE_NODISCARD value_type mean() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (total_ == 0U)
        {
            return static_cast<value_type>(0);
        }
        value_type sum = static_cast<value_type>(0);
        for (size_type i = 0U; i < size_; ++i)
        {
            sum += entries_[i].mean * static_cast<value_type>(entries_[i].weight);
        }
        return sum / static_cast<value_type>(total_);
    }

    /** @brief Returns the smallest sample, exact. Zero when empty. */
    CASTLE_NODISCARD value_type min() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return min_;
    }

    /** @brief Returns the largest sample, exact. Zero when empty. */
    CASTLE_NODISCARD value_type max() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return max_;
    }

    /** @brief Returns the total weight: the number of samples added, `0 .. max_weight()`. */
    CASTLE_NODISCARD weight_type total() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return total_;
    }

    /** @brief Reports whether no sample was added since construction or `clear()`. */
    CASTLE_NODISCARD bool empty() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return total_ == 0U;
    }

    /**
     * @brief Returns the number of stored entries: merged centroids plus buffered samples.
     * @note After `compress()`, this is the centroid count, at most `Capacity`.
     */
    CASTLE_NODISCARD size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return size_;
    }

    /** @brief Forgets every sample. Complexity: O(1). */
    void clear() CASTLE_NOEXCEPT
    {
        size_ = 0U;
        merged_ = 0U;
        total_ = 0U;
        min_ = static_cast<value_type>(0);
        max_ = static_cast<value_type>(0);
    }

    /**
     * @brief Returns the first stored entry.
     * @note Call `compress()` first to iterate only merged centroids, sorted by mean. Each entry
     * can be replayed with `add(mean, weight)`, for example to persist or transfer the digest.
     */
    CASTLE_NODISCARD const_iterator begin() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return entries_.data();
    }

    /** @brief Returns one past the last stored entry. */
    CASTLE_NODISCARD const_iterator end() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return entries_.data() + size_;
    }

private:
    static CASTLE_CONSTEXPR size_type slot_count = Capacity + BufferSize;

    /** @brief Orders entries by ascending mean, for `castle::sort`. */
    struct mean_less
    {
        bool operator()(CASTLE_CONST centroid_type& lhs, CASTLE_CONST centroid_type& rhs) CASTLE_CONST CASTLE_NOEXCEPT
        {
            return lhs.mean < rhs.mean;
        }
    };

    /** @brief Stores one entry, merging first when every slot is used. */
    void append(CASTLE_CONST centroid_type& entry) CASTLE_NOEXCEPT
    {
        // Count the weight only once the entry is stored: compress() normalizes by total_.
        if (size_ == slot_count)
        {
            compress();
        }
        entries_[size_] = entry;
        ++size_;
        total_ += entry.weight;
    }

    /**
     * @brief Returns the largest cumulative weight the centroid starting after `before` may reach.
     * @return `total * q_limit` with `q_limit = k^-1(k(before / total) + 1)` for the `k1` scale,
     * or `total` when the centroid may run to the end.
     */
    static value_type weight_limit(weight_type before, value_type total) CASTLE_NOEXCEPT
    {
        CASTLE_CONST value_type one = static_cast<value_type>(1);
        CASTLE_CONST value_type delta = compression();
        CASTLE_CONST value_type q = static_cast<value_type>(before) / total;
        CASTLE_CONST value_type k = delta / math::tau<value_type>() * math::asin(static_cast<value_type>(2) * q - one) + one;
        if (k >= delta / static_cast<value_type>(4))
        {
            return total;
        }
        return total * (math::sin(k * math::tau<value_type>() / delta) + one) / static_cast<value_type>(2);
    }

    static value_type lerp(value_type from, value_type to, double share) CASTLE_NOEXCEPT
    {
        return from + (to - from) * static_cast<value_type>(share);
    }

    array<centroid_type, slot_count> entries_;
    size_type size_;   // entries in use: [0, merged_) centroids, [merged_, size_) buffered samples
    size_type merged_; // entries sorted and merged by the last compress()
    weight_type total_;
    value_type min_;
    value_type max_;
};

} // namespace dsa
} // namespace castle

#endif // CASTLE_DSA_T_DIGEST_HPP

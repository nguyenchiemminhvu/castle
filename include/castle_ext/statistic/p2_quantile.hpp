// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file p2_quantile.hpp
 * @brief P² streaming estimator of one quantile in O(1) time and memory.
 *
 * Implements the P² ("P-square") algorithm of Jain & Chlamtac (1985). Five
 * markers track the minimum, the target quantile, two intermediate quantiles,
 * and the maximum. Each sample moves the marker positions; marker heights are
 * corrected with a piecewise-parabolic (or, when that would break ordering,
 * linear) prediction. No sample is stored, so the estimator suits unbounded
 * streams where a window or histogram is too large or the value range is
 * unknown. Up to and including the fifth sample, the exact (linear) quantile
 * of the samples seen so far is reported.
 *
 * @code
 * #include "castle_ext/statistic/p2_quantile.hpp"
 *
 * castle::statistic::p2_quantile<double> p95(0.95);
 * for (int i = 1; i <= 100; ++i)
 * {
 *     p95.push(static_cast<double>(i));
 * }
 * double estimate = 0.0;
 * if (p95.quantile(estimate) == castle::status::ok)
 * {
 *     // estimate is close to 95
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_P2_QUANTILE_HPP
#define CASTLE_EXT_STATISTIC_P2_QUANTILE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/array.hpp"
#include "castle_ext/statistic/validation.hpp"
#include "castle_ext/statistic/order_statistics.hpp"

namespace castle
{
namespace statistic
{

/**
 * @brief Streaming estimate of the `p`-quantile with five markers.
 * @tparam T Arithmetic sample type.
 * @note Storage: 20 values of `result_t<T>` plus a counter and flags; no sample history.
 * @note Accuracy improves with the sample count; extreme quantiles (p near 0 or 1)
 *       need many samples to settle. Use `sorted_window` when exact values are required.
 * @warning Not thread-safe; protect shared instances externally.
 */
template <typename T = double>
class p2_quantile
{
    static_assert(meta::is_arithmetic<T>::value,
                  "castle::statistic::p2_quantile: T must be an arithmetic type");

public:
    using value_type  = T;
    using result_type = result_t<T>;
    using size_type   = castle::size_type;

    /** @brief Constructs an unconfigured estimator; `push()` reports `status::not_configured`. */
    p2_quantile() CASTLE_NOEXCEPT
        : probability_(static_cast<result_type>(0)), count_(0U), configured_(false)
    {
    }

    /**
     * @brief Constructs an estimator for the `probability`-quantile.
     * @note An invalid probability leaves it unconfigured; check `configured()`.
     */
    explicit p2_quantile(result_type probability) CASTLE_NOEXCEPT
        : probability_(static_cast<result_type>(0)), count_(0U), configured_(false)
    {
        (void)configure(probability);
    }

    /**
     * @brief Selects the estimated quantile and discards every sample.
     * @param probability Quantile in the open interval (0, 1).
     * @return `status::ok`, or `status::invalid_config` (the estimator becomes unconfigured).
     */
    castle::status configure(result_type probability) CASTLE_NOEXCEPT
    {
        count_ = 0U;
        if (!((probability > static_cast<result_type>(0)) && (probability < static_cast<result_type>(1))))
        {
            configured_ = false;
            probability_ = static_cast<result_type>(0);
            return castle::status::invalid_config;
        }
        probability_ = probability;
        configured_ = true;
        return castle::status::ok;
    }

    /** @brief Reports whether a valid probability is set. */
    bool configured() CASTLE_CONST CASTLE_NOEXCEPT { return configured_; }

    /** @brief Estimated quantile probability. */
    result_type probability() CASTLE_CONST CASTLE_NOEXCEPT { return probability_; }

    /** @brief Number of samples seen (saturates at the maximum `size_type`). */
    size_type count() CASTLE_CONST CASTLE_NOEXCEPT { return count_; }

    /** @brief Reports whether no sample was seen. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return count_ == 0U; }

    /**
     * @brief Feeds one sample.
     * @return `status::ok`, `status::not_configured`, or `status::invalid_argument` for NaN or infinity.
     * @note Complexity: O(1) time and stack.
     */
    castle::status push(T sample) CASTLE_NOEXCEPT
    {
        if (!configured_)
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
        CASTLE_CONST result_type x = static_cast<result_type>(sample);
        if (count_ < marker_count)
        {
            insert_initial(x);
            return castle::status::ok;
        }
        update(x);
        if (count_ != max_count) // LCOV_EXCL_BR_LINE: saturation needs 2^N samples.
        {
            ++count_;
        }
        return castle::status::ok;
    }

    /**
     * @brief Current estimate of the configured quantile.
     * @return `status::ok`, `status::not_configured`, or `status::empty` without samples.
     * @note Complexity: O(1).
     */
    castle::status quantile(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!configured_)
        {
            return castle::status::not_configured;
        }
        if (count_ <= marker_count)
        {
            return quantile_sorted(heights_.data(), heights_.data() + count_, probability_, out);
        }
        out = heights_[2U];
        return castle::status::ok;
    }

    /** @brief Discards every sample; the probability is kept. */
    void clear() CASTLE_NOEXCEPT
    {
        count_ = 0U;
    }

private:
    static CASTLE_CONSTEXPR size_type marker_count = 5U;
    static CASTLE_CONSTEXPR size_type max_count = static_cast<size_type>(-1);

    /** Sorted insertion of the first five samples; the fifth initializes the markers. */
    void insert_initial(result_type x) CASTLE_NOEXCEPT
    {
        size_type i = count_;
        while ((i > 0U) && (heights_[i - 1U] > x))
        {
            heights_[i] = heights_[i - 1U];
            --i;
        }
        heights_[i] = x;
        ++count_;
        if (count_ == marker_count)
        {
            CASTLE_CONST result_type p = probability_;
            CASTLE_CONST result_type one = static_cast<result_type>(1);
            CASTLE_CONST result_type two = static_cast<result_type>(2);
            for (size_type m = 0U; m < marker_count; ++m)
            {
                positions_[m] = static_cast<result_type>(m);
            }
            desired_[0U] = static_cast<result_type>(0);
            desired_[1U] = two * p;
            desired_[2U] = static_cast<result_type>(4) * p;
            desired_[3U] = two + two * p;
            desired_[4U] = static_cast<result_type>(4);
            increments_[0U] = static_cast<result_type>(0);
            increments_[1U] = p / two;
            increments_[2U] = p;
            increments_[3U] = (one + p) / two;
            increments_[4U] = one;
        }
    }

    void update(result_type x) CASTLE_NOEXCEPT
    {
        // Locate the cell k with heights_[k] <= x < heights_[k + 1], widening the extremes.
        size_type k = 0U;
        if (x < heights_[0U])
        {
            heights_[0U] = x;
        }
        else if (x >= heights_[4U])
        {
            heights_[4U] = x;
            k = 3U;
        }
        else
        {
            while (x >= heights_[k + 1U])
            {
                ++k;
            }
        }
        for (size_type m = k + 1U; m < marker_count; ++m)
        {
            positions_[m] += static_cast<result_type>(1);
        }
        for (size_type m = 0U; m < marker_count; ++m)
        {
            desired_[m] += increments_[m];
        }
        for (size_type m = 1U; m < marker_count - 1U; ++m)
        {
            adjust(m);
        }
    }

    /** Moves marker `m` by one position toward its desired position when it lags by at least one. */
    void adjust(size_type m) CASTLE_NOEXCEPT
    {
        CASTLE_CONST result_type one = static_cast<result_type>(1);
        CASTLE_CONST result_type drift = desired_[m] - positions_[m];
        CASTLE_CONST bool move_up = (drift >= one) && (positions_[m + 1U] - positions_[m] > one);
        CASTLE_CONST bool move_down = (drift <= -one) && (positions_[m - 1U] - positions_[m] < -one);
        if (!(move_up || move_down))
        {
            return;
        }
        CASTLE_CONST result_type step = move_up ? one : -one;
        CASTLE_CONST result_type candidate = parabolic(m, step);
        if ((heights_[m - 1U] < candidate) && (candidate < heights_[m + 1U]))
        {
            heights_[m] = candidate;
        }
        else
        {
            heights_[m] = linear(m, move_up);
        }
        positions_[m] += step;
    }

    /** Piecewise-parabolic (P²) height prediction for moving marker `m` by `step`. */
    result_type parabolic(size_type m, result_type step) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST result_type n_prev = positions_[m - 1U];
        CASTLE_CONST result_type n_curr = positions_[m];
        CASTLE_CONST result_type n_next = positions_[m + 1U];
        CASTLE_CONST result_type q_prev = heights_[m - 1U];
        CASTLE_CONST result_type q_curr = heights_[m];
        CASTLE_CONST result_type q_next = heights_[m + 1U];
        return q_curr + step / (n_next - n_prev)
                            * ((n_curr - n_prev + step) * (q_next - q_curr) / (n_next - n_curr)
                               + (n_next - n_curr - step) * (q_curr - q_prev) / (n_curr - n_prev));
    }

    /** Linear height prediction toward the neighbour in the direction of the move. */
    result_type linear(size_type m, bool up) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST size_type neighbour = up ? m + 1U : m - 1U;
        return heights_[m] + (heights_[neighbour] - heights_[m])
                                 / (positions_[neighbour] - positions_[m])
                                 * (up ? static_cast<result_type>(1) : static_cast<result_type>(-1));
    }

    castle::dsa::array<result_type, 5U> heights_{};
    castle::dsa::array<result_type, 5U> positions_{};
    castle::dsa::array<result_type, 5U> desired_{};
    castle::dsa::array<result_type, 5U> increments_{};
    result_type probability_;
    size_type count_;
    bool configured_;
};

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_P2_QUANTILE_HPP

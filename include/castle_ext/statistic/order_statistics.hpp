// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file order_statistics.hpp
 * @brief Exact batch quantiles, median, interquartile range, and mode.
 *
 * The `*_sorted` functions read an ascending, finite range in O(1) (mode in
 * O(N)). `sort_samples()` validates and sorts a caller-owned range in place
 * with `castle::sort` (heapsort: O(N log N) worst case, O(1) stack), and
 * `quantile_inplace()` combines both steps for a one-off query. No function
 * allocates or keeps hidden workspace.
 *
 * Two quantile definitions are offered (Hyndman & Fan, 1996):
 * - `quantile_method::linear` (type 7, the default of R and NumPy) interpolates
 *   between the order statistics at position `p * (n - 1)`.
 * - `quantile_method::nearest_rank` (type 1) returns the smallest sample whose
 *   empirical CDF reaches `p`, so at least `p * n` samples are less than or
 *   equal to the result. GNSS test reports often use this "p% of fixes within
 *   r" convention.
 *
 * @code
 * #include "castle_ext/statistic/order_statistics.hpp"
 *
 * double errors[] = {2.0, 0.5, 1.5, 1.0};
 * double p95 = 0.0;
 * if (castle::statistic::quantile_inplace(errors, errors + 4, 0.95, p95) == castle::status::ok)
 * {
 *     // errors[] is now sorted; p95 == 1.925
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_ORDER_STATISTICS_HPP
#define CASTLE_EXT_STATISTIC_ORDER_STATISTICS_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/dsa/algorithm.hpp"
#include "castle_ext/statistic/validation.hpp"

#include <stdint.h>

namespace castle
{
namespace statistic
{

/**
 * @brief Quantile definitions for exact order statistics.
 */
enum class quantile_method : uint8_t
{
    linear = 0U,  /**< Hyndman–Fan type 7: linear interpolation at `p * (n - 1)`. */
    nearest_rank  /**< Hyndman–Fan type 1: smallest sample `x` with ECDF(x) >= p. */
};

namespace detail
{

/**
 * @brief Zero-based index of the nearest-rank quantile, `ceil(p * n) - 1`, clamped to `[0, n - 1]`.
 * @note A relative tolerance of 16 epsilon absorbs products such as `0.95 * 100`
 *       that round just above an integer, so they do not select the next rank.
 */
template <typename R>
CASTLE_CONSTEXPR size_type nearest_rank_index(R probability, size_type count) CASTLE_NOEXCEPT
{
    CASTLE_CONST R position = probability * static_cast<R>(count);
    size_type rank = static_cast<size_type>(position);
    CASTLE_CONST R tolerance = position * static_cast<R>(16) * meta::floating_epsilon<R>::value;
    if (position - static_cast<R>(rank) > tolerance)
    {
        ++rank;
    }
    return (rank == 0U) ? 0U : rank - 1U;
}

} // namespace detail

/**
 * @brief Quantile of an ascending, finite range.
 * @tparam TRandomIterator Random-access iterator or pointer.
 * @tparam R Floating-point result type (deduced from `out`).
 * @param first Iterator to the smallest sample.
 * @param last Iterator one past the largest sample.
 * @param probability Quantile in [0, 1]; 0.5 is the median.
 * @param out Receives the quantile on success; unchanged on failure.
 * @param method Quantile definition.
 * @return `status::ok`, `status::invalid_argument` for a probability outside
 *         [0, 1] or NaN, or `status::empty` for an empty range.
 * @note Complexity: O(1) time and stack.
 * @warning The range must be sorted ascending and contain only finite values.
 */
template <typename TRandomIterator, typename R>
castle::status quantile_sorted(TRandomIterator first,
                               TRandomIterator last,
                               meta::type_identity_t<R> probability,
                               R& out,
                               quantile_method method = quantile_method::linear) CASTLE_NOEXCEPT
{
    static_assert(meta::is_floating_point<R>::value,
                  "castle::statistic::quantile_sorted: R must be a floating-point type");
    if (!is_probability(probability))
    {
        return castle::status::invalid_argument;
    }
    CASTLE_CONST difference_type distance = last - first;
    if (distance <= 0)
    {
        return castle::status::empty;
    }
    CASTLE_CONST size_type count = static_cast<size_type>(distance);
    if (method == quantile_method::nearest_rank)
    {
        out = static_cast<R>(first[detail::nearest_rank_index(probability, count)]);
        return castle::status::ok;
    }
    CASTLE_CONST R position = probability * static_cast<R>(count - 1U);
    CASTLE_CONST size_type lower = static_cast<size_type>(position);
    if (lower + 1U >= count)
    {
        out = static_cast<R>(first[count - 1U]);
        return castle::status::ok;
    }
    CASTLE_CONST R low = static_cast<R>(first[lower]);
    CASTLE_CONST R high = static_cast<R>(first[lower + 1U]);
    out = low + (high - low) * (position - static_cast<R>(lower));
    return castle::status::ok;
}

/**
 * @brief Median of an ascending, finite range (linear quantile at 0.5).
 * @return `status::ok`, or `status::empty` for an empty range.
 * @note For an even count this is the mean of the two middle samples. Complexity: O(1).
 */
template <typename TRandomIterator, typename R>
castle::status median_sorted(TRandomIterator first, TRandomIterator last, R& out) CASTLE_NOEXCEPT
{
    return quantile_sorted(first, last, static_cast<R>(0.5), out);
}

/**
 * @brief Interquartile range `Q3 - Q1` of an ascending, finite range.
 * @return `status::ok`, or `status::empty` for an empty range.
 * @note Complexity: O(1).
 */
template <typename TRandomIterator, typename R>
castle::status interquartile_range_sorted(TRandomIterator first,
                                          TRandomIterator last,
                                          R& out,
                                          quantile_method method = quantile_method::linear) CASTLE_NOEXCEPT
{
    R q1 = static_cast<R>(0);
    R q3 = static_cast<R>(0);
    CASTLE_CONST castle::status result = quantile_sorted(first, last, static_cast<R>(0.25), q1, method);
    if (result != castle::status::ok)
    {
        return result;
    }
    (void)quantile_sorted(first, last, static_cast<R>(0.75), q3, method);
    out = q3 - q1;
    return castle::status::ok;
}

/**
 * @brief Most frequent value of an ascending range.
 * @tparam TForwardIterator Forward iterator or pointer.
 * @param out Receives the mode; ties resolve to the smallest value.
 * @return `status::ok`, or `status::empty` for an empty range.
 * @note Exact equality is used, so the mode is meaningful for discrete data
 *       (integers, quantized readings). Bin continuous data with `histogram`.
 * @note Complexity: O(N) time, O(1) stack.
 */
template <typename TForwardIterator, typename R>
castle::status mode_sorted(TForwardIterator first, TForwardIterator last, R& out) CASTLE_NOEXCEPT
{
    if (first == last)
    {
        return castle::status::empty;
    }
    TForwardIterator best = first;
    size_type best_count = 0U;
    while (first != last)
    {
        CASTLE_CONST TForwardIterator run = first;
        size_type run_count = 0U;
        while ((first != last) && (*first == *run))
        {
            ++run_count;
            ++first;
        }
        if (run_count > best_count)
        {
            best = run;
            best_count = run_count;
        }
    }
    out = static_cast<R>(*best);
    return castle::status::ok;
}

/**
 * @brief Validates and sorts a caller-owned range in place, ready for the `*_sorted` queries.
 * @tparam TRandomIterator Mutable random-access iterator or pointer.
 * @return `status::ok`, or `status::invalid_argument` when a value is NaN or
 *         infinite (the range is then left unmodified).
 * @note Complexity: O(N log N) time worst case (heapsort), O(1) stack.
 */
template <typename TRandomIterator>
castle::status sort_samples(TRandomIterator first, TRandomIterator last) CASTLE_NOEXCEPT
{
    using sample_type = meta::remove_cv_t<meta::remove_reference_t<decltype(*first)>>;
    CASTLE_IF_CONSTEXPR (meta::is_floating_point<sample_type>::value)
    {
        for (TRandomIterator it = first; it != last; ++it)
        {
            if (!is_finite(*it))
            {
                return castle::status::invalid_argument;
            }
        }
    }
    castle::sort(first, last);
    return castle::status::ok;
}

/**
 * @brief One-off exact quantile of an unsorted range; sorts the range in place.
 * @param probability Quantile in [0, 1].
 * @param out Receives the quantile on success; unchanged on failure.
 * @param method Quantile definition.
 * @return `status::ok`, `status::invalid_argument` (bad probability or a
 *         non-finite sample), or `status::empty`.
 * @note Complexity: O(N log N) time, O(1) stack. Sort once with `sort_samples()`
 *       and use the `*_sorted` functions when several statistics are needed.
 */
template <typename TRandomIterator, typename R>
castle::status quantile_inplace(TRandomIterator first,
                                TRandomIterator last,
                                meta::type_identity_t<R> probability,
                                R& out,
                                quantile_method method = quantile_method::linear) CASTLE_NOEXCEPT
{
    if (!is_probability(probability))
    {
        return castle::status::invalid_argument;
    }
    CASTLE_CONST castle::status result = sort_samples(first, last);
    if (result != castle::status::ok)
    {
        return result;
    }
    return quantile_sorted(first, last, probability, out, method);
}

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_ORDER_STATISTICS_HPP

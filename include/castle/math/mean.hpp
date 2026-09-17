// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file mean.hpp
 * @brief Running arithmetic-mean accumulator.
 *
 * This header provides a small stateful accumulator for computing an arithmetic
 * mean without heap allocation, exceptions, RTTI, or STL containers. Use it
 * when samples arrive incrementally or when an iterator pair can describe a
 * deterministic input range.
 *
 * Example:
 * @code
 * int values[] = {10, 20, 30};
 * castle::math::mean<int, long long> average(values, values + 3);
 * const double result = average.get_mean();
 * (void)result;
 * @endcode
 *
 * @note For integral inputs, choose `TCalc` wide enough for the expected sum.
 */
#ifndef CASTLE_MATH_MEAN_HPP
#define CASTLE_MATH_MEAN_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Accumulates samples and reports their arithmetic mean as `double`.
 * @tparam TInput Input sample type accepted by `add()`.
 * @tparam TCalc Accumulation type used for non-floating-point inputs.
 * @note Floating-point inputs always accumulate in `TInput`.
 * @warning `TCalc` must be wide enough to hold the running sum without
 *          overflow, and the internal sample counter is a `uint32_t`.
 */
template <typename TInput, typename TCalc = TInput>
class mean
{
private:
    using calc_t = meta::conditional_t<meta::is_floating_point<TInput>::value, TInput, TCalc>;

public:
    /**
     * @brief Constructs an empty accumulator.
     * @note The initial mean is `0.0` until samples are added.
     */
    CASTLE_CONSTEXPR mean() CASTLE_NOEXCEPT
    {
        clear();
    }

    /**
     * @brief Constructs an accumulator from an iterator range.
     * @tparam TIterator Iterator type for the input range.
     * @param first Iterator to the first sample.
     * @param last Iterator one past the final sample.
     * @note The range is consumed immediately by repeated calls to `add()`.
     * @warning The iterator operations used inside this constructor must be
     *          valid and deterministic for the supplied type.
     */
    template <typename TIterator>
    CASTLE_CONSTEXPR mean(TIterator first, TIterator last) CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(++first))
    {
        clear();
        add(first, last);
    }

    /**
     * @brief Adds one sample to the accumulator.
     * @param value Sample value to append.
     * @note The cached mean is invalidated and recomputed lazily on demand.
     */
    CASTLE_CONSTEXPR void add(TInput value) CASTLE_NOEXCEPT
    {
        sum_ += static_cast<calc_t>(value);
        ++counter_;
        recalculate_ = true;
    }

    /**
     * @brief Adds all samples in an iterator range.
     * @tparam TIterator Iterator type for the input range.
     * @param first Iterator to the first sample.
     * @param last Iterator one past the final sample.
     * @note Complexity is linear in the number of elements traversed.
     */
    template <typename TIterator>
    CASTLE_CONSTEXPR void add(TIterator first, TIterator last) CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(++first))
    {
        while (first != last)
        {
            add(*first);
            ++first;
        }
    }

    /**
     * @brief Adds one sample using function-call syntax.
     * @param value Sample value to append.
     * @note Equivalent to `add(value)`.
     */
    CASTLE_CONSTEXPR void operator()(TInput value) CASTLE_NOEXCEPT
    {
        add(value);
    }

    /**
     * @brief Adds an iterator range using function-call syntax.
     * @tparam TIterator Iterator type for the input range.
     * @param first Iterator to the first sample.
     * @param last Iterator one past the final sample.
     * @note Equivalent to `add(first, last)`.
     */
    template <typename TIterator>
    CASTLE_CONSTEXPR void operator()(TIterator first, TIterator last) CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(++first))
    {
        add(first, last);
    }

    /**
     * @brief Returns the current arithmetic mean.
     * @return Mean of all samples added so far as `double`.
     * @note An empty accumulator returns `0.0`.
     * @note The value is cached and recomputed only after a mutation.
     */
    CASTLE_CONSTEXPR double get_mean() CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (recalculate_) // LCOV_EXCL_BR_LINE
        {
            mean_value_ = 0.0;

            if (counter_ != 0U)
            {
                double n = static_cast<double>(counter_);
                mean_value_ = static_cast<double>(sum_) / n;
            }

            recalculate_ = false;
        }

        return mean_value_;
    }

    /**
     * @brief Converts the accumulator to its current mean.
     * @return Same value as `get_mean()`.
     * @note This is a convenience conversion for expression contexts.
     */
    CASTLE_CONSTEXPR operator double() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return get_mean();
    }

    /**
     * @brief Reports how many samples have been accumulated.
     * @return Number of samples seen so far.
     */
    [[nodiscard]] CASTLE_CONSTEXPR size_type count() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<size_type>(counter_);
    }

    /**
     * @brief Resets the accumulator to its empty state.
     * @note After `clear()`, `count()` returns zero and `get_mean()` returns
     *       `0.0` until new samples are added.
     */
    CASTLE_CONSTEXPR void clear() CASTLE_NOEXCEPT
    {
        sum_         = static_cast<calc_t>(0);
        counter_     = 0U;
        mean_value_  = 0.0;
        recalculate_ = true;
    }

private:
    calc_t                 sum_;
    uint32_t               counter_;
    CASTLE_MUTABLE double  mean_value_;
    CASTLE_MUTABLE bool    recalculate_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_MEAN_HPP

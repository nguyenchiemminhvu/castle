#ifndef CASTLE_MATH_MEAN_H
#define CASTLE_MATH_MEAN_H

#include "castle/core/compiler.h"
#include "castle/core/traits.h"
#include "castle/core/types.h"

namespace castle
{
namespace math
{

// -------------------------------------------------------------
// Running accumulator for computing statistical mean (average).
// TInput The type of elements being fed into the system.
// TCalc Internal variable storage type used to avoid overflows.
// -------------------------------------------------------------
template <typename TInput, typename TCalc = TInput>
class mean
{
private:
    using calc_t = meta::conditional_t<meta::is_floating_point<TInput>::value, TInput, TCalc>;

public:
    // Default constructor initializing an empty state
    CASTLE_CONSTEXPR mean() CASTLE_NOEXCEPT
    {
        clear();
    }

    // Iterator range constructor
    template <typename TIterator>
    CASTLE_CONSTEXPR mean(TIterator first, TIterator last) CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(++first))
    {
        clear();
        add(first, last);
    }

    // Inserts a single data point into the system
    CASTLE_CONSTEXPR void add(TInput value) CASTLE_NOEXCEPT
    {
        sum_ += static_cast<calc_t>(value);
        ++counter_;
        recalculate_ = true;
    }

    // Inserts a range of collection items via iterators
    template <typename TIterator>
    CASTLE_CONSTEXPR void add(TIterator first, TIterator last) CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(++first))
    {
        while (first != last)
        {
            add(*first);
            ++first;
        }
    }

    // Functional syntax overload for a single value insertion
    CASTLE_CONSTEXPR void operator()(TInput value) CASTLE_NOEXCEPT
    {
        add(value);
    }

    // Functional syntax overload for collection range insertion
    template <typename TIterator>
    CASTLE_CONSTEXPR void operator()(TIterator first, TIterator last) CASTLE_NOEXCEPT(CASTLE_NOEXCEPT(++first))
    {
        add(first, last);
    }

    // Evaluates and yields the resulting accumulated mean value
    CASTLE_CONSTEXPR double get_mean() const CASTLE_NOEXCEPT
    {
        if (recalculate_)
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

    // Implicit cast transformation evaluator shortcut mapping to get_mean()
    CASTLE_CONSTEXPR operator double() const CASTLE_NOEXCEPT
    {
        return get_mean();
    }

    // Total tracked records added
    [[nodiscard]] CASTLE_CONSTEXPR size_type count() const CASTLE_NOEXCEPT
    {
        return static_cast<size_type>(counter_);
    }

    // Resets state parameters back to blank
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

#endif // CASTLE_MATH_MEAN_H

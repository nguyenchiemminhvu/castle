// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file online_regression.hpp
 * @brief One-pass covariance, Pearson correlation, and least-squares line fit.
 *
 * Maintains the means of x and y and the co-moment sums Sxx, Syy, Sxy with
 * Welford-style updates, which avoid the cancellation of the textbook
 * `sum(xy) - sum(x) * sum(y) / n` formula. The fitted model is
 * `y = intercept + slope * x`. Typical uses: clock drift (offset vs. time),
 * sensor calibration, and trend detection on a stream.
 *
 * @code
 * #include "castle_ext/statistic/online_regression.hpp"
 *
 * castle::statistic::online_regression<double> drift;
 * drift.push(0.0, 1.0);
 * drift.push(1.0, 3.0);
 * drift.push(2.0, 5.0);
 * double slope = 0.0;
 * double intercept = 0.0;
 * if (drift.fit(slope, intercept) == castle::status::ok)
 * {
 *     // slope == 2.0, intercept == 1.0
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_ONLINE_REGRESSION_HPP
#define CASTLE_EXT_STATISTIC_ONLINE_REGRESSION_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/math/clamp.hpp"
#include "castle/math/sqrt_real.hpp"
#include "castle_ext/statistic/validation.hpp"

namespace castle
{
namespace statistic
{

/**
 * @brief Online bivariate statistics and simple linear regression.
 * @tparam T Arithmetic sample type of both variables.
 * @note Storage: a counter and five `result_t<T>` values. O(1) per pair.
 * @warning Not thread-safe; protect shared instances externally.
 */
template <typename T = double>
class online_regression
{
    static_assert(meta::is_arithmetic<T>::value,
                  "castle::statistic::online_regression: T must be an arithmetic type");

public:
    using value_type  = T;
    using result_type = result_t<T>;
    using size_type   = castle::size_type;

    /** @brief Constructs an empty model. */
    online_regression() CASTLE_NOEXCEPT
    {
        clear();
    }

    /**
     * @brief Adds one (x, y) observation.
     * @return `status::ok`, `status::invalid_argument` for NaN or infinity, or
     *         `status::full` when the counter is exhausted.
     * @note Complexity: O(1).
     */
    castle::status push(T x, T y) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (meta::is_floating_point<T>::value)
        {
            if (!(is_finite(x) && is_finite(y)))
            {
                return castle::status::invalid_argument;
            }
        }
        if (count_ == static_cast<size_type>(-1)) // LCOV_EXCL_BR_LINE: needs 2^N observations.
        {
            return castle::status::full; // LCOV_EXCL_LINE
        }
        CASTLE_CONST result_type x_value = static_cast<result_type>(x);
        CASTLE_CONST result_type y_value = static_cast<result_type>(y);
        ++count_;
        CASTLE_CONST result_type n = static_cast<result_type>(count_);
        CASTLE_CONST result_type dx = x_value - mean_x_;
        CASTLE_CONST result_type dy = y_value - mean_y_;
        mean_x_ += dx / n;
        mean_y_ += dy / n;
        sxx_ += dx * (x_value - mean_x_);
        syy_ += dy * (y_value - mean_y_);
        sxy_ += dx * (y_value - mean_y_);
        return castle::status::ok;
    }

    /** @brief Number of observations. */
    size_type count() CASTLE_CONST CASTLE_NOEXCEPT { return count_; }

    /** @brief Reports whether no observation was added. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return count_ == 0U; }

    /** @brief Mean of x; `status::empty` without observations. */
    castle::status mean_x(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return report(mean_x_, out);
    }

    /** @brief Mean of y; `status::empty` without observations. */
    castle::status mean_y(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return report(mean_y_, out);
    }

    /** @brief Population covariance `Sxy / n`; `status::empty` without observations. */
    castle::status population_covariance(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return covariance(0U, out);
    }

    /** @brief Sample covariance `Sxy / (n - 1)`; `status::empty` with fewer than two observations. */
    castle::status sample_covariance(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return covariance(1U, out);
    }

    /**
     * @brief Pearson correlation coefficient in [-1, 1].
     * @return `status::ok`, `status::empty` with fewer than two observations, or
     *         `status::invalid_argument` when x or y has zero variance.
     */
    castle::status correlation(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (count_ < 2U)
        {
            return castle::status::empty;
        }
        if (!((sxx_ > static_cast<result_type>(0)) && (syy_ > static_cast<result_type>(0))))
        {
            return castle::status::invalid_argument;
        }
        out = castle::math::clamp(sxy_ / castle::math::sqrt_real(sxx_ * syy_),
                                  static_cast<result_type>(-1),
                                  static_cast<result_type>(1));
        return castle::status::ok;
    }

    /**
     * @brief Least-squares line `y = intercept + slope * x`.
     * @return `status::ok`, `status::empty` without observations, or
     *         `status::invalid_argument` when every x is identical. Outputs are
     *         unchanged on failure.
     */
    castle::status fit(result_type& slope, result_type& intercept) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (count_ == 0U)
        {
            return castle::status::empty;
        }
        if (!(sxx_ > static_cast<result_type>(0)))
        {
            return castle::status::invalid_argument;
        }
        CASTLE_CONST result_type fitted_slope = sxy_ / sxx_;
        slope = fitted_slope;
        intercept = mean_y_ - fitted_slope * mean_x_;
        return castle::status::ok;
    }

    /** @brief Discards every observation. */
    void clear() CASTLE_NOEXCEPT
    {
        count_ = 0U;
        mean_x_ = static_cast<result_type>(0);
        mean_y_ = static_cast<result_type>(0);
        sxx_ = static_cast<result_type>(0);
        syy_ = static_cast<result_type>(0);
        sxy_ = static_cast<result_type>(0);
    }

private:
    castle::status report(result_type value, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (count_ == 0U)
        {
            return castle::status::empty;
        }
        out = value;
        return castle::status::ok;
    }

    /** `Sxy / (n - bias)`; needs more than `bias` observations. */
    castle::status covariance(size_type bias, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (count_ <= bias)
        {
            return castle::status::empty;
        }
        out = sxy_ / static_cast<result_type>(count_ - bias);
        return castle::status::ok;
    }

    size_type count_;
    result_type mean_x_;
    result_type mean_y_;
    result_type sxx_;
    result_type syy_;
    result_type sxy_;
};

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_ONLINE_REGRESSION_HPP

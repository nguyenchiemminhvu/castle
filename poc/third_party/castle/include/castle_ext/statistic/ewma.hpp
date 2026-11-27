// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file ewma.hpp
 * @brief Exponentially weighted moving average and variance (first-order IIR smoother).
 *
 * Each sample updates `mean += alpha * (x - mean)`, so a sample's weight
 * decays by `(1 - alpha)` per step: larger `alpha` reacts faster, smaller
 * `alpha` smooths more (`alpha = 2 / (N + 1)` matches the centre of mass of an
 * N-sample moving average). The exponentially weighted variance uses the
 * incremental form of Finch (2009), `var = (1 - alpha) * (var + diff * alpha * diff)`,
 * which stays non-negative. The first sample initializes the mean with zero variance.
 *
 * @code
 * #include "castle_ext/statistic/ewma.hpp"
 *
 * castle::statistic::ewma<float> speed(0.2f);
 * speed.push(10.0f);
 * speed.push(12.0f);
 * float smoothed = 0.0f;
 * if (speed.mean(smoothed) == castle::status::ok)
 * {
 *     // smoothed == 10.4f
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_EWMA_HPP
#define CASTLE_EXT_STATISTIC_EWMA_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/math/sqrt_real.hpp"
#include "castle_ext/statistic/validation.hpp"

namespace castle
{
namespace statistic
{

/**
 * @brief Exponentially weighted moving mean and variance.
 * @tparam T Arithmetic sample type.
 * @note Storage: three `result_t<T>` values and two flags. O(1) per sample.
 * @warning Not thread-safe; protect shared instances externally.
 */
template <typename T = double>
class ewma
{
    static_assert(meta::is_arithmetic<T>::value,
                  "castle::statistic::ewma: T must be an arithmetic type");

public:
    using value_type  = T;
    using result_type = result_t<T>;

    /** @brief Constructs an unconfigured filter; `push()` reports `status::not_configured`. */
    ewma() CASTLE_NOEXCEPT
        : alpha_(static_cast<result_type>(0)),
          mean_(static_cast<result_type>(0)),
          variance_(static_cast<result_type>(0)),
          configured_(false),
          initialized_(false)
    {
    }

    /**
     * @brief Constructs a filter with smoothing factor `alpha`.
     * @note An invalid `alpha` leaves it unconfigured; check `configured()`.
     */
    explicit ewma(result_type alpha) CASTLE_NOEXCEPT
        : ewma()
    {
        (void)configure(alpha);
    }

    /**
     * @brief Sets the smoothing factor and discards the state.
     * @param alpha Weight of the newest sample, in (0, 1]; 1 tracks the input exactly.
     * @return `status::ok`, or `status::invalid_config` (the filter becomes unconfigured).
     */
    castle::status configure(result_type alpha) CASTLE_NOEXCEPT
    {
        clear();
        if (!((alpha > static_cast<result_type>(0)) && (alpha <= static_cast<result_type>(1))))
        {
            alpha_ = static_cast<result_type>(0);
            configured_ = false;
            return castle::status::invalid_config;
        }
        alpha_ = alpha;
        configured_ = true;
        return castle::status::ok;
    }

    /** @brief Reports whether a valid smoothing factor is set. */
    bool configured() CASTLE_CONST CASTLE_NOEXCEPT { return configured_; }

    /** @brief Smoothing factor; zero when unconfigured. */
    result_type alpha() CASTLE_CONST CASTLE_NOEXCEPT { return alpha_; }

    /** @brief Reports whether no sample was received since configuration or `clear()`. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return !initialized_; }

    /**
     * @brief Feeds one sample.
     * @return `status::ok`, `status::not_configured`, or `status::invalid_argument` for NaN or infinity.
     * @note Complexity: O(1).
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
        if (!initialized_)
        {
            mean_ = x;
            variance_ = static_cast<result_type>(0);
            initialized_ = true;
            return castle::status::ok;
        }
        CASTLE_CONST result_type difference = x - mean_;
        CASTLE_CONST result_type increment = alpha_ * difference;
        mean_ += increment;
        variance_ = (static_cast<result_type>(1) - alpha_) * (variance_ + difference * increment);
        return castle::status::ok;
    }

    /** @brief Smoothed value; `status::empty` before the first sample. */
    castle::status mean(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return report(mean_, out);
    }

    /** @brief Exponentially weighted variance; `status::empty` before the first sample. */
    castle::status variance(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return report(variance_, out);
    }

    /** @brief Square root of `variance()`; `status::empty` before the first sample. */
    castle::status stddev(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return report(castle::math::sqrt_real(variance_), out);
    }

    /** @brief Discards the state; the smoothing factor is kept. */
    void clear() CASTLE_NOEXCEPT
    {
        mean_ = static_cast<result_type>(0);
        variance_ = static_cast<result_type>(0);
        initialized_ = false;
    }

private:
    castle::status report(result_type value, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!initialized_)
        {
            return castle::status::empty;
        }
        out = value;
        return castle::status::ok;
    }

    result_type alpha_;
    result_type mean_;
    result_type variance_;
    bool configured_;
    bool initialized_;
};

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_EWMA_HPP

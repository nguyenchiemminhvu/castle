// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file cep.hpp
 * @brief Circular error probable (CEP) over streaming position errors.
 *
 * CEPp is the radius of a circle, centred on the reference point, that
 * contains p percent of the position fixes: CEP50 holds half of them, CEP95
 * holds 95 percent. `cep<TEstimator>` turns any quantile estimator into a CEP
 * tracker: it validates radial errors, converts 2-D/3-D residuals to radii,
 * and offers `cep50()`, `cep68()`, `cep90()`, `cep95()`, `cep99()`, arbitrary
 * percentiles, and the fraction of fixes inside a given radius.
 *
 * Ready-made configurations:
 * | Alias | Estimator | Push | Query | Memory |
 * |---|---|---|---|---|
 * | `exact_cep<T, N>` | `sorted_window` (newest N, exact) | O(N) | O(1) | 2N samples |
 * | `sliding_histogram_cep<T, N, B>` | `sliding_histogram` (newest N, binned) | O(1) | O(B) | N samples + B counters |
 * | `cumulative_cep<T, B>` | expanding `histogram` (whole stream, binned) | O(1) amortized | O(B) | B counters |
 *
 * An estimator type must provide `value_type`, `result_type`, `push(value_type)`,
 * `quantile(result_type, result_type&) const`, `cdf(result_type, result_type&) const`,
 * `count() const`, `empty() const`, and `clear()`.
 *
 * @code
 * #include "castle_ext/statistic/cep.hpp"
 *
 * // Horizontal accuracy of the latest 100 fixes.
 * castle::statistic::exact_cep<float, 100U> horizontal;
 * horizontal.push_residual(0.8f, -1.1f); // east/north error in metres
 * horizontal.push_distance(0.4f);        // radial error in metres
 * float cep95 = 0.0f;
 * if (horizontal.cep95(cep95) == castle::status::ok)
 * {
 *     // 95 % of the retained fixes lie within cep95 metres
 * }
 * @endcode
 */
#ifndef CASTLE_EXT_STATISTIC_CEP_HPP
#define CASTLE_EXT_STATISTIC_CEP_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/status.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/math/hypot.hpp"
#include "castle/utility/forward.hpp"
#include "castle_ext/statistic/validation.hpp"
#include "castle_ext/statistic/histogram.hpp"
#include "castle_ext/statistic/sliding_histogram.hpp"
#include "castle_ext/statistic/sorted_window.hpp"

#include <stdint.h>

namespace castle
{
namespace statistic
{

/**
 * @brief CEP tracker over a quantile estimator of radial errors.
 * @tparam TEstimator Estimator type; see the file documentation for the contract.
 * @note Composition only: all state lives in the estimator, reachable through `estimator()`.
 * @warning Not thread-safe; protect shared instances externally.
 */
template <typename TEstimator>
class cep
{
public:
    using estimator_type = TEstimator;
    using value_type     = typename TEstimator::value_type;
    using result_type    = typename TEstimator::result_type;

    /** @brief Default-constructs the estimator. */
    cep() CASTLE_NOEXCEPT CASTLE_DEFAULT;

    /**
     * @brief Constructs the estimator from the given arguments, e.g. a histogram range.
     * @param first First estimator constructor argument.
     * @param rest Remaining estimator constructor arguments.
     */
    template <typename TArg, typename... TArgs>
    explicit cep(TArg&& first, TArgs&&... rest) CASTLE_NOEXCEPT
        : estimator_(castle::forward<TArg>(first), castle::forward<TArgs>(rest)...)
    {
    }

    /**
     * @brief Adds one radial error (distance from the reference point).
     * @return `status::ok`, `status::invalid_argument` for a negative or non-finite
     *         distance, or the estimator's `push()` status.
     * @note Complexity: that of the estimator's `push()`.
     */
    castle::status push_distance(value_type distance) CASTLE_NOEXCEPT
    {
        if (!valid_distance(distance))
        {
            return castle::status::invalid_argument;
        }
        return estimator_.push(distance);
    }

    /**
     * @brief Adds a horizontal residual; its radius is `hypot(east, north)`.
     * @return `status::ok`, `status::invalid_argument` for a non-finite component,
     *         or the estimator's `push()` status.
     * @note Available for floating-point `value_type` only (no silent truncation).
     */
    castle::status push_residual(result_type east, result_type north) CASTLE_NOEXCEPT
    {
        static_assert(meta::is_floating_point<value_type>::value,
                      "castle::statistic::cep: push_residual() needs a floating-point value_type");
        if (!(is_finite(east) && is_finite(north)))
        {
            return castle::status::invalid_argument;
        }
        return push_distance(static_cast<value_type>(castle::math::hypot(east, north)));
    }

    /**
     * @brief Adds a 3-D residual (spherical error); its radius is `hypot(east, north, up)`.
     * @return `status::ok`, `status::invalid_argument` for a non-finite component,
     *         or the estimator's `push()` status.
     * @note Available for floating-point `value_type` only (no silent truncation).
     */
    castle::status push_residual(result_type east, result_type north, result_type up) CASTLE_NOEXCEPT
    {
        static_assert(meta::is_floating_point<value_type>::value,
                      "castle::statistic::cep: push_residual() needs a floating-point value_type");
        if (!(is_finite(east) && is_finite(north) && is_finite(up)))
        {
            return castle::status::invalid_argument;
        }
        return push_distance(static_cast<value_type>(castle::math::hypot(east, north, up)));
    }

    /**
     * @brief Radius containing the fraction `probability` of the errors.
     * @param probability Containment probability in [0, 1].
     * @return The estimator's `quantile()` status (`status::empty` without samples).
     */
    castle::status quantile(result_type probability, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return estimator_.quantile(probability, out);
    }

    /**
     * @brief Radius containing `percent` percent of the errors.
     * @param percent Containment in [0, 100].
     * @return `status::invalid_argument` outside [0, 100] or NaN, otherwise the `quantile()` status.
     */
    castle::status percentile(result_type percent, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        if (!((percent >= static_cast<result_type>(0)) && (percent <= static_cast<result_type>(100))))
        {
            return castle::status::invalid_argument;
        }
        return estimator_.quantile(percent / static_cast<result_type>(100), out);
    }

    /** @brief CEP50: radius containing 50 % of the errors. */
    castle::status cep50(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return estimator_.quantile(static_cast<result_type>(0.50), out);
    }

    /** @brief CEP68: radius containing 68 % of the errors. */
    castle::status cep68(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return estimator_.quantile(static_cast<result_type>(0.68), out);
    }

    /** @brief CEP90: radius containing 90 % of the errors. */
    castle::status cep90(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return estimator_.quantile(static_cast<result_type>(0.90), out);
    }

    /** @brief CEP95: radius containing 95 % of the errors. */
    castle::status cep95(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return estimator_.quantile(static_cast<result_type>(0.95), out);
    }

    /** @brief CEP99: radius containing 99 % of the errors. */
    castle::status cep99(result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return estimator_.quantile(static_cast<result_type>(0.99), out);
    }

    /**
     * @brief Fraction of the errors at or below `radius` (the empirical CDF).
     * @return The estimator's `cdf()` status.
     */
    castle::status fraction_within(result_type radius, result_type& out) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return estimator_.cdf(radius, out);
    }

    /** @brief Number (or total weight) of errors held by the estimator. */
    auto count() CASTLE_CONST CASTLE_NOEXCEPT { return estimator_.count(); }

    /** @brief Reports whether the estimator holds no error. */
    bool empty() CASTLE_CONST CASTLE_NOEXCEPT { return estimator_.empty(); }

    /** @brief Discards every error. */
    void clear() CASTLE_NOEXCEPT { estimator_.clear(); }

    /** @brief Underlying estimator, e.g. for configuration or extra statistics. */
    estimator_type& estimator() CASTLE_NOEXCEPT { return estimator_; }

    /** @brief Underlying estimator (read-only). */
    CASTLE_CONST estimator_type& estimator() CASTLE_CONST CASTLE_NOEXCEPT { return estimator_; }

private:
    /** Non-negative and finite; each check exists only for the types that need it. */
    static bool valid_distance(value_type distance) CASTLE_NOEXCEPT
    {
        CASTLE_IF_CONSTEXPR (meta::is_floating_point<value_type>::value)
        {
            return is_finite(distance) && (distance >= static_cast<value_type>(0));
        }
        else CASTLE_IF_CONSTEXPR (meta::is_signed<value_type>::value)
        {
            return distance >= static_cast<value_type>(0);
        }
        else
        {
            (void)distance;
            return true;
        }
    }

    estimator_type estimator_{};
};

/** @brief Exact CEP of the newest `Capacity` errors (default: 100 fixes). */
template <typename T = double, size_type Capacity = 100U>
using exact_cep = cep<sorted_window<T, Capacity>>;

/** @brief Binned CEP of the newest `Capacity` errors with O(1) push; construct with the radius range. */
template <typename T, size_type Capacity, size_type BinCount>
using sliding_histogram_cep = cep<sliding_histogram<T, Capacity, BinCount>>;

/** @brief Binned CEP of the whole stream; construct with an initial radius range that grows on demand. */
template <typename T, size_type BinCount, typename TCount = uint32_t>
using cumulative_cep = cep<histogram<T, BinCount, TCount, histogram_range::expanding>>;

} // namespace statistic
} // namespace castle

#endif // CASTLE_EXT_STATISTIC_CEP_HPP

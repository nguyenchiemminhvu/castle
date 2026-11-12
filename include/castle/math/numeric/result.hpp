// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file result.hpp
 * @brief Common result record for iterative numeric algorithms.
 * @details Iterative routines (Romberg integration, root finders) return a
 * `numeric_result<T>` instead of throwing or writing through out-parameters.
 * It bundles the best estimate, an error estimate, the number of iterations
 * used and a `castle::status` describing whether the requested tolerance was
 * reached. The estimate is always populated with the best value found, even
 * when `code` reports a failure other than `invalid_argument`.
 *
 * @code
 * #include "castle/math/numeric/result.hpp"
 *
 * castle::math::numeric_result<double> r = {1.0, 1e-9, 5U, castle::status::ok};
 * if (r.ok())
 * {
 *     // use r.value
 * }
 * @endcode
 */
#ifndef CASTLE_MATH_NUMERIC_RESULT_HPP
#define CASTLE_MATH_NUMERIC_RESULT_HPP

#include "castle/dsa/algorithm.hpp"
#include "castle/core/compiler.hpp"
#include "castle/error/status.hpp"
#include "castle/math/abs.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Outcome of an iterative numeric algorithm.
 * @tparam T Floating-point value type.
 */
template <typename T>
struct numeric_result
{
    /** @brief Best estimate of the sought quantity. */
    T value;
    /** @brief Estimated absolute error of `value`. */
    T error;
    /** @brief Iterations (or refinement levels) consumed. */
    unsigned iterations;
    /**
     * @brief Completion status.
     * @details `status::ok` when the tolerance was met; `status::invalid_argument`
     *          for unusable input (e.g. a root bracket without a sign change);
     *          `status::out_of_range` when the iteration budget ran out first.
     */
    castle::status code;

    /** @brief Returns `true` when the algorithm converged. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool ok() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return code == castle::status::ok;
    }
};

namespace detail
{

/**
 * @brief Tests an error estimate against a mixed absolute/relative tolerance.
 * @tparam T Floating-point type.
 * @param error Non-negative error estimate.
 * @param value Current estimate the error refers to.
 * @param tolerance Tolerance selected by the caller.
 * @return `error <= tolerance * max(1, |value|)`, the same scaling used by
 *         `near_equal()`.
 */
template <typename T>
CASTLE_NODISCARD CASTLE_CONSTEXPR bool within_tolerance(T error, T value, T tolerance) CASTLE_NOEXCEPT
{
    return error <= tolerance * castle::max(static_cast<T>(1), castle::math::abs(value));
}

} // namespace detail
} // namespace math
} // namespace castle

#endif // CASTLE_MATH_NUMERIC_RESULT_HPP

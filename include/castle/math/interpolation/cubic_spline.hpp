// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file cubic_spline.hpp
 * @brief Fixed-capacity C1/C2 cubic spline interpolation over tabulated data.
 * @details `cubic_spline<T, Capacity>` copies up to `Capacity` knots into
 * inline storage and precomputes one tangent per knot, so each segment is a
 * cubic Hermite polynomial (see `curves.hpp`). Three tangent rules are
 * available through `spline_kind`:
 *  - `natural`  : `C2` spline with zero second derivative at both ends.
 *  - `clamped`  : `C2` spline with caller-supplied end slopes.
 *  - `monotone` : `C1` shape-preserving (PCHIP, Fritsch-Carlson/Butland)
 *                 interpolant that never overshoots monotone data, which suits
 *                 sensor calibration curves.
 *
 * Fitting is `O(n)` (Thomas algorithm for the `C2` kinds) and each query is an
 * `O(log n)` segment lookup plus a cubic evaluation. No heap, no exceptions;
 * invalid input is reported through `castle::status`.
 *
 * @code
 * #include "castle/math/interpolation/cubic_spline.hpp"
 *
 * const float xs[] = {0.0F, 1.0F, 2.0F, 3.0F};
 * const float ys[] = {0.0F, 1.0F, 8.0F, 27.0F};
 *
 * castle::math::cubic_spline<float, 8U> spline;
 * if (spline.build(xs, ys, 4U) == castle::status::ok)
 * {
 *     const float y = spline.evaluate(1.5F);
 *     const float slope = spline.derivative(1.5F);
 * }
 * @endcode
 *
 * @note `build()` places `sizeof(T) * Capacity` bytes of scratch space on the
 *       stack for the tridiagonal solve.
 */
#ifndef CASTLE_MATH_INTERPOLATION_CUBIC_SPLINE_HPP
#define CASTLE_MATH_INTERPOLATION_CUBIC_SPLINE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/math/abs.hpp"
#include "castle/math/interpolation/curves.hpp"
#include "castle/math/interpolation/knots.hpp"
#include "castle/math/sign.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Tangent rule used by `cubic_spline::build()`.
 */
enum class spline_kind : uint8_t
{
    natural,  /**< `C2` spline, zero second derivative at both ends. */
    clamped,  /**< `C2` spline, end slopes supplied by the caller. */
    monotone  /**< `C1` shape-preserving interpolant (PCHIP). */
};

/**
 * @brief Owning cubic spline with compile-time knot capacity.
 * @tparam T Floating-point value type.
 * @tparam Capacity Maximum number of knots, at least 2.
 */
template <typename T, size_type Capacity>
class cubic_spline
{
    static_assert(meta::is_floating_point<T>::value,
                  "math::cubic_spline requires a floating-point type");
    static_assert(Capacity >= 2U, "math::cubic_spline requires a capacity of at least two knots");

public:
    /** @brief Value type. */
    using value_type = T;
    /** @brief Size type. */
    using size_type = castle::size_type;
    /** @brief Compact storage type for the knot count. */
    using index_type = meta::smallest_uint_t<Capacity>;

    /** @brief Constructs an empty spline; `valid()` is `false`. */
    CASTLE_CONSTEXPR cubic_spline() CASTLE_NOEXCEPT
        : xs_{}
        , ys_{}
        , slopes_{}
        , count_(0U)
    {
    }

    /** @brief Returns the maximum number of knots. */
    static CASTLE_CONSTEXPR size_type capacity() CASTLE_NOEXCEPT
    {
        return Capacity;
    }

    /**
     * @brief Copies the knots and computes the spline tangents.
     * @param xs Strictly increasing abscissas.
     * @param ys Ordinates, same length as `xs`.
     * @param count Number of knots, in `[2, Capacity]`.
     * @param kind Tangent rule.
     * @param start_slope Slope at the first knot; used by `spline_kind::clamped` only.
     * @param end_slope Slope at the last knot; used by `spline_kind::clamped` only.
     * @return `status::ok` on success; `status::invalid_argument` for null
     *         pointers, fewer than two knots or non-increasing abscissas;
     *         `status::full` when `count` exceeds `Capacity`. On failure the
     *         spline is left empty.
     * @note `O(count)`.
     */
    CASTLE_CONSTEXPR status build(CASTLE_CONST T* xs,
                                  CASTLE_CONST T* ys,
                                  size_type count,
                                  spline_kind kind = spline_kind::natural,
                                  T start_slope = static_cast<T>(0),
                                  T end_slope = static_cast<T>(0)) CASTLE_NOEXCEPT
    {
        count_ = 0U;

        status result = detail::validate_knots(xs, count);
        if ((result == status::ok) && (ys == nullptr))
        {
            result = status::invalid_argument;
        }
        if ((result == status::ok) && (count > Capacity))
        {
            result = status::full;
        }
        if (result != status::ok)
        {
            return result;
        }

        for (size_type i = 0U; i < count; ++i)
        {
            xs_[i] = xs[i];
            ys_[i] = ys[i];
        }
        count_ = static_cast<index_type>(count);

        if (kind == spline_kind::monotone)
        {
            compute_monotone_slopes();
        }
        else
        {
            compute_smooth_slopes(kind, start_slope, end_slope);
        }
        return status::ok;
    }

    /** @brief Returns `true` when the spline holds fitted knots. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return count_ != 0U;
    }

    /** @brief Returns the number of knots, or zero when empty. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return count_;
    }

    /** @brief Returns the smallest abscissa. @warning The spline must be valid. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T x_min() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT_OR_RETURN_VALUE(valid(), // LCOV_EXCL_BR_LINE
                                      CASTLE_ERROR_GENERIC("castle::math::cubic_spline: spline is not built"),
                                      static_cast<T>(0));
        return xs_[0];
    }

    /** @brief Returns the largest abscissa. @warning The spline must be valid. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T x_max() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT_OR_RETURN_VALUE(valid(), // LCOV_EXCL_BR_LINE
                                      CASTLE_ERROR_GENERIC("castle::math::cubic_spline: spline is not built"),
                                      static_cast<T>(0));
        return xs_[count_ - 1U];
    }

    /**
     * @brief Evaluates the spline at `x`.
     * @param x Query position.
     * @param mode Behaviour outside `[x_min(), x_max()]`.
     * @return The interpolated value; with `extrapolation::clamp` the end
     *         ordinates, with `extrapolation::linear` the tangent line at the
     *         nearest end knot (keeps the result `C1` across the boundary).
     * @warning The spline must be valid; an empty spline triggers the Castle
     *          error handler and yields zero.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR
    T evaluate(T x, extrapolation mode = extrapolation::clamp) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT_OR_RETURN_VALUE(valid(), // LCOV_EXCL_BR_LINE
                                      CASTLE_ERROR_GENERIC("castle::math::cubic_spline: spline is not built"),
                                      static_cast<T>(0));
        CASTLE_CONST size_type last = count_ - 1U;
        if (x < xs_[0])
        {
            return (mode == extrapolation::clamp)
                   ? ys_[0]
                   : ys_[0] + slopes_[0] * (x - xs_[0]);
        }
        if (x > xs_[last])
        {
            return (mode == extrapolation::clamp)
                   ? ys_[last]
                   : ys_[last] + slopes_[last] * (x - xs_[last]);
        }

        CASTLE_CONST size_type i = detail::find_segment(xs_, count_, x);
        CASTLE_CONST T width = xs_[i + 1U] - xs_[i];
        return castle::math::cubic_hermite(ys_[i], width * slopes_[i],
                                           ys_[i + 1U], width * slopes_[i + 1U],
                                           (x - xs_[i]) / width);
    }

    /** @brief Equivalent to `evaluate(x)` with clamped extrapolation. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T operator()(T x) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return evaluate(x);
    }

    /**
     * @brief Evaluates the first derivative of the spline at `x`.
     * @param x Query position.
     * @param mode Behaviour outside `[x_min(), x_max()]`.
     * @return `dy/dx`. Outside the table it is zero for `extrapolation::clamp`
     *         and the end-knot slope for `extrapolation::linear`.
     * @warning The spline must be valid; an empty spline triggers the Castle
     *          error handler and yields zero.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR
    T derivative(T x, extrapolation mode = extrapolation::clamp) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT_OR_RETURN_VALUE(valid(), // LCOV_EXCL_BR_LINE
                                      CASTLE_ERROR_GENERIC("castle::math::cubic_spline: spline is not built"),
                                      static_cast<T>(0));
        CASTLE_CONST size_type last = count_ - 1U;
        if (x < xs_[0])
        {
            return (mode == extrapolation::clamp) ? static_cast<T>(0) : slopes_[0];
        }
        if (x > xs_[last])
        {
            return (mode == extrapolation::clamp) ? static_cast<T>(0) : slopes_[last];
        }

        CASTLE_CONST size_type i = detail::find_segment(xs_, count_, x);
        CASTLE_CONST T width = xs_[i + 1U] - xs_[i];
        return castle::math::cubic_hermite_derivative(ys_[i], width * slopes_[i],
                                                      ys_[i + 1U], width * slopes_[i + 1U],
                                                      (x - xs_[i]) / width) / width;
    }

private:
    /** @brief Width of segment `i`. */
    CASTLE_CONSTEXPR T width(size_type i) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return xs_[i + 1U] - xs_[i];
    }

    /** @brief Secant slope of segment `i`. */
    CASTLE_CONSTEXPR T secant(size_type i) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (ys_[i + 1U] - ys_[i]) / width(i);
    }

    /**
     * @brief Produces row `i` of the tridiagonal system `a*m[i-1] + b*m[i] + c*m[i+1] = r`.
     * @details Interior rows enforce continuity of the second derivative;
     *          the first and last rows encode the natural or clamped end condition.
     */
    CASTLE_CONSTEXPR void system_row(size_type i, spline_kind kind, T start_slope, T end_slope,
                                     T& a, T& b, T& c, T& r) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_CONST bool clamped = (kind == spline_kind::clamped);
        if (i == 0U)
        {
            a = static_cast<T>(0);
            b = clamped ? static_cast<T>(1) : static_cast<T>(2);
            c = clamped ? static_cast<T>(0) : static_cast<T>(1);
            r = clamped ? start_slope : static_cast<T>(3) * secant(0U);
        }
        else if (i == count_ - 1U)
        {
            a = clamped ? static_cast<T>(0) : static_cast<T>(1);
            b = clamped ? static_cast<T>(1) : static_cast<T>(2);
            c = static_cast<T>(0);
            r = clamped ? end_slope : static_cast<T>(3) * secant(i - 1U);
        }
        else
        {
            CASTLE_CONST T h_prev = width(i - 1U);
            CASTLE_CONST T h_next = width(i);
            a = h_next;
            b = static_cast<T>(2) * (h_prev + h_next);
            c = h_prev;
            r = static_cast<T>(3) * (h_next * secant(i - 1U) + h_prev * secant(i));
        }
    }

    /** @brief Solves the `C2` tangent system with the Thomas algorithm. */
    CASTLE_CONSTEXPR void compute_smooth_slopes(spline_kind kind, T start_slope, T end_slope) CASTLE_NOEXCEPT
    {
        T upper[Capacity] = {};
        T prev_upper = static_cast<T>(0);
        T prev_rhs = static_cast<T>(0);

        for (size_type i = 0U; i < count_; ++i)
        {
            T a = static_cast<T>(0);
            T b = static_cast<T>(0);
            T c = static_cast<T>(0);
            T r = static_cast<T>(0);
            system_row(i, kind, start_slope, end_slope, a, b, c, r);

            CASTLE_CONST T pivot = b - a * prev_upper;
            upper[i] = c / pivot;
            slopes_[i] = (r - a * prev_rhs) / pivot;
            prev_upper = upper[i];
            prev_rhs = slopes_[i];
        }

        for (size_type i = count_ - 1U; i > 0U; --i)
        {
            slopes_[i - 1U] -= upper[i - 1U] * slopes_[i];
        }
    }

    /**
     * @brief One-sided three-point end tangent, limited to keep monotonicity.
     * @param h0 Width of the segment adjacent to the end knot.
     * @param h1 Width of the next segment inward.
     * @param d0 Secant of the adjacent segment.
     * @param d1 Secant of the next segment inward.
     */
    static CASTLE_CONSTEXPR T monotone_end_slope(T h0, T h1, T d0, T d1) CASTLE_NOEXCEPT
    {
        CASTLE_CONST T slope = ((static_cast<T>(2) * h0 + h1) * d0 - h0 * d1) / (h0 + h1);
        if (castle::math::sign(slope) != castle::math::sign(d0))
        {
            return static_cast<T>(0);
        }
        if ((castle::math::sign(d0) != castle::math::sign(d1)) &&
            (castle::math::abs(slope) > static_cast<T>(3) * castle::math::abs(d0)))
        {
            return static_cast<T>(3) * d0;
        }
        return slope;
    }

    /** @brief Computes PCHIP tangents (weighted harmonic means of adjacent secants). */
    CASTLE_CONSTEXPR void compute_monotone_slopes() CASTLE_NOEXCEPT
    {
        if (count_ == 2U)
        {
            slopes_[0] = secant(0U);
            slopes_[1] = slopes_[0];
            return;
        }

        for (size_type i = 1U; i + 1U < count_; ++i)
        {
            CASTLE_CONST T d_prev = secant(i - 1U);
            CASTLE_CONST T d_next = secant(i);
            if (d_prev * d_next <= static_cast<T>(0))
            {
                slopes_[i] = static_cast<T>(0);
            }
            else
            {
                CASTLE_CONST T w1 = static_cast<T>(2) * width(i) + width(i - 1U);
                CASTLE_CONST T w2 = width(i) + static_cast<T>(2) * width(i - 1U);
                slopes_[i] = (w1 + w2) / (w1 / d_prev + w2 / d_next);
            }
        }

        slopes_[0] = monotone_end_slope(width(0U), width(1U), secant(0U), secant(1U));
        slopes_[count_ - 1U] = monotone_end_slope(width(count_ - 2U), width(count_ - 3U),
                                                  secant(count_ - 2U), secant(count_ - 3U));
    }

    T xs_[Capacity];
    T ys_[Capacity];
    T slopes_[Capacity];
    index_type count_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_INTERPOLATION_CUBIC_SPLINE_HPP

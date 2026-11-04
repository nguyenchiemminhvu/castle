// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file linear_table.hpp
 * @brief Piecewise-linear interpolation over a non-owning table of knots.
 * @details `linear_table<T>` binds to caller-owned `xs` / `ys` arrays (often
 * `const` data in flash) and evaluates the polyline through the points
 * `(xs[i], ys[i])`. The abscissas must be strictly increasing, which is
 * validated once when the table is bound; afterwards each query costs one
 * `O(log n)` segment lookup plus a `lerp`. The class owns no storage, never
 * allocates, and reports bad input through `castle::status`. Queries outside
 * the table follow the selected `extrapolation` policy.
 *
 * @code
 * #include "castle/math/interpolation/linear_table.hpp"
 *
 * static const float volts[] = {0.5F, 1.5F, 2.5F, 3.3F};
 * static const float celsius[] = {-20.0F, 0.0F, 40.0F, 85.0F};
 *
 * const castle::math::linear_table<float> curve(volts, celsius, 4U);
 * const float temperature = curve.evaluate(2.0F); // 20 degrees C
 * @endcode
 *
 * @warning The referenced arrays must outlive the table and must not be
 *          modified while it is in use.
 */
#ifndef CASTLE_MATH_INTERPOLATION_LINEAR_TABLE_HPP
#define CASTLE_MATH_INTERPOLATION_LINEAR_TABLE_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/error_handler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/error/status.hpp"
#include "castle/math/interpolation/knots.hpp"
#include "castle/math/interpolation/lerp.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Non-owning piecewise-linear lookup table.
 * @tparam T Floating-point value type.
 */
template <typename T>
class linear_table
{
    static_assert(meta::is_floating_point<T>::value,
                  "math::linear_table requires a floating-point type");

public:
    /** @brief Value type. */
    using value_type = T;
    /** @brief Size type. */
    using size_type = castle::size_type;

    /** @brief Constructs an unbound table; `valid()` is `false`. */
    CASTLE_CONSTEXPR linear_table() CASTLE_NOEXCEPT
        : xs_(nullptr)
        , ys_(nullptr)
        , count_(0U)
    {
    }

    /**
     * @brief Constructs a table bound to the given knots.
     * @param xs Strictly increasing abscissas.
     * @param ys Ordinates, same length as `xs`.
     * @param count Number of knots, at least 2.
     * @note On invalid input the table stays unbound; check `valid()` or use
     *       `assign()` to obtain the failure status.
     */
    CASTLE_CONSTEXPR linear_table(CASTLE_CONST T* xs, CASTLE_CONST T* ys, size_type count) CASTLE_NOEXCEPT
        : linear_table()
    {
        static_cast<void>(assign(xs, ys, count));
    }

    /**
     * @brief Binds the table to new knots.
     * @param xs Strictly increasing abscissas.
     * @param ys Ordinates, same length as `xs`.
     * @param count Number of knots, at least 2.
     * @return `status::ok` on success; `status::invalid_argument` for null
     *         pointers, fewer than two knots or non-increasing abscissas.
     *         On failure the table is left unbound.
     * @note `O(count)` for validation.
     */
    CASTLE_CONSTEXPR status assign(CASTLE_CONST T* xs, CASTLE_CONST T* ys, size_type count) CASTLE_NOEXCEPT
    {
        status result = detail::validate_knots(xs, count);
        if ((result == status::ok) && (ys == nullptr))
        {
            result = status::invalid_argument;
        }

        if (result != status::ok)
        {
            xs_ = nullptr;
            ys_ = nullptr;
            count_ = 0U;
            return result;
        }

        xs_ = xs;
        ys_ = ys;
        count_ = count;
        return status::ok;
    }

    /** @brief Returns `true` when the table is bound to valid knots. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR bool valid() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return count_ != 0U;
    }

    /** @brief Returns the number of knots, or zero when unbound. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR size_type size() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return count_;
    }

    /** @brief Returns the smallest abscissa. @warning The table must be valid. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T x_min() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT_OR_RETURN_VALUE(valid(), // LCOV_EXCL_BR_LINE
                                      CASTLE_ERROR_GENERIC("castle::math::linear_table: table is not bound"),
                                      static_cast<T>(0));
        return xs_[0];
    }

    /** @brief Returns the largest abscissa. @warning The table must be valid. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T x_max() CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT_OR_RETURN_VALUE(valid(), // LCOV_EXCL_BR_LINE
                                      CASTLE_ERROR_GENERIC("castle::math::linear_table: table is not bound"),
                                      static_cast<T>(0));
        return xs_[count_ - 1U];
    }

    /**
     * @brief Interpolates the table at `x`.
     * @param x Query position.
     * @param mode Behaviour outside `[x_min(), x_max()]`.
     * @return The interpolated value; with `extrapolation::clamp` the end
     *         ordinates, with `extrapolation::linear` the end segment's line.
     * @warning The table must be valid; an unbound table triggers the Castle
     *          error handler and yields zero.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR
    T evaluate(T x, extrapolation mode = extrapolation::clamp) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT_OR_RETURN_VALUE(valid(), // LCOV_EXCL_BR_LINE
                                      CASTLE_ERROR_GENERIC("castle::math::linear_table: table is not bound"),
                                      static_cast<T>(0));
        if (x < xs_[0])
        {
            return (mode == extrapolation::clamp)
                   ? ys_[0]
                   : ys_[0] + segment_slope(0U) * (x - xs_[0]);
        }

        CASTLE_CONST size_type last = count_ - 1U;
        if (x > xs_[last])
        {
            return (mode == extrapolation::clamp)
                   ? ys_[last]
                   : ys_[last] + segment_slope(last - 1U) * (x - xs_[last]);
        }

        CASTLE_CONST size_type i = detail::find_segment(xs_, count_, x);
        return castle::math::lerp(ys_[i], ys_[i + 1U], (x - xs_[i]) / (xs_[i + 1U] - xs_[i]));
    }

    /** @brief Equivalent to `evaluate(x)` with clamped extrapolation. */
    CASTLE_NODISCARD CASTLE_CONSTEXPR T operator()(T x) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return evaluate(x);
    }

    /**
     * @brief Returns the slope of the polyline at `x`.
     * @param x Query position.
     * @param mode Behaviour outside `[x_min(), x_max()]`.
     * @return Slope of the containing segment (the right segment at an
     *         interior knot). Outside the table it is zero for
     *         `extrapolation::clamp` and the end segment's slope for
     *         `extrapolation::linear`.
     * @warning The table must be valid; an unbound table triggers the Castle
     *          error handler and yields zero.
     */
    CASTLE_NODISCARD CASTLE_CONSTEXPR
    T derivative(T x, extrapolation mode = extrapolation::clamp) CASTLE_CONST CASTLE_NOEXCEPT
    {
        CASTLE_ASSERT_OR_RETURN_VALUE(valid(), // LCOV_EXCL_BR_LINE
                                      CASTLE_ERROR_GENERIC("castle::math::linear_table: table is not bound"),
                                      static_cast<T>(0));
        if ((mode == extrapolation::clamp) && ((x < xs_[0]) || (x > xs_[count_ - 1U])))
        {
            return static_cast<T>(0);
        }
        return segment_slope(detail::find_segment(xs_, count_, x));
    }

private:
    CASTLE_CONSTEXPR T segment_slope(size_type i) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return (ys_[i + 1U] - ys_[i]) / (xs_[i + 1U] - xs_[i]);
    }

    CASTLE_CONST T* xs_;
    CASTLE_CONST T* ys_;
    size_type count_;
};

} // namespace math
} // namespace castle

#endif // CASTLE_MATH_INTERPOLATION_LINEAR_TABLE_HPP

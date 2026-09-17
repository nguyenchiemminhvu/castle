// MIT License
// Copyright (c) 2026 nguyenchiemminhvu@gmail.com

/**
 * @file invert.hpp
 * @brief Arithmetic reflection helper with configurable offset and minuend.
 * @details `castle::math::invert<T>` transforms a value with the expression
 * `(offset + minuend) - value`. The default configuration behaves like numeric
 * negation for signed arithmetic types and like reflection around the maximum
 * representable unsigned value for unsigned arithmetic types. This is a
 * subtractive inversion helper, not a reciprocal operation.
 *
 * @code
 * #include "castle/math/invert.hpp"
 *
 * castle::math::invert<int> negate;
 * constexpr int reflected = negate(5);
 *
 * castle::math::invert<unsigned int> mirror(10U, 50U);
 * constexpr unsigned int mapped = mirror(12U);
 * @endcode
 */

#ifndef CASTLE_MATH_INVERT_HPP
#define CASTLE_MATH_INVERT_HPP

#include "castle/core/compiler.hpp"
#include "castle/core/traits.hpp"
#include "castle/core/types.hpp"
#include "castle/core/type_ranges.hpp"

namespace castle
{
namespace math
{

/**
 * @brief Stores the parameters for a simple affine inversion.
 * @tparam T Arithmetic type used for the stored parameters and transformed
 * values.
 * @note The callable result is `minuend_ - (value - offset_)`, which is
 * algebraically equivalent to `(offset_ + minuend_) - value`.
 * @warning Signed arithmetic is not range-checked; if intermediate subtraction
 * overflows, behavior follows the underlying type rules.
 */
template <typename T>
class invert
{
    static_assert(meta::is_arithmetic<T>::value,
                  "castle::math::invert requires an arithmetic type");

public:
    /**
     * @brief Constructs the default inversion mapping.
     * @return Constructs an `invert<T>` with zero offset and a type-dependent
     * default minuend.
     * @note Signed arithmetic types default to `0`, so calling the functor
     * negates the input. Unsigned arithmetic types default to `max()`, so the
     * mapping reflects the value around the full unsigned range.
     * @warning For floating-point types, this helper performs subtraction-based
     * reflection, not reciprocal inversion.
     */
    CASTLE_CONSTEXPR invert() CASTLE_NOEXCEPT
        : offset_(T{0})
        , minuend_(meta::is_signed<T>::value
                   ? T{0}
                   : castle::numeric_limits<T>::max())
    {
    }

    /**
     * @brief Constructs an inversion mapping with explicit parameters.
     * @param offset Value subtracted from the input before reflection.
     * @param minuend Value used as the reflection target after offset
     * adjustment.
     * @return Constructs an `invert<T>` configured with the supplied values.
     * @note Applying the functor computes `(offset + minuend) - value`.
     * @warning No validation is performed; callers must ensure the chosen
     * parameters keep later arithmetic within the desired numeric range.
     */
    CASTLE_CONSTEXPR invert(T offset, T minuend) CASTLE_NOEXCEPT
        : offset_(offset)
        , minuend_(minuend)
    {
    }

    /**
     * @brief Applies the configured inversion mapping to a value.
     * @param value Input value to transform.
     * @return `minuend_ - (value - offset_)`.
     * @note The mapping sends `offset_` to `minuend_` and `offset_ + minuend_`
     * to zero when those sums are representable.
     * @warning Signed overflow is not checked.
     */
    CASTLE_CONSTEXPR T operator()(T value) CASTLE_CONST CASTLE_NOEXCEPT
    {
        return static_cast<T>(minuend_ - static_cast<T>(value - offset_));
    }

    /**
     * @brief Returns the stored offset parameter.
     * @return The configured offset.
     * @note This accessor is `constexpr` and side-effect free.
     * @warning The returned value is configuration only; using it in later
     * arithmetic still requires the caller to manage range safety.
     */
    CASTLE_CONSTEXPR T offset() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return offset_;
    }

    /**
     * @brief Returns the stored minuend parameter.
     * @return The configured minuend.
     * @note This accessor exposes the value that participates in the inversion
     * formula.
     * @warning The stored minuend is not validated against future inputs.
     */
    CASTLE_CONSTEXPR T minuend() CASTLE_CONST CASTLE_NOEXCEPT
    {
        return minuend_;
    }

private:
    T offset_;
    T minuend_;
};

}
}

#endif
